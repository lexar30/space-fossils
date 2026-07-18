#pragma once

#include <iosfwd>

namespace space_fossils::cli {
	struct CommandResult;
	enum class ParseStatus;

	class CliRenderer
	{
	public:
		static constexpr const char PromptMessage[] = "\nspase-fossils> ";

		static constexpr const char ParseStatusEmptyInputMessage[] = "Empty input";
		static constexpr const char ParseStatusInvalidCommandMessage[] = "Invalid command";
		static constexpr const char ParseStatusInvalidArgsMessage[] = "Invalid args";
		static constexpr const char ParseStatusInvalidQuotesMessage[] = "Invalid quotes";

		static constexpr const char CommandStatusInvalidStateMessage[] = "Command failed by InvalidState:";
		static constexpr const char CommandStatusInvalidArgsMessage[] = "Command failed by InvalidArgs:";
		static constexpr const char CommandStatusExecutionFailedMessage[] = "Command execution failed:";

	public:
		static void RenderPrompt(std::ostream& output);
		static void RenderParseError(std::ostream& output, ParseStatus parseStatus);
		static void RenderCommandResult(std::ostream& output, const CommandResult& commandResult);
	};
}
