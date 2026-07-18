#pragma once

#include <string>

namespace space_fossils::cli {
	enum class CommandStatus
	{
		Undefined
		, InvalidState
		, InvalidArgs
		, ExecutionFailed
		, Successful
	};

	struct CommandResult
	{
		CommandStatus status = CommandStatus::Undefined;
		std::string message;
	};
}
