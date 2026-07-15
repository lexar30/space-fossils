#include "space_fossils/cli/command_dispatcher.hxx"

#include "space_fossils/cli/command_spec.hxx"
#include "space_fossils/cli/command_type.hxx"
#include "space_fossils/core/file_tree/model/tree_pool_bundle.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::cli;
		using namespace space_fossils::core::file_tree;

		ParsedCommand MakeCommand(CommandType type)
		{
			ParsedCommand command;
			command.type = type;
			return command;
		}

		std::string MakeExpectedHelpMessage()
		{
			std::string message;
			for (const CommandSpec& spec : CommandSpecs) {
				message += spec.usage;
				message += " ";
				message += spec.description;
				message += '\n';
			}

			return message;
		}

		void PopulateTree(AppState& state)
		{
			TreePoolBundle bundle;
			bundle.namePool = std::make_unique<NamePool>(DefaultNameBlockSize);
			bundle.nodePool = std::make_unique<NodePool>(DefaultNodeBlockSize);
			bundle.root = bundle.nodePool->Create();
			SF_ASSERT_EQ(bundle.root != nullptr, true);
			bundle.root->entryType = EntryType::Directory;
			bundle.createdNodesCount = 1;

			std::optional<AppliedChange> change = state.context->storage.TryAdoptRoot(std::move(bundle));
			SF_ASSERT_EQ(change.has_value(), true);
		}
	}

	SF_TEST(command_dispatcher, UndefinedCommandReturnsFailureWithoutChangingState)
	{
		AppState state;
		PopulateTree(state);
		TreeContext* originalContext = state.context.get();
		const auto originalVersion = state.context->storage.GetVersion();

		CommandResult result = CommandDispatcher::Dispatch(MakeCommand(CommandType::Undefined), state);

		SF_ASSERT_EQ(result.status, CommandStatus::ExecutionFailed);
		SF_ASSERT_EQ(result.message, "Undefined parsed command.");
		SF_ASSERT_EQ(state.isQuitRequested, false);
		SF_ASSERT_EQ(state.context.get() == originalContext, true);
		SF_ASSERT_EQ(state.context->storage.GetVersion(), originalVersion);
		SF_ASSERT_EQ(state.HasActiveTree(), true);
	}

	SF_TEST(command_dispatcher, QuitRequestsExit)
	{
		AppState state;
		PopulateTree(state);
		TreeContext* originalContext = state.context.get();
		const auto originalVersion = state.context->storage.GetVersion();

		CommandResult result = CommandDispatcher::Dispatch(MakeCommand(CommandType::Quit), state);

		SF_ASSERT_EQ(result.status, CommandStatus::Successful);
		SF_ASSERT_EQ(result.message, "Quitting.");
		SF_ASSERT_EQ(state.isQuitRequested, true);
		SF_ASSERT_EQ(state.context.get() == originalContext, true);
		SF_ASSERT_EQ(state.context->storage.GetVersion(), originalVersion);
		SF_ASSERT_EQ(state.HasActiveTree(), true);
	}

	SF_TEST(command_dispatcher, RepeatedQuitReturnsFailureAndKeepsExitRequested)
	{
		AppState state;
		CommandDispatcher::Dispatch(MakeCommand(CommandType::Quit), state);

		CommandResult result = CommandDispatcher::Dispatch(MakeCommand(CommandType::Quit), state);

		SF_ASSERT_EQ(result.status, CommandStatus::ExecutionFailed);
		SF_ASSERT_EQ(result.message, "Quitting is in process.");
		SF_ASSERT_EQ(state.isQuitRequested, true);
	}

	SF_TEST(command_dispatcher, UnsupportedKnownCommandsReturnFailureWithoutChangingState)
	{
		constexpr std::array unsupportedTypes{
			CommandType::Scan,
			CommandType::Save,
			CommandType::Load,
			CommandType::Info,
			CommandType::ChangeDirectory,
			CommandType::GoUp,
			CommandType::GoRoot,
			CommandType::EnterSubByIndex,
			CommandType::EnterSubByName,
			CommandType::SelectSubByIndex,
			CommandType::SelectSubFirst,
			CommandType::SelectSubNext,
			CommandType::SelectSubPrevious
		};

		for (CommandType type : unsupportedTypes) {
			AppState state;
			PopulateTree(state);
			TreeContext* originalContext = state.context.get();
			const auto originalVersion = state.context->storage.GetVersion();

			CommandResult result = CommandDispatcher::Dispatch(MakeCommand(type), state);

			SF_ASSERT_EQ(result.status, CommandStatus::ExecutionFailed);
			SF_ASSERT_EQ(result.message, "Unknown command.");
			SF_ASSERT_EQ(state.isQuitRequested, false);
			SF_ASSERT_EQ(state.context.get() == originalContext, true);
			SF_ASSERT_EQ(state.context->storage.GetVersion(), originalVersion);
			SF_ASSERT_EQ(state.HasActiveTree(), true);
		}
	}

	SF_TEST(command_dispatcher, OutOfRangeCommandTypeReturnsFailureWithoutChangingState)
	{
		AppState state;
		PopulateTree(state);
		TreeContext* originalContext = state.context.get();
		const auto originalVersion = state.context->storage.GetVersion();
		const CommandType invalidType = static_cast<CommandType>(999);

		CommandResult result = CommandDispatcher::Dispatch(MakeCommand(invalidType), state);

		SF_ASSERT_EQ(result.status, CommandStatus::ExecutionFailed);
		SF_ASSERT_EQ(result.message, "Unknown command.");
		SF_ASSERT_EQ(state.isQuitRequested, false);
		SF_ASSERT_EQ(state.context.get() == originalContext, true);
		SF_ASSERT_EQ(state.context->storage.GetVersion(), originalVersion);
		SF_ASSERT_EQ(state.HasActiveTree(), true);
	}

	SF_TEST(command_dispatcher, HelpReturnsAllRegisteredCommandDescriptionsWithoutChangingState)
	{
		AppState state;
		PopulateTree(state);
		state.isQuitRequested = true;
		TreeContext* originalContext = state.context.get();
		const auto originalVersion = state.context->storage.GetVersion();

		CommandResult result = CommandDispatcher::Dispatch(MakeCommand(CommandType::Help), state);

		SF_ASSERT_EQ(result.status, CommandStatus::Successful);
		SF_ASSERT_EQ(result.message, MakeExpectedHelpMessage());
		SF_ASSERT_EQ(state.isQuitRequested, true);
		SF_ASSERT_EQ(state.context.get() == originalContext, true);
		SF_ASSERT_EQ(state.context->storage.GetVersion(), originalVersion);
		SF_ASSERT_EQ(state.HasActiveTree(), true);
	}
}
