#include "space_fossils/cli/command_dispatcher.hxx"
#include "space_fossils/cli/command_type.hxx"
#include "space_fossils/cli/command_spec.hxx"

namespace space_fossils::cli {
	CommandResult CommandDispatcher::Dispatch(const ParsedCommand& parsedCommand, AppState& appState)
	{
		CommandResult result;

		if (parsedCommand.type == CommandType::Undefined) {
			result.status = CommandStatus::ExecutionFailed;
			result.message = "Undefined parsed command";
			return result;
		}

		if (parsedCommand.type == CommandType::Quit) {
			if (!appState.isQuitRequested) {
				appState.isQuitRequested = true;
				result.status = CommandStatus::Successful;
				result.message = "Quitting";
			}
			else {
				result.status = CommandStatus::ExecutionFailed;
				result.message = "Quitting is in process";
			}

			return result;
		}

		if (parsedCommand.type == CommandType::Help) {
			for (const auto& spec : CommandSpecs) {
				result.message += spec.usage;
				result.message += " ";
				result.message += spec.description;
				result.message += '\n';
			}

			result.status = CommandStatus::Successful;
			return result;
		}

		result.status = CommandStatus::ExecutionFailed;
		result.message = "Unknown command";

		return result;
	}
}
