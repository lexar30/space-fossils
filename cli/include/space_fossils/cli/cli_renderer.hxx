#pragma once

#include <iosfwd>

namespace space_fossils::cli {
	struct CommandResult;
	enum class ParseStatus;

	class CliRenderer
	{
	public:
		static void RenderPrompt(std::ostream& output);
		static void RenderParseError(std::ostream& output, ParseStatus parseStatus);
		static void RenderCommandResult(std::ostream& output, const CommandResult& commandResult);
	};
}
