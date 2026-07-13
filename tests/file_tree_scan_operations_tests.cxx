#include "space_fossils/file_tree/scan/operations.hxx"

#include "space_fossils/file_tree/scan/coordinator.hxx"
#include "space_fossils/file_tree/storage/storage.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;
		using namespace space_fossils::core::file_tree::scan;

		StorageConfig MakeConfig()
		{
			return StorageConfig{
				sizeof(Node) * 4,
				sizeof(NativeChar) * 32
			};
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

		TreePoolBundle MakeBundle()
		{
			StorageConfig config = MakeConfig();

			TreePoolBundle bundle;
			bundle.namePool = std::make_unique<NamePool>(config.nameBlockSize);
			bundle.nodePool = std::make_unique<NodePool>(config.nodeBlockSize);

			return bundle;
		}

		Node* CreateBundleNode(TreePoolBundle& bundle, const NativeString& name, EntryType entryType)
		{
			Node* node = bundle.nodePool->Create();
			SF_ASSERT_EQ(node != nullptr, true);

			node->name = bundle.namePool->Store(name);
			node->entryType = entryType;
			++bundle.createdNodesCount;

			return node;
		}

		Node* AppendBundleChild(TreePoolBundle& bundle, Node* parent, const NativeString& name, EntryType entryType)
		{
			Node* child = CreateBundleNode(bundle, name, entryType);
			child->parent = parent;

			if (parent->firstChild == nullptr) {
				parent->firstChild = child;
				return child;
			}

			Node* lastChild = parent->firstChild;
			while (lastChild->nextSibling != nullptr) {
				lastChild = lastChild->nextSibling;
			}

			lastChild->nextSibling = child;
			return child;
		}

		TreePoolBundle MakeSubtree(const NativeString& rootName)
		{
			TreePoolBundle bundle = MakeBundle();
			bundle.root = CreateBundleNode(bundle, rootName, EntryType::Directory);

			return bundle;
		}

		IncomingChange MakeAdoptRootChange(TreePoolBundle&& bundle)
		{
			IncomingChange change;
			change.type = IncomingChangeType::AdoptRoot;
			change.bundle = std::move(bundle);

			return change;
		}

		Node* ApplyAdoptRoot(Storage& storage, TreePoolBundle&& bundle)
		{
			std::optional<AppliedChange> appliedChange = storage.ApplyChange(MakeAdoptRootChange(std::move(bundle)));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::AdoptRoot);

			return appliedChange->addedRoot;
		}
	}

	SF_TEST(file_tree_scan_operations, TryScheduleAttachScanRejectsFileTarget)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString fileName = MakeNativeString("file.txt");

		TreePoolBundle bundle = MakeSubtree(rootName);
		Node* file = AppendBundleChild(bundle, bundle.root, fileName, EntryType::File);
		ApplyAdoptRoot(storage, std::move(bundle));

		ScanInput input;
		input.path = std::filesystem::path(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT);
		input.maxDepth = 1;

		Coordinator coordinator(storage);
		Operations operations;

		SF_ASSERT_EQ(operations.TryScheduleAttachScan(coordinator, input, file), false);
		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);
	}

	SF_TEST(file_tree_scan_operations, TryScheduleScansRejectInvalidTargets)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");

		ApplyAdoptRoot(storage, MakeSubtree(rootName));

		ScanInput input;
		input.path = std::filesystem::path(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT);
		input.maxDepth = 1;

		Coordinator coordinator(storage);
		Operations operations;

		SF_ASSERT_EQ(operations.TryScheduleAttachScan(coordinator, input, nullptr), false);
		SF_ASSERT_EQ(operations.TryScheduleReplaceScan(coordinator, input, nullptr), false);
		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);
	}

	SF_TEST(file_tree_scan_operations, TryScheduleAttachScanAndRunScanningAddsChildToDirectory)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));

		ScanInput input;
		input.path = std::filesystem::path(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_TEST_FILE);
		input.maxDepth = 1;

		Coordinator coordinator(storage);
		Operations operations;

		SF_ASSERT_EQ(operations.TryScheduleAttachScan(coordinator, input, root), true);

		Summary summary = operations.RunScanning(coordinator);

		SF_ASSERT_EQ(root->firstChild != nullptr, true);
		SF_ASSERT_EQ(root->firstChild->parent == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 2);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 1);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 0);
	}

	SF_TEST(file_tree_scan_operations, TryScheduleReplaceScanAndRunScanningReplacesTarget)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString folderName = MakeNativeString("folder");

		TreePoolBundle bundle = MakeSubtree(rootName);
		Node* oldFolder = AppendBundleChild(bundle, bundle.root, folderName, EntryType::Directory);
		Node* root = ApplyAdoptRoot(storage, std::move(bundle));

		ScanInput input;
		input.path = std::filesystem::path(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_TEST_FILE);
		input.maxDepth = 1;

		Coordinator coordinator(storage);
		Operations operations;

		SF_ASSERT_EQ(operations.TryScheduleReplaceScan(coordinator, input, oldFolder), true);

		Summary summary = operations.RunScanning(coordinator);

		SF_ASSERT_EQ(root->firstChild != nullptr, true);
		SF_ASSERT_EQ(root->firstChild == oldFolder, false);
		SF_ASSERT_EQ(root->firstChild->parent == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 2);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 1);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 0);
	}

	SF_TEST(file_tree_scan_operations, TryScheduleRootScanAndRunScanningApplyQueuedJobs)
	{
		Storage storage;

		ScanInput input;
		input.path = std::filesystem::path(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT);
		input.maxDepth = 1;

		Coordinator coordinator(storage);
		Operations operations;

		SF_ASSERT_EQ(operations.TryScheduleRootScan(coordinator, input), true);
		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), true);

		Summary summary = operations.RunScanning(coordinator);

		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);
		SF_ASSERT_EQ(storage.GetRoot() != nullptr, true);
		SF_ASSERT_EQ(summary.storedNodesCount, 8);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 4);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 0);
	}
}
