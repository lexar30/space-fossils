#include "command_parser.hxx"
#include "command_spec.hxx"

#include <algorithm>
#include <cctype>

namespace space_fossils::app {
	ParsedResult CommandParser::TryParse(std::string_view input)
	{
		ParsedResult result;
		TokenizedResult tokenizedResult = TryTokenize(input);
		result.status = tokenizedResult.status;

		if (result.status != ParseStatus::Successful) {
			return result;
		}

		const CommandType foundCommandType = TryFindCommandType(tokenizedResult.tokens.front());
		if (foundCommandType == CommandType::Undefined) {
			result.status = ParseStatus::InvalidCommand;
			return result;
		}

		if (!ValidateArgsCount(foundCommandType, tokenizedResult)) {
			result.status = ParseStatus::InvalidArgs;
			return result;
		}

		ParsedCommand parsedCommand;
		parsedCommand.type = foundCommandType;
		for (std::size_t tokenIndex = 1; tokenIndex < tokenizedResult.tokens.size(); ++tokenIndex) {
			parsedCommand.arguments.push_back(std::move(tokenizedResult.tokens[tokenIndex]));
		}
		result.parsedCommand = std::move(parsedCommand);

		return result;
	}

	TokenizedResult CommandParser::TryTokenize(std::string_view input)
	{
		TokenizedResult result;

		bool isInsideQuotes = false;
		bool tokenStarted = false;
		std::string currentToken;

		for (const char symbol : input) {
			if (symbol == '"') {
				isInsideQuotes = !isInsideQuotes;
				tokenStarted = true;
				continue;
			}

			if (std::isspace(static_cast<unsigned char>(symbol)) && !isInsideQuotes) {
				if (tokenStarted) {
					result.tokens.push_back(std::move(currentToken));
					currentToken.clear();
					tokenStarted = false;
				}

				continue;
			}

			currentToken.push_back(symbol);
			tokenStarted = true;
		}

		if (isInsideQuotes) {
			result.status = ParseStatus::InvalidQuotes;
			return result;
		}

		if (tokenStarted) {
			result.tokens.push_back(std::move(currentToken));
		}

		result.status = result.tokens.empty()
			? ParseStatus::EmptyInput
			: ParseStatus::Successful;

		return result;
	}

	CommandType CommandParser::TryFindCommandType(std::string_view input)
	{
		if (input.empty()) {
			return CommandType::Undefined;
		}

		const auto it = std::find_if(CommandSpecs.begin(), CommandSpecs.end(),
			[&input](const auto& spec) {
				return input == spec.name;
			});

		if (it == CommandSpecs.end()) {
			return CommandType::Undefined;
		}

		return it->type;
	}

	bool CommandParser::ValidateArgsCount(CommandType commandType, const TokenizedResult& tokenizedResult)
	{
		if (commandType == CommandType::Undefined) {
			return false;
		}

		if (tokenizedResult.tokens.empty()) {
			return false;
		}

		const auto it = std::find_if(CommandSpecs.begin(), CommandSpecs.end(),
			[&commandType](const auto& spec) {
				return commandType == spec.type;
			});

		if (it == CommandSpecs.end()) {
			return false;
		}

		return (tokenizedResult.tokens.size() - 1) == it->argsCount;
	}
}
