#pragma once

#include "space_fossils/cli/command_type.hxx"
#include "space_fossils/cli/parsed_command.hxx"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace space_fossils::cli {
	enum class ParseStatus
	{
		EmptyInput
		, InvalidCommand
		, InvalidArgs
		, InvalidQuotes
		, Successful
	};

	struct ParsedResult
	{
		ParseStatus status = ParseStatus::EmptyInput;
		std::optional<ParsedCommand> parsedCommand;
	};

	struct TokenizedResult
	{
		ParseStatus status = ParseStatus::EmptyInput;
		std::vector<std::string> tokens;
	};

	class CommandParser
	{
	public:
		static ParsedResult TryParse(std::string_view input);

	private:
		static TokenizedResult TryTokenize(std::string_view input);
		static CommandType TryFindCommandType(std::string_view input);
		static bool ValidateArgsCount(CommandType commandType, const TokenizedResult& tokenizedResult);
	};
}
