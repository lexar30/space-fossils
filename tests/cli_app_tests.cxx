#include "space_fossils/cli/cli_app.hxx"

#include "space_fossils_tests/micro_test_framework.hxx"

#include <sstream>
#include <string>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::cli;

		std::string RunCli(std::string inputText)
		{
			std::istringstream input(std::move(inputText));
			std::ostringstream output;
			CliApp app;

			app.Run(input, output);

			return output.str();
		}
	}

	SF_TEST(cli_app, ImmediateEndOfInputPrintsPromptAndStops)
	{
		SF_ASSERT_EQ(RunCli(""), "sf> ");
	}

	SF_TEST(cli_app, ExecutesHelpAndQuitInOneSession)
	{
		const std::string expectedOutput =
			"sf> help Shows available commands\n"
			"quit Quits app\n"
			"\n"
			"sf> Quitting\n";

		SF_ASSERT_EQ(RunCli("help\nquit\n"), expectedOutput);
	}

	SF_TEST(cli_app, RendersParseErrorsAndContinuesReadingCommands)
	{
		const std::string expectedOutput =
			"sf> Empty input\n"
			"sf> Invalid command\n"
			"sf> Invalid args\n"
			"sf> Invalid quotes\n"
			"sf> Quitting\n";

		SF_ASSERT_EQ(
			RunCli(" \nunknown\nhelp extra\n\"help\nquit\n")
			, expectedOutput);
	}

	SF_TEST(cli_app, StopsAfterQuitWithoutProcessingRemainingInput)
	{
		SF_ASSERT_EQ(RunCli("quit\nhelp\n"), "sf> Quitting\n");
	}

	SF_TEST(cli_app, ProcessesFinalCommandWithoutTrailingNewline)
	{
		SF_ASSERT_EQ(RunCli("quit"), "sf> Quitting\n");
	}

	SF_TEST(cli_app, ReprintsPromptBeforeWaitingForNextCommand)
	{
		const std::string expectedOutput =
			"sf> help Shows available commands\n"
			"quit Quits app\n"
			"\n"
			"sf> ";

		SF_ASSERT_EQ(RunCli("help\n"), expectedOutput);
	}
}
