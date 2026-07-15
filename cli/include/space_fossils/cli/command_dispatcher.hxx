#pragma once

#include "app_state.hxx"
#include "command_result.hxx"
#include "parsed_command.hxx"

namespace space_fossils::cli {
	class CommandDispatcher
	{
	public:
		static CommandResult Dispatch(const ParsedCommand& parsedCommand, AppState& appState);
	};
}
