#include "space_fossils/cli/command_dispatcher.hxx"

#include "space_fossils/cli/command_spec.hxx"
#include "space_fossils/cli/command_type.hxx"
#include "space_fossils/core/file_tree/model/tree_pool_bundle.hxx"
#include "space_fossils/core/version.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::cli;
		using namespace space_fossils::core;
		using namespace space_fossils::core::file_tree;

		ParsedCommand MakeCommand(
			CommandType type,
			std::initializer_list<std::string_view> arguments = {})
		{
			ParsedCommand command;
			command.type = type;
			for (std::string_view argument : arguments) {
				command.arguments.emplace_back(argument);
			}
			return command;
		}

		NativeString MakeNativeString(const char* value)
		{
			NativeString result;
			while (*value != '\0') {
				result.push_back(static_cast<NativeChar>(*value));
				++value;
			}
			return result;
		}

		std::string PathToUtf8(const std::filesystem::path& path)
		{
			const std::u8string utf8 = path.u8string();
			return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
		}

		Node* CreateNode(
			TreePoolBundle& bundle,
			const char* name,
			EntryType type,
			FileSize logicalSize)
		{
			Node* node = bundle.nodePool->Create();
			SF_ASSERT_EQ(node != nullptr, true);
			node->name = bundle.namePool->Store(MakeNativeString(name));
			node->entryType = type;
			node->entryStatus = EntryStatus::Accessible;
			node->scanStatus = EntryScanStatus::Complete;
			node->logicalSize = logicalSize;
			++bundle.createdNodesCount;
			return node;
		}

		void PopulateTree(AppState& state)
		{
			TreePoolBundle bundle;
			bundle.namePool = std::make_unique<NamePool>(DefaultNameBlockSize);
			bundle.nodePool = std::make_unique<NodePool>(DefaultNodeBlockSize);

			Node* root = CreateNode(bundle, "root", EntryType::Directory, 600);
			Node* folder = CreateNode(bundle, "folder", EntryType::Directory, 300);
			Node* small = CreateNode(bundle, "small.txt", EntryType::File, 100);
			Node* large = CreateNode(bundle, "large.txt", EntryType::File, 200);
			Node* nested = CreateNode(bundle, "nested.txt", EntryType::File, 300);

			root->firstChild = folder;
			folder->parent = root;
			folder->nextSibling = small;
			small->parent = root;
			small->nextSibling = large;
			large->parent = root;
			folder->firstChild = nested;
			nested->parent = folder;
			bundle.root = root;

			std::optional<AppliedChange> change = state.context->storage.TryAdoptRoot(std::move(bundle));
			SF_ASSERT_EQ(change.has_value(), true);
		}

		std::string MakeExpectedHelpMessage()
		{
			std::size_t labelWidth = 0;
			std::vector<std::string> labels;
			for (const CommandSpec& spec : CommandSpecs) {
				std::string label(spec.usage);
				if (!spec.shortName.empty()) {
					label += " (";
					label += spec.shortName;
					label += ')';
				}
				labelWidth = std::max(labelWidth, label.size());
				labels.push_back(std::move(label));
			}

			std::ostringstream message;
			for (std::size_t index = 0; index < CommandSpecs.size(); ++index) {
				message << labels[index]
					<< std::string(labelWidth - labels[index].size(), ' ')
					<< "  -  " << CommandSpecs[index].description << '\n';
			}
			return message.str();
		}

		std::filesystem::path PrepareSnapshotPath(const char* filename)
		{
			const std::filesystem::path directory =
				std::filesystem::current_path() / "space_fossils_command_dispatcher_tests";
			std::error_code error;
			std::filesystem::create_directories(directory, error);
			SF_ASSERT_EQ(bool(error), false);

			const std::filesystem::path path = directory / filename;
			std::filesystem::remove(path, error);
			SF_ASSERT_EQ(bool(error), false);
			return path;
		}

		void RemoveSnapshotPath(const std::filesystem::path& path)
		{
			std::error_code error;
			std::filesystem::remove(path, error);
			SF_ASSERT_EQ(bool(error), false);
			std::filesystem::remove(path.parent_path(), error);
			SF_ASSERT_EQ(bool(error), false);
		}

		void AssertMetadataEquals(const TreeMetadata& actual, const TreeMetadata& expected)
		{
			SF_ASSERT_EQ(actual.scanSourcePath == expected.scanSourcePath, true);
			SF_ASSERT_EQ(actual.treeSource, expected.treeSource);
			SF_ASSERT_EQ(actual.applicationVersion, expected.applicationVersion);
			SF_ASSERT_EQ(actual.updatedAtUnixSeconds, expected.updatedAtUnixSeconds);
		}

		void AssertDefaultMetadata(const TreeMetadata& metadata)
		{
			AssertMetadataEquals(metadata, {});
		}
	}

	SF_TEST(command_dispatcher, UndefinedAndOutOfRangeCommandsReturnFailureWithoutChangingState)
	{
		for (CommandType type : { CommandType::Undefined, static_cast<CommandType>(999) }) {
			AppState state;
			PopulateTree(state);
			TreeContext* originalContext = state.context.get();
			const StorageVersion originalVersion = state.context->storage.GetVersion();

			CommandResult result = CommandDispatcher::Dispatch(MakeCommand(type), state);

			SF_ASSERT_EQ(result.status, CommandStatus::ExecutionFailed);
			SF_ASSERT_EQ(result.message, "Undefined parsed command");
			SF_ASSERT_EQ(state.context.get() == originalContext, true);
			SF_ASSERT_EQ(state.context->storage.GetVersion(), originalVersion);
		}
	}

	SF_TEST(command_dispatcher, HelpReturnsAllRegisteredCommandsWithoutChangingState)
	{
		AppState state;
		PopulateTree(state);
		TreeContext* originalContext = state.context.get();

		CommandResult result = CommandDispatcher::Dispatch(MakeCommand(CommandType::Help), state);

		SF_ASSERT_EQ(result.status, CommandStatus::Successful);
		SF_ASSERT_EQ(result.message, MakeExpectedHelpMessage());
		SF_ASSERT_EQ(state.context.get() == originalContext, true);
	}

	SF_TEST(command_dispatcher, QuitRequestsExitAndRepeatedQuitFails)
	{
		AppState state;

		CommandResult first = CommandDispatcher::Dispatch(MakeCommand(CommandType::Quit), state);
		CommandResult second = CommandDispatcher::Dispatch(MakeCommand(CommandType::Quit), state);

		SF_ASSERT_EQ(first.status, CommandStatus::Successful);
		SF_ASSERT_EQ(first.message, "Quitting");
		SF_ASSERT_EQ(second.status, CommandStatus::ExecutionFailed);
		SF_ASSERT_EQ(state.isQuitRequested, true);
	}

	SF_TEST(command_dispatcher, ValidatesArgumentCountBeforeState)
	{
		AppState state;

		CommandResult missing = CommandDispatcher::Dispatch(MakeCommand(CommandType::Scan), state);
		CommandResult extra = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::Help, { "extra" }), state);

		SF_ASSERT_EQ(missing.status, CommandStatus::InvalidArgs);
		SF_ASSERT_EQ(extra.status, CommandStatus::InvalidArgs);
	}

	SF_TEST(command_dispatcher, SetUnitsAcceptsKnownValuesAndRejectsUnknownValue)
	{
		AppState state;

		CommandResult binary = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::SetUnits, { "binary" }), state);
		SF_ASSERT_EQ(binary.status, CommandStatus::Successful);
		SF_ASSERT_EQ(state.unitsType, FileSizeUnitSystem::Binary);

		CommandResult invalid = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::SetUnits, { "bits" }), state);
		SF_ASSERT_EQ(invalid.status, CommandStatus::InvalidArgs);
		SF_ASSERT_EQ(state.unitsType, FileSizeUnitSystem::Binary);

		CommandResult decimal = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::SetUnits, { "decimal" }), state);
		SF_ASSERT_EQ(decimal.status, CommandStatus::Successful);
		SF_ASSERT_EQ(state.unitsType, FileSizeUnitSystem::Decimal);
	}

	SF_TEST(command_dispatcher, TreeCommandsRequireActiveTree)
	{
		constexpr CommandType treeCommands[] = {
			CommandType::Rescan,
			CommandType::SaveSnapshot,
			CommandType::ShowTree,
			CommandType::ListChildren,
			CommandType::ShowInfo,
			CommandType::ShowTop,
			CommandType::ChangeDirectory,
			CommandType::PrintWorkingDirectory
		};

		for (CommandType type : treeCommands) {
			AppState state;
			ParsedCommand command = type == CommandType::SaveSnapshot
				|| type == CommandType::ChangeDirectory
				? MakeCommand(type, { "path" })
				: MakeCommand(type);

			CommandResult result = CommandDispatcher::Dispatch(command, state);
			SF_ASSERT_EQ(result.status, CommandStatus::InvalidState);
		}
	}

	SF_TEST(command_dispatcher, ResetWorksForFreshAndPopulatedState)
	{
		AppState state;
		TreeContext* freshContext = state.context.get();

		CommandResult freshReset = CommandDispatcher::Dispatch(MakeCommand(CommandType::Reset), state);
		SF_ASSERT_EQ(freshReset.status, CommandStatus::Successful);
		SF_ASSERT_EQ(state.context.get() == freshContext, false);
		SF_ASSERT_EQ(state.IsFreshStorage(), true);

		PopulateTree(state);
		CommandResult populatedReset = CommandDispatcher::Dispatch(MakeCommand(CommandType::Reset), state);
		SF_ASSERT_EQ(populatedReset.status, CommandStatus::Successful);
		SF_ASSERT_EQ(state.HasActiveTree(), false);
		SF_ASSERT_EQ(state.IsFreshStorage(), true);
	}

	SF_TEST(command_dispatcher, TreeReportsAndNavigationUseCurrentDirectory)
	{
		AppState state;
		PopulateTree(state);

		CommandResult tree = CommandDispatcher::Dispatch(MakeCommand(CommandType::ShowTree), state);
		CommandResult children = CommandDispatcher::Dispatch(MakeCommand(CommandType::ListChildren), state);
		CommandResult top = CommandDispatcher::Dispatch(MakeCommand(CommandType::ShowTop), state);

		SF_ASSERT_EQ(tree.status, CommandStatus::Successful);
		SF_ASSERT_EQ(tree.message.find("nested.txt") != std::string::npos, true);
		SF_ASSERT_EQ(
			children.message,
			"folder     0.3 KB  directory\n"
			"small.txt  0.1 KB  file\n"
			"large.txt  0.2 KB  file\n");
		const std::size_t folderPosition = top.message.find("folder");
		const std::size_t largePosition = top.message.find("large.txt");
		const std::size_t smallPosition = top.message.find("small.txt");
		SF_ASSERT_EQ(folderPosition < largePosition, true);
		SF_ASSERT_EQ(largePosition < smallPosition, true);

		CommandResult cd = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::ChangeDirectory, { "folder" }), state);
		CommandResult pwd = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::PrintWorkingDirectory), state);
		CommandResult info = CommandDispatcher::Dispatch(MakeCommand(CommandType::ShowInfo), state);
		SF_ASSERT_EQ(cd.status, CommandStatus::Successful);
		SF_ASSERT_EQ(pwd.message, cd.message);
		SF_ASSERT_EQ(info.message.find("Children   : 1") != std::string::npos, true);

		CommandResult fileCd = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::ChangeDirectory, { "nested.txt" }), state);
		SF_ASSERT_EQ(fileCd.status, CommandStatus::ExecutionFailed);

		CommandResult rootCd = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::ChangeDirectory, { "/" }), state);
		SF_ASSERT_EQ(rootCd.status, CommandStatus::Successful);
		SF_ASSERT_EQ(rootCd.message, "root");
	}

	SF_TEST(command_dispatcher, ScanAndRescanUseOriginalSourcePath)
	{
		AppState state;

		CommandResult scan = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::Scan, { SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT }), state);
		SF_ASSERT_EQ(scan.status, CommandStatus::Successful);
		SF_ASSERT_EQ(state.HasActiveTree(), true);

		std::error_code pathError;
		const std::filesystem::path expectedSourcePath = std::filesystem::absolute(
			SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT,
			pathError).lexically_normal();
		SF_ASSERT_EQ(bool(pathError), false);
		SF_ASSERT_EQ(state.context->treeMetadata.scanSourcePath == expectedSourcePath, true);
		SF_ASSERT_EQ(state.context->treeMetadata.treeSource, TreeSource::Scan);
		SF_ASSERT_EQ(state.context->treeMetadata.applicationVersion.empty(), true);
		SF_ASSERT_EQ(state.context->treeMetadata.updatedAtUnixSeconds > 0, true);

		CommandResult repeatedScan = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::Scan, { SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT }), state);
		SF_ASSERT_EQ(repeatedScan.status, CommandStatus::InvalidState);

		state.context->treeMetadata.updatedAtUnixSeconds = 0;
		CommandResult rescan = CommandDispatcher::Dispatch(MakeCommand(CommandType::Rescan), state);
		SF_ASSERT_EQ(rescan.status, CommandStatus::Successful);
		SF_ASSERT_EQ(state.HasActiveTree(), true);
		SF_ASSERT_EQ(state.context->treeMetadata.scanSourcePath == expectedSourcePath, true);
		SF_ASSERT_EQ(state.context->treeMetadata.treeSource, TreeSource::Scan);
		SF_ASSERT_EQ(state.context->treeMetadata.updatedAtUnixSeconds > 0, true);
	}

	SF_TEST(command_dispatcher, RescanWithoutSourcePathLeavesTreeAndMetadataUnchanged)
	{
		AppState state;
		PopulateTree(state);
		TreeMetadata expectedMetadata;
		expectedMetadata.treeSource = TreeSource::Snapshot;
		expectedMetadata.applicationVersion = "snapshot-version";
		expectedMetadata.updatedAtUnixSeconds = 123456;
		state.context->treeMetadata = expectedMetadata;
		const Node* originalRoot = state.context->storage.GetRoot();
		const StorageVersion originalVersion = state.context->storage.GetVersion();

		CommandResult rescan = CommandDispatcher::Dispatch(MakeCommand(CommandType::Rescan), state);

		SF_ASSERT_EQ(rescan.status, CommandStatus::ExecutionFailed);
		SF_ASSERT_EQ(rescan.message, "Scan source path is unavailable");
		SF_ASSERT_EQ(state.context->storage.GetRoot() == originalRoot, true);
		SF_ASSERT_EQ(state.context->storage.GetVersion(), originalVersion);
		AssertMetadataEquals(state.context->treeMetadata, expectedMetadata);
	}

	SF_TEST(command_dispatcher, InvalidScanPathLeavesStorageFresh)
	{
		AppState state;

		CommandResult result = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::Scan, { SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH }), state);

		SF_ASSERT_EQ(result.status, CommandStatus::ExecutionFailed);
		SF_ASSERT_EQ(state.HasActiveTree(), false);
		SF_ASSERT_EQ(state.IsFreshStorage(), true);
		AssertDefaultMetadata(state.context->treeMetadata);
	}

	SF_TEST(command_dispatcher, SnapshotRoundTripReplacesContextOnlyAfterSuccessfulLoad)
	{
		const std::filesystem::path snapshotPath = PrepareSnapshotPath("round-trip.sfvb");
		const std::string pathArgument = PathToUtf8(snapshotPath);
		AppState source;
		PopulateTree(source);
		TreeMetadata sourceMetadata;
		sourceMetadata.scanSourcePath = std::filesystem::path("source") / "root";
		sourceMetadata.treeSource = TreeSource::Scan;
		sourceMetadata.applicationVersion = "ignored-writer-version";
		sourceMetadata.updatedAtUnixSeconds = 123456;
		source.context->treeMetadata = sourceMetadata;

		CommandResult save = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::SaveSnapshot, { pathArgument }), source);
		SF_ASSERT_EQ(save.status, CommandStatus::Successful);

		AppState destination;
		TreeContext* freshContext = destination.context.get();
		CommandResult load = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::LoadSnapshot, { pathArgument }), destination);
		SF_ASSERT_EQ(load.status, CommandStatus::Successful);
		SF_ASSERT_EQ(destination.context.get() == freshContext, false);
		SF_ASSERT_EQ(destination.context->storage.GetNodesCount(), 5);
		SF_ASSERT_EQ(destination.context->treeMetadata.scanSourcePath == sourceMetadata.scanSourcePath, true);
		SF_ASSERT_EQ(destination.context->treeMetadata.treeSource, TreeSource::Snapshot);
		SF_ASSERT_EQ(destination.context->treeMetadata.applicationVersion, std::string(version::ApplicationVersion));
		SF_ASSERT_EQ(destination.context->treeMetadata.updatedAtUnixSeconds, sourceMetadata.updatedAtUnixSeconds);
		SF_ASSERT_EQ(
			&destination.context->session.GetStorage() == &destination.context->storage,
			true);

		TreeContext* loadedContext = destination.context.get();
		const StorageVersion loadedVersion = destination.context->storage.GetVersion();
		TreeMetadata preservedMetadata;
		preservedMetadata.scanSourcePath = std::filesystem::path("preserved") / "source";
		preservedMetadata.treeSource = TreeSource::Scan;
		preservedMetadata.applicationVersion = "preserved-version";
		preservedMetadata.updatedAtUnixSeconds = 987654;
		destination.context->treeMetadata = preservedMetadata;
		CommandResult failedLoad = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::LoadSnapshot, { "missing.sfvb" }), destination);
		SF_ASSERT_EQ(failedLoad.status, CommandStatus::ExecutionFailed);
		SF_ASSERT_EQ(destination.context.get() == loadedContext, true);
		SF_ASSERT_EQ(destination.context->storage.GetVersion(), loadedVersion);
		AssertMetadataEquals(destination.context->treeMetadata, preservedMetadata);

		RemoveSnapshotPath(snapshotPath);
	}

	SF_TEST(command_dispatcher, LoadedScannedSnapshotRetainsSourcePathForRescan)
	{
		const std::filesystem::path snapshotPath = PrepareSnapshotPath("loaded-rescan.sfvb");
		const std::string snapshotPathArgument = PathToUtf8(snapshotPath);
		AppState source;

		CommandResult scan = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::Scan, { SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT }), source);
		SF_ASSERT_EQ(scan.status, CommandStatus::Successful);
		CommandResult save = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::SaveSnapshot, { snapshotPathArgument }), source);
		SF_ASSERT_EQ(save.status, CommandStatus::Successful);

		AppState loaded;
		CommandResult load = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::LoadSnapshot, { snapshotPathArgument }), loaded);
		SF_ASSERT_EQ(load.status, CommandStatus::Successful);
		SF_ASSERT_EQ(loaded.context->treeMetadata.treeSource, TreeSource::Snapshot);
		SF_ASSERT_EQ(loaded.context->treeMetadata.scanSourcePath, source.context->treeMetadata.scanSourcePath);

		CommandResult changeDirectory = CommandDispatcher::Dispatch(
			MakeCommand(CommandType::ChangeDirectory, { "sub_directory_1" }), loaded);
		SF_ASSERT_EQ(changeDirectory.status, CommandStatus::Successful);
		loaded.context->treeMetadata.updatedAtUnixSeconds = 0;
		CommandResult rescan = CommandDispatcher::Dispatch(MakeCommand(CommandType::Rescan), loaded);

		SF_ASSERT_EQ(rescan.status, CommandStatus::Successful);
		SF_ASSERT_EQ(loaded.context->treeMetadata.treeSource, TreeSource::Snapshot);
		SF_ASSERT_EQ(loaded.context->treeMetadata.scanSourcePath, source.context->treeMetadata.scanSourcePath);
		SF_ASSERT_EQ(loaded.context->treeMetadata.updatedAtUnixSeconds > 0, true);

		RemoveSnapshotPath(snapshotPath);
	}
}
