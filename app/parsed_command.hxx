#pragma once

#include "command_type.hxx"

#include <string>
#include <vector>

namespace space_fossils::app {
	struct ParsedCommand
	{
		CommandType type = CommandType::Undefined;
		std::vector<std::string> arguments;
	};
}
