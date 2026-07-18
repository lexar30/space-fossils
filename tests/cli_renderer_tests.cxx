#include "space_fossils/cli/cli_renderer.hxx"

#include "space_fossils/cli/command_parser.hxx"
#include "space_fossils/cli/command_result.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <ostream>
#include <sstream>
#include <string>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::cli;

		class SyncCountingBuffer final : public std::stringbuf
		{
		public:
			int syncCount = 0;

		protected:
			int sync() override
			{
				++syncCount;
				return std::stringbuf::sync();
			}
		};

		void AssertParseError(ParseStatus status, const std::string& expectedOutput)
		{
			std::ostringstream output;

			CliRenderer::RenderParseError(output, status);

			SF_ASSERT_EQ(output.str(), expectedOutput);
		}

		void AssertCommandResult(
			CommandStatus status
			, const std::string& message
			, const std::string& expectedOutput)
		{
			std::ostringstream output;
			CommandResult result;
			result.status = status;
			result.message = message;

			CliRenderer::RenderCommandResult(output, result);

			SF_ASSERT_EQ(output.str(), expectedOutput);
		}
	}

	SF_TEST(cli_renderer, PromptHasExpectedTextAndFlushesOutput)
	{
		SyncCountingBuffer buffer;
		std::ostream output(&buffer);

		CliRenderer::RenderPrompt(output);

		SF_ASSERT_EQ(buffer.str(), CliRenderer::PromptMessage);
		SF_ASSERT_EQ(buffer.syncCount, 1);
	}

	SF_TEST(cli_renderer, RendersEveryParseError)
	{
		AssertParseError(ParseStatus::EmptyInput, "Empty input\n");
		AssertParseError(ParseStatus::InvalidCommand, "Invalid command\n");
		AssertParseError(ParseStatus::InvalidArgs, "Invalid args\n");
		AssertParseError(ParseStatus::InvalidQuotes, "Invalid quotes\n");
	}

	SF_TEST(cli_renderer, SuccessfulAndUnknownParseStatusesProduceNoOutput)
	{
		AssertParseError(ParseStatus::Successful, "");
		AssertParseError(static_cast<ParseStatus>(999), "");
	}

	SF_TEST(cli_renderer, RendersSuccessfulCommandMessageWithoutPrefix)
	{
		AssertCommandResult(CommandStatus::Successful, "Done", "Done\n");
	}

	SF_TEST(cli_renderer, RendersCommandFailurePrefixes)
	{
		AssertCommandResult(
			CommandStatus::InvalidState
			, "Storage is empty"
			, "Command failed by InvalidState: Storage is empty\n");
		AssertCommandResult(
			CommandStatus::ExecutionFailed
			, "Cannot open file"
			, "Command execution failed: Cannot open file\n");
	}

	SF_TEST(cli_renderer, UndefinedAndUnknownCommandStatusesProduceNoOutput)
	{
		AssertCommandResult(CommandStatus::Undefined, "Ignored", "");
		AssertCommandResult(static_cast<CommandStatus>(999), "Ignored", "");
	}

	SF_TEST(cli_renderer, PreservesMultilineCommandMessage)
	{
		AssertCommandResult(
			CommandStatus::Successful
			, "first line\nsecond line"
			, "first line\nsecond line\n");
	}
}
