#include "space_fossils/cli/command_parser.hxx"

#include "space_fossils_tests/micro_test_framework.hxx"

#include <initializer_list>
#include <string>
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

		void AssertParsedCommand(
			std::string_view input
			, CommandType expectedType
			, std::initializer_list<std::string_view> expectedArguments = {})
		{
			ParsedResult result = CommandParser::TryParse(input);

			SF_ASSERT_EQ(result.status, ParseStatus::Successful);
			SF_ASSERT_EQ(result.parsedCommand.has_value(), true);
			SF_ASSERT_EQ(result.parsedCommand->type, expectedType);
			SF_ASSERT_EQ(result.parsedCommand->arguments.size(), expectedArguments.size());

			std::size_t argumentIndex = 0;
			for (std::string_view expectedArgument : expectedArguments) {
				SF_ASSERT_EQ(result.parsedCommand->arguments[argumentIndex], expectedArgument);
				++argumentIndex;
			}
		}
	}

	SF_TEST(command_parser, ParsesKnownCommands)
	{
		AssertParsedCommand("help", CommandType::Help);
		AssertParsedCommand("quit", CommandType::Quit);
		AssertParsedCommand("units binary", CommandType::SetUnits, { "binary" });
		AssertParsedCommand("scan \"C:\\Program Files\"", CommandType::Scan, { "C:\\Program Files" });
		AssertParsedCommand("rescan", CommandType::Rescan);
		AssertParsedCommand("save snapshot.bin", CommandType::SaveSnapshot, { "snapshot.bin" });
		AssertParsedCommand("load snapshot.bin", CommandType::LoadSnapshot, { "snapshot.bin" });
		AssertParsedCommand("tree", CommandType::ShowTree);
		AssertParsedCommand("children", CommandType::ListChildren);
		AssertParsedCommand("info", CommandType::ShowInfo);
		AssertParsedCommand("top", CommandType::ShowTop);
		AssertParsedCommand("cd src/core", CommandType::ChangeDirectory, { "src/core" });
		AssertParsedCommand("pwd", CommandType::PrintWorkingDirectory);
	}

	SF_TEST(command_parser, ParsesShortCommandNames)
	{
		AssertParsedCommand("h", CommandType::Help);
		AssertParsedCommand("q", CommandType::Quit);
		AssertParsedCommand("t", CommandType::ShowTree);
		AssertParsedCommand("ls", CommandType::ListChildren);
		AssertParsedCommand("i", CommandType::ShowInfo);
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
		AssertParseFailure("units", ParseStatus::InvalidArgs);
		AssertParseFailure("units binary extra", ParseStatus::InvalidArgs);
		AssertParseFailure("scan", ParseStatus::InvalidArgs);
		AssertParseFailure("rescan now", ParseStatus::InvalidArgs);
		AssertParseFailure("save", ParseStatus::InvalidArgs);
		AssertParseFailure("load", ParseStatus::InvalidArgs);
		AssertParseFailure("cd", ParseStatus::InvalidArgs);
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
