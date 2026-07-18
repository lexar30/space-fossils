#pragma once

#include "app_state.hxx"
#include "command_result.hxx"
#include "parsed_command.hxx"

#include <string>
#include <vector>

namespace space_fossils::cli {
	class CommandDispatcher
	{
	public:
		static constexpr const char UndefinedCommandTypeMessage[] = "Undefined parsed command";
		static constexpr const char QuittingMessage[] = "Quitting";

	public:
		static CommandResult Dispatch(const ParsedCommand& parsedCommand, AppState& appState);

	private:
		static CommandResult ExecuteUndefined();
		static CommandResult ExecuteHelp(AppState& appState);
		static CommandResult ExecuteQuit(AppState& appState);
		static CommandResult ExecuteSetUnits(const std::vector<std::string>& arguments, AppState& appState);
		static CommandResult ExecuteScan(const std::vector<std::string>& arguments, AppState& appState);
		static CommandResult ExecuteRescan(AppState& appState);
		static CommandResult ExecuteSaveSnapshot(const std::vector<std::string>& arguments, AppState& appState);
		static CommandResult ExecuteLoadSnapshot(const std::vector<std::string>& arguments, AppState& appState);
		static CommandResult ExecuteShowTree(AppState& appState);
		static CommandResult ExecuteListChildren(AppState& appState);
		static CommandResult ExecuteShowInfo(AppState& appState);
		static CommandResult ExecuteShowTop(AppState& appState);
		static CommandResult ExecuteChangeDirectory(const std::vector<std::string>& arguments, AppState& appState);
		static CommandResult ExecutePrintWorkingDirectory(AppState& appState);
		static CommandResult ExecuteReset(AppState& appState);
	};
}
