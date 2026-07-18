#include "space_fossils/cli/cli_app.hxx"
#include "space_fossils/cli/cli_renderer.hxx"
#include "space_fossils/cli/command_spec.hxx"

#include "space_fossils_tests/micro_test_framework.hxx"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

		std::string MakeExpectedHelpMessage()
		{
			std::size_t labelWidth = 0;
			std::vector<std::string> labels;
			for (const CommandSpec& spec : CommandSpecs) {
				std::string label(spec.usage);
				if (!spec.shortName.empty()) {
					label += " (";
					label += spec.shortName;
					label += ')';
				}
				labelWidth = std::max(labelWidth, label.size());
				labels.push_back(std::move(label));
			}

			std::ostringstream message;
			for (std::size_t index = 0; index < CommandSpecs.size(); ++index) {
				message << labels[index]
					<< std::string(labelWidth - labels[index].size(), ' ')
					<< "  -  " << CommandSpecs[index].description << '\n';
			}
			return message.str();
		}
	}

	SF_TEST(cli_app, ImmediateEndOfInputPrintsPromptAndStops)
	{
		SF_ASSERT_EQ(RunCli(""), CliRenderer::PromptMessage);
	}

	SF_TEST(cli_app, ExecutesHelpAndQuitInOneSession)
	{
		const std::string expectedOutput =
			std::string(CliRenderer::PromptMessage)
			+ MakeExpectedHelpMessage()
			+ CliRenderer::PromptMessage
			+ "Quitting\n";

		SF_ASSERT_EQ(RunCli("help\nquit\n"), expectedOutput);
	}

	SF_TEST(cli_app, RendersParseErrorsAndContinuesReadingCommands)
	{
		const std::string expectedOutput =
			std::string(CliRenderer::PromptMessage) + "Empty input\n"
			+ CliRenderer::PromptMessage + "Invalid command\n"
			+ CliRenderer::PromptMessage + "Invalid args\n"
			+ CliRenderer::PromptMessage + "Invalid quotes\n"
			+ CliRenderer::PromptMessage + "Quitting\n";

		SF_ASSERT_EQ(
			RunCli(" \nunknown\nhelp extra\n\"help\nquit\n")
			, expectedOutput);
	}

	SF_TEST(cli_app, StopsAfterQuitWithoutProcessingRemainingInput)
	{
		SF_ASSERT_EQ(
			RunCli("quit\nhelp\n"),
			std::string(CliRenderer::PromptMessage) + "Quitting\n");
	}

	SF_TEST(cli_app, ProcessesFinalCommandWithoutTrailingNewline)
	{
		SF_ASSERT_EQ(
			RunCli("quit"),
			std::string(CliRenderer::PromptMessage) + "Quitting\n");
	}

	SF_TEST(cli_app, ReprintsPromptBeforeWaitingForNextCommand)
	{
		const std::string expectedOutput =
			std::string(CliRenderer::PromptMessage)
			+ MakeExpectedHelpMessage()
			+ CliRenderer::PromptMessage;

		SF_ASSERT_EQ(RunCli("help\n"), expectedOutput);
	}
}
