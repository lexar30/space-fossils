#pragma once

#include "space_fossils/cli/command_type.hxx"

#include <array>
#include <cstddef>
#include <string_view>

namespace space_fossils::cli {
	struct CommandSpec
	{
		CommandType type;
		std::string_view name;
		std::string_view shortName;
		std::string_view usage;
		std::string_view description;
		std::size_t argsCountMin;
		std::size_t argsCountMax;
	};

	inline constexpr std::array CommandSpecs{
		CommandSpec{
			CommandType::Help
			, "help"
			, "h"
			, "help"
			, "Shows available commands"
			, 0
			, 0
		}
		, CommandSpec{
			CommandType::Quit
			, "quit"
			, "q"
			, "quit"
			, "Quits app"
			, 0
			, 0
		}

		, CommandSpec{
			CommandType::SetUnits
			, "units"
			, ""
			, "units <binary|decimal>"
			, "Sets file size units"
			, 1
			, 1
		}

		, CommandSpec{
			CommandType::Scan
			, "scan"
			, ""
			, "scan <path>"
			, "Scans a directory"
			, 1
			, 1
		}
		, CommandSpec{
			CommandType::Rescan
			, "rescan"
			, ""
			, "rescan"
			, "Rescans current directory"
			, 0
			, 0
		}

		, CommandSpec{
			CommandType::SaveSnapshot
			, "save"
			, ""
			, "save <path>"
			, "Saves snapshot to a file"
			, 1
			, 1
		}
		, CommandSpec{
			CommandType::LoadSnapshot
			, "load"
			, ""
			, "load <path>"
			, "Loads snapshot from a file"
			, 1
			, 1
		}

		, CommandSpec{
			CommandType::ShowTree
			, "tree"
			, "t"
			, "tree"
			, "Shows current subtree"
			, 0
			, 0
		}
		, CommandSpec{
			CommandType::ListChildren
			, "children"
			, "ls"
			, "children"
			, "Lists current directory children"
			, 0
			, 0
		}
		, CommandSpec{
			CommandType::ShowInfo
			, "info"
			, "i"
			, "info"
			, "Shows current entry information"
			, 0
			, 0
		}
		, CommandSpec{
			CommandType::ShowTop
			, "top"
			, ""
			, "top [count]"
			, "Shows the largest entries in the current directory"
			, 0
			, 1
		}

		, CommandSpec{
			CommandType::ChangeDirectory
			, "cd"
			, ""
			, "cd <path>"
			, "Changes current directory"
			, 1
			, 1
		}
		, CommandSpec{
			CommandType::PrintWorkingDirectory
			, "pwd"
			, ""
			, "pwd"
			, "Shows current directory path"
			, 0
			, 0
		}

		, CommandSpec{
			CommandType::Reset
			, "reset"
			, "rst"
			, "reset"
			, "Resets current file tree"
			, 0
			, 0
		}
	};

	constexpr const CommandSpec* TryFindCommandSpecByType(CommandType commandType)
	{
		for (const CommandSpec& spec : CommandSpecs) {
			if (spec.type == commandType) {
				return &spec;
			}
		}

		return nullptr;
	}

	constexpr const CommandSpec* TryFindCommandSpecByName(std::string_view commandName)
	{
		for (const CommandSpec& spec : CommandSpecs) {
			if (commandName == spec.name) {
				return &spec;
			}

			if (!spec.shortName.empty() &&
				commandName == spec.shortName) {
				return &spec;
			}
		}

		return nullptr;
	}
}
