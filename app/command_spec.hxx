#pragma once

#include "command_type.hxx"

#include <array>
#include <cstddef>
#include <string_view>

namespace space_fossils::app {
	struct CommandSpec
	{
		const CommandType type;
		const std::string_view name;
		const std::string_view usage;
		const std::string_view description;
		const std::size_t argsCount;
	};

	constexpr std::array CommandSpecs{
		CommandSpec{
			CommandType::Help
			, "help"
			, "help"
			, "Shows available commands"
			,0
		}
		, CommandSpec{
			CommandType::Quit
			, "quit"
			, "quit"
			, "Quits app"
			,0
		}
	};
}
