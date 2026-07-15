#include "space_fossils/cli/command_parser.hxx"

#include "space_fossils_tests/micro_test_framework.hxx"

#include <string_view>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::cli;

		void AssertParseFailure(std::string_view input, ParseStatus expectedStatus)
		{
			ParsedResult result = CommandParser::TryParse(input);

			SF_ASSERT_EQ(result.status, expectedStatus);
			SF_ASSERT_EQ(result.parsedCommand.has_value(), false);
		}

		void AssertParsedCommand(std::string_view input, CommandType expectedType)
		{
			ParsedResult result = CommandParser::TryParse(input);

			SF_ASSERT_EQ(result.status, ParseStatus::Successful);
			SF_ASSERT_EQ(result.parsedCommand.has_value(), true);
			SF_ASSERT_EQ(result.parsedCommand->type, expectedType);
			SF_ASSERT_EQ(result.parsedCommand->arguments.empty(), true);
		}
	}

	SF_TEST(command_parser, ParsesKnownCommands)
	{
		AssertParsedCommand("help", CommandType::Help);
		AssertParsedCommand("quit", CommandType::Quit);
	}

	SF_TEST(command_parser, IgnoresLeadingTrailingAndRepeatedWhitespace)
	{
		AssertParsedCommand("   help   ", CommandType::Help);
		AssertParsedCommand("\t\tquit\t\t", CommandType::Quit);
		AssertParsedCommand(" \t\r\n help \t\r\n ", CommandType::Help);
	}

	SF_TEST(command_parser, AcceptsQuotedKnownCommandName)
	{
		AssertParsedCommand("\"help\"", CommandType::Help);
		AssertParsedCommand("  \"quit\"  ", CommandType::Quit);
	}

	SF_TEST(command_parser, ReportsEmptyInput)
	{
		AssertParseFailure({}, ParseStatus::EmptyInput);
		AssertParseFailure("", ParseStatus::EmptyInput);
		AssertParseFailure(" ", ParseStatus::EmptyInput);
		AssertParseFailure(" \t\r\n\f\v ", ParseStatus::EmptyInput);
	}

	SF_TEST(command_parser, ReportsUnknownCommands)
	{
		AssertParseFailure("unknown", ParseStatus::InvalidCommand);
		AssertParseFailure("\"unknown command\"", ParseStatus::InvalidCommand);
		AssertParseFailure("\"\"", ParseStatus::InvalidCommand);
		AssertParseFailure("\"   \"", ParseStatus::InvalidCommand);
	}

	SF_TEST(command_parser, RejectsUnexpectedArguments)
	{
		AssertParseFailure("help extra", ParseStatus::InvalidArgs);
		AssertParseFailure("quit now", ParseStatus::InvalidArgs);
		AssertParseFailure("help first second", ParseStatus::InvalidArgs);
	}

	SF_TEST(command_parser, PreservesEmptyAndWhitespaceOnlyQuotedArguments)
	{
		AssertParseFailure("help \"\"", ParseStatus::InvalidArgs);
		AssertParseFailure("help \"   \"", ParseStatus::InvalidArgs);
	}

	SF_TEST(command_parser, ReportsUnterminatedQuotes)
	{
		AssertParseFailure("\"", ParseStatus::InvalidQuotes);
		AssertParseFailure("\"help", ParseStatus::InvalidQuotes);
		AssertParseFailure("help\"", ParseStatus::InvalidQuotes);
		AssertParseFailure("help \"unfinished argument", ParseStatus::InvalidQuotes);
	}

	SF_TEST(command_parser, ReportsInvalidQuotesBeforeCommandAndArgumentValidation)
	{
		AssertParseFailure("unknown \"unfinished", ParseStatus::InvalidQuotes);
		AssertParseFailure("help extra \"unfinished", ParseStatus::InvalidQuotes);
	}
}
