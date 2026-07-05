#include "space_fossils/file_tree/scan_coordinator.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;

		NativeString MakeNativeString(const char* value)
		{
			NativeString result;
			while (*value != '\0') {
				result.push_back(static_cast<NativeChar>(*value));
				++value;
			}

			return result;
		}

		bool NameEquals(const Node& node, const char* expectedValue)
		{
			NativeString expectedName = MakeNativeString(expectedValue);
			NativeStringView actualName = ToStringView(node.name);

			if (actualName.size() != expectedName.size()) {
				return false;
			}

			for (std::size_t index = 0; index < expectedName.size(); ++index) {
				if (actualName[index] != expectedName[index]) {
					return false;
				}
			}

			return true;
		}

		void AssertNameEquals(const Node& node, const char* expectedValue)
		{
			SF_ASSERT_EQ(NameEquals(node, expectedValue), true);
		}

		Node* FindChild(Node& parent, const char* name)
		{
			Node* child = parent.firstChild;
			while (child != nullptr) {
				if (NameEquals(*child, name)) {
					return child;
				}

				child = child->nextSibling;
			}

			return nullptr;
		}

		Node* RequireChild(Node& parent, const char* name)
		{
			Node* child = FindChild(parent, name);
			SF_ASSERT_EQ(child != nullptr, true);
			SF_ASSERT_EQ(child->parent == &parent, true);

			return child;
		}

		std::size_t CountChildren(const Node& parent)
		{
			std::size_t count = 0;
			const Node* child = parent.firstChild;
			while (child != nullptr) {
				++count;
				child = child->nextSibling;
			}

			return count;
		}

		std::size_t CountPendingDirectories(const Node* node)
		{
			if (node == nullptr) {
				return 0;
			}

			std::size_t count = 0;
			if (node->entryType == EntryType::Directory && node->scanStatus == EntryScanStatus::Pending) {
				++count;
			}

			for (const Node* child = node->firstChild; child != nullptr; child = child->nextSibling) {
				count += CountPendingDirectories(child);
			}

			return count;
		}

		ScanCoordinator MakeCoordinatorWithRoot(Storage& storage, const char* rootPath, std::size_t defaultScanDepth = 1)
		{
			ScanCoordinatorConfig config;
			config.rootPath = std::filesystem::path(rootPath);
			config.defaultScanDepth = defaultScanDepth;

			return ScanCoordinator(storage, std::move(config));
		}

		ScanCoordinator MakeCoordinator(Storage& storage, std::size_t defaultScanDepth = 1)
		{
			return MakeCoordinatorWithRoot(storage, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, defaultScanDepth);
		}
	}

	SF_TEST(file_tree_scan_coordinator, ProcessNextReturnsEmptyWhenNoJobs)
	{
		Storage storage;
		ScanCoordinator coordinator = MakeCoordinator(storage);

		std::optional<AppliedChange> appliedChange = coordinator.ProcessNext();

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_scan_coordinator, ScheduleRootScanAdoptsRoot)
	{
		Storage storage;
		ScanCoordinator coordinator = MakeCoordinator(storage);

		coordinator.ScheduleRootScan(1);
		std::optional<AppliedChange> appliedChange = coordinator.ProcessNext();

		SF_ASSERT_EQ(appliedChange.has_value(), true);
		SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::AdoptRoot);
		SF_ASSERT_EQ(appliedChange->target == nullptr, true);
		SF_ASSERT_EQ(appliedChange->addedRoot != nullptr, true);
		SF_ASSERT_EQ(appliedChange->addedNodesCount, 4);
		SF_ASSERT_EQ(appliedChange->removedNodesCount, 0);
		SF_ASSERT_EQ(storage.GetRoot() == appliedChange->addedRoot, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 4);

		Node& root = *storage.GetRoot();
		AssertNameEquals(root, "root");
		SF_ASSERT_EQ(root.entryType, EntryType::Directory);
		SF_ASSERT_EQ(root.entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(root.scanStatus, EntryScanStatus::Partial);
		SF_ASSERT_EQ(CountChildren(root), 3);

		Node* firstDirectory = RequireChild(root, "sub_directory_1");
		Node* secondDirectory = RequireChild(root, "sub_directory_2");
		SF_ASSERT_EQ(firstDirectory->scanStatus, EntryScanStatus::Pending);
		SF_ASSERT_EQ(secondDirectory->scanStatus, EntryScanStatus::Pending);
	}

	SF_TEST(file_tree_scan_coordinator, PendingRootSchedulesRootReplacement)
	{
		Storage storage;
		ScanCoordinator coordinator = MakeCoordinator(storage);

		coordinator.ScheduleRootScan(0);
		std::optional<AppliedChange> rootChange = coordinator.ProcessNext();
		SF_ASSERT_EQ(rootChange.has_value(), true);
		SF_ASSERT_EQ(rootChange->type, IncomingChangeType::AdoptRoot);
		SF_ASSERT_EQ(rootChange->addedNodesCount, 1);
		SF_ASSERT_EQ(rootChange->removedNodesCount, 0);

		Node* oldRoot = rootChange->addedRoot;
		SF_ASSERT_EQ(oldRoot != nullptr, true);
		SF_ASSERT_EQ(storage.GetRoot() == oldRoot, true);
		SF_ASSERT_EQ(oldRoot->scanStatus, EntryScanStatus::Pending);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);

		std::optional<AppliedChange> replacementChange = coordinator.ProcessNext();

		SF_ASSERT_EQ(replacementChange.has_value(), true);
		SF_ASSERT_EQ(replacementChange->type, IncomingChangeType::Replace);
		SF_ASSERT_EQ(replacementChange->target == oldRoot, true);
		SF_ASSERT_EQ(replacementChange->addedRoot != nullptr, true);
		SF_ASSERT_EQ(replacementChange->addedRoot == oldRoot, false);
		SF_ASSERT_EQ(replacementChange->addedNodesCount, 4);
		SF_ASSERT_EQ(replacementChange->removedNodesCount, 1);
		SF_ASSERT_EQ(storage.GetRoot() == replacementChange->addedRoot, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 4);
	}

	SF_TEST(file_tree_scan_coordinator, ProcessNextAfterRootScanProcessesScheduledPendingDirectory)
	{
		Storage storage;
		ScanCoordinator coordinator = MakeCoordinator(storage);

		coordinator.ScheduleRootScan(1);
		SF_ASSERT_EQ(coordinator.ProcessNext().has_value(), true);

		std::optional<AppliedChange> appliedChange = coordinator.ProcessNext();

		SF_ASSERT_EQ(appliedChange.has_value(), true);
		SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::Replace);
		SF_ASSERT_EQ(appliedChange->target != nullptr, true);
		SF_ASSERT_EQ(appliedChange->addedRoot != nullptr, true);
		SF_ASSERT_EQ(appliedChange->addedNodesCount >= 1, true);
		SF_ASSERT_EQ(appliedChange->removedNodesCount, 1);
		SF_ASSERT_EQ(storage.GetNodesCount() > 4, true);
		SF_ASSERT_EQ(CountPendingDirectories(storage.GetRoot()) >= 1, true);
	}

	SF_TEST(file_tree_scan_coordinator, ProcessNextSkipsStaleRejectedJobAndContinues)
	{
		Storage storage;
		ScanCoordinator coordinator = MakeCoordinator(storage);

		coordinator.ScheduleRootScan(1);
		std::optional<AppliedChange> rootChange = coordinator.ProcessNext();
		SF_ASSERT_EQ(rootChange.has_value(), true);

		Node& root = *storage.GetRoot();
		Node* firstDirectory = RequireChild(root, "sub_directory_1");
		Node* secondDirectory = RequireChild(root, "sub_directory_2");

		IncomingChange removeChange;
		removeChange.type = IncomingChangeType::Remove;
		removeChange.target = firstDirectory;
		std::optional<AppliedChange> removeResult = storage.ApplyChange(std::move(removeChange));
		SF_ASSERT_EQ(removeResult.has_value(), true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 3);

		std::optional<AppliedChange> appliedChange = coordinator.ProcessNext();

		SF_ASSERT_EQ(appliedChange.has_value(), true);
		SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::Replace);
		SF_ASSERT_EQ(appliedChange->target == secondDirectory, true);
		SF_ASSERT_EQ(appliedChange->addedRoot != nullptr, true);
		SF_ASSERT_EQ(appliedChange->removedNodesCount, 1);
		SF_ASSERT_EQ(FindChild(*storage.GetRoot(), "sub_directory_1") == nullptr, true);
		SF_ASSERT_EQ(FindChild(*storage.GetRoot(), "sub_directory_2") != nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 4);
	}

	SF_TEST(file_tree_scan_coordinator, ProcessesPendingDirectoriesUntilQueueIsEmpty)
	{
		Storage storage;
		ScanCoordinator coordinator = MakeCoordinator(storage);

		coordinator.ScheduleRootScan(1);
		std::optional<AppliedChange> rootChange = coordinator.ProcessNext();
		SF_ASSERT_EQ(rootChange.has_value(), true);

		std::size_t processedFollowUpJobs = 0;
		while (coordinator.ProcessNext().has_value()) {
			++processedFollowUpJobs;
		}

		SF_ASSERT_EQ(processedFollowUpJobs, 3);
		SF_ASSERT_EQ(storage.GetNodesCount(), 8);
		SF_ASSERT_EQ(CountPendingDirectories(storage.GetRoot()), 0);

		Node& root = *storage.GetRoot();
		Node* firstDirectory = RequireChild(root, "sub_directory_1");
		Node* secondDirectory = RequireChild(root, "sub_directory_2");
		Node* nestedDirectory = RequireChild(*secondDirectory, "sub_directory_3");

		SF_ASSERT_EQ(firstDirectory->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(secondDirectory->scanStatus, EntryScanStatus::Partial);
		SF_ASSERT_EQ(nestedDirectory->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(CountChildren(*firstDirectory), 2);
		SF_ASSERT_EQ(CountChildren(*secondDirectory), 1);
		SF_ASSERT_EQ(CountChildren(*nestedDirectory), 1);
	}

	SF_TEST(file_tree_scan_coordinator, DeepDefaultScanDepthCompletesFollowUpSubtreesInFewerJobs)
	{
		Storage storage;
		ScanCoordinator coordinator = MakeCoordinator(storage, 3);

		coordinator.ScheduleRootScan(1);
		SF_ASSERT_EQ(coordinator.ProcessNext().has_value(), true);

		std::size_t processedFollowUpJobs = 0;
		while (coordinator.ProcessNext().has_value()) {
			++processedFollowUpJobs;
		}

		SF_ASSERT_EQ(processedFollowUpJobs, 2);
		SF_ASSERT_EQ(storage.GetNodesCount(), 8);
		SF_ASSERT_EQ(CountPendingDirectories(storage.GetRoot()), 0);
	}

	SF_TEST(file_tree_scan_coordinator, MissingRootPathDoesNotScheduleFollowUpJobs)
	{
		Storage storage;
		ScanCoordinator coordinator = MakeCoordinatorWithRoot(storage, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH);

		coordinator.ScheduleRootScan(1);
		std::optional<AppliedChange> appliedChange = coordinator.ProcessNext();

		SF_ASSERT_EQ(appliedChange.has_value(), true);
		SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::AdoptRoot);
		SF_ASSERT_EQ(appliedChange->addedNodesCount, 1);
		SF_ASSERT_EQ(appliedChange->removedNodesCount, 0);
		SF_ASSERT_EQ(storage.GetRoot() == appliedChange->addedRoot, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);

		Node& root = *storage.GetRoot();
		AssertNameEquals(root, "root");
		SF_ASSERT_EQ(root.entryType, EntryType::Unknown);
		SF_ASSERT_EQ(root.entryStatus, EntryStatus::NotFound);
		SF_ASSERT_EQ(root.scanStatus, EntryScanStatus::Error);
		SF_ASSERT_EQ(coordinator.ProcessNext().has_value(), false);
	}
}
