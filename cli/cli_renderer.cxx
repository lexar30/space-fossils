#include "space_fossils/cli/cli_renderer.hxx"

#include "space_fossils/cli/command_parser.hxx"
#include "space_fossils/cli/command_result.hxx"

#include <iostream>
#include <string>

namespace space_fossils::cli {
	void CliRenderer::RenderPrompt(std::ostream& output)
	{
		output << PromptMessage << std::flush;
	}

	void CliRenderer::RenderParseError(std::ostream& output, ParseStatus parseStatus)
	{
		if (parseStatus == ParseStatus::Successful) {
			return;
		}

		std::string outputStr;

		switch (parseStatus)
		{
		case ParseStatus::EmptyInput:
			output << ParseStatusEmptyInputMessage << '\n';
			return;

		case ParseStatus::InvalidCommand:
			output << ParseStatusInvalidCommandMessage << '\n';
			return;

		case ParseStatus::InvalidArgs:
			output << ParseStatusInvalidArgsMessage << '\n';
			return;

		case ParseStatus::InvalidQuotes:
			output << ParseStatusInvalidQuotesMessage << '\n';
			return;

		case ParseStatus::Successful:
			return;

		default:
			return;
		}
	}

	void CliRenderer::RenderCommandResult(std::ostream& output, const CommandResult& commandResult)
	{
		switch (commandResult.status)
		{
		case CommandStatus::InvalidState:
			output << CommandStatusInvalidStateMessage << ' ' << commandResult.message << '\n';
			return;

		case CommandStatus::InvalidArgs:
			output << CommandStatusInvalidArgsMessage << ' ' << commandResult.message << '\n';
			return;

		case CommandStatus::ExecutionFailed:
			output << CommandStatusExecutionFailedMessage << ' ' << commandResult.message << '\n';
			return;

		case CommandStatus::Successful:
			if (commandResult.message.empty()) {
				return;
			}

			output << commandResult.message;
			if (commandResult.message.back() != '\n') {
				output << '\n';
			}
			return;

		default:
			return;
		}
	}
}
