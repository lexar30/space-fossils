#include "space_fossils/cli/cli_renderer.hxx"

#include "space_fossils/cli/command_parser.hxx"
#include "space_fossils/cli/command_result.hxx"

#include <iostream>
#include <string>

namespace space_fossils::cli {
	void CliRenderer::RenderPrompt(std::ostream& output)
	{
		output << "sf> " << std::flush;
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
			outputStr = "Empty input";
			break;

		case ParseStatus::InvalidCommand:
			outputStr = "Invalid command";
			break;

		case ParseStatus::InvalidArgs:
			outputStr = "Invalid args";
			break;

		case ParseStatus::InvalidQuotes:
			outputStr = "Invalid quotes";
			break;

		case ParseStatus::Successful:
			return;

		default:
			return;
		}

		output << outputStr << '\n';
	}

	void CliRenderer::RenderCommandResult(std::ostream& output, const CommandResult& commandResult)
	{
		std::string outputStr;

		switch (commandResult.status)
		{
		case CommandStatus::InvalidState:
			outputStr += "Command failed by InvalidState: ";
			break;
		case CommandStatus::ExecutionFailed:
			outputStr += "Command execution failed: ";
			break;
		case CommandStatus::Successful:
			break;
		default:
			return;
		}

		outputStr += commandResult.message;

		output << outputStr << '\n';
	}
}
