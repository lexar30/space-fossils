#pragma once

#include "space_fossils/cli/command_type.hxx"

#include <string>
#include <vector>

namespace space_fossils::cli {
	struct ParsedCommand
	{
		CommandType type = CommandType::Undefined;
		std::vector<std::string> arguments;
	};
}
