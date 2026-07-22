#include "space_fossils/cli/cli_app.hxx"

#include "space_fossils/cli/cli_renderer.hxx"
#include "space_fossils/cli/command_parser.hxx"
#include "space_fossils/cli/command_dispatcher.hxx"

#include <iostream>
#include <string>

namespace space_fossils::cli {
	void CliApp::Run(std::istream& input, std::ostream& output)
	{
		while (!appState.isQuitRequested) {
			CliRenderer::RenderPrompt(output);

			// TODO: kinda dangerous, win input encoding is unknown
			std::string line;
			if (!std::getline(input, line)) {
				break;
			}

			const ParsedResult parsedResult = CommandParser::TryParse(line);
			if (parsedResult.status != ParseStatus::Successful) {
				CliRenderer::RenderParseError(output, parsedResult.status);
				continue;
			}

			const CommandResult commandResult = CommandDispatcher::Dispatch(parsedResult.parsedCommand.value(), appState);
			CliRenderer::RenderCommandResult(output, commandResult);
		}
	}
}
