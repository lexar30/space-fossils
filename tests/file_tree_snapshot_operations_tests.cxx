#include "space_fossils/core/file_tree/snapshot/operations.hxx"

#include "space_fossils/core/file_tree/model/tree_pool_bundle.hxx"
#include "space_fossils/core/file_tree/storage/storage.hxx"
#include "space_fossils/core/version.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;
		using namespace space_fossils::core::file_tree::snapshot;

		NativeString MakeNativeString(const char* value)
		{
			NativeString result;
			while (*value != '\0') {
				result.push_back(static_cast<NativeChar>(*value));
				++value;
			}

			return result;
		}

		void AssertNameEquals(const Node& node, const char* expectedValue)
		{
			NativeString expectedName = MakeNativeString(expectedValue);
			NativeStringView actualName = ToStringView(node.name);

			SF_ASSERT_EQ(actualName.size(), expectedName.size());
			for (std::size_t index = 0; index < expectedName.size(); ++index) {
				SF_ASSERT_EQ(actualName[index] == expectedName[index], true);
			}
		}

		TreePoolBundle MakeTestTree(const char* rootName, const char* fileName, FileSize logicalSize)
		{
			TreePoolBundle bundle;
			bundle.namePool = std::make_unique<NamePool>(DefaultNameBlockSize);
			bundle.nodePool = std::make_unique<NodePool>(DefaultNodeBlockSize);

			Node* root = bundle.nodePool->Create();
			SF_ASSERT_EQ(root != nullptr, true);
			root->name = bundle.namePool->Store(MakeNativeString(rootName));
			root->logicalSize = logicalSize;
			root->entryType = EntryType::Directory;
			root->entryStatus = EntryStatus::Accessible;
			root->scanStatus = EntryScanStatus::Complete;
			++bundle.createdNodesCount;

			Node* file = bundle.nodePool->Create();
			SF_ASSERT_EQ(file != nullptr, true);
			file->name = bundle.namePool->Store(MakeNativeString(fileName));
			file->logicalSize = logicalSize;
			file->entryType = EntryType::File;
			file->entryStatus = EntryStatus::Accessible;
			file->scanStatus = EntryScanStatus::Complete;
			file->parent = root;
			++bundle.createdNodesCount;

			root->firstChild = file;
			bundle.root = root;

			return bundle;
		}

		Node* ApplyTestTree(Storage& storage, const char* rootName, const char* fileName, FileSize logicalSize)
		{
			std::optional<AppliedChange> change = storage.TryAdoptRoot(MakeTestTree(rootName, fileName, logicalSize));
			SF_ASSERT_EQ(change.has_value(), true);
			SF_ASSERT_EQ(change->addedNodesCount, 2);

			return change->addedRoot;
		}

		void AssertTestTree(const Storage& storage, const char* rootName, const char* fileName, FileSize logicalSize)
		{
			const Node* root = storage.GetRoot();
			SF_ASSERT_EQ(root != nullptr, true);
			AssertNameEquals(*root, rootName);
			SF_ASSERT_EQ(root->logicalSize, logicalSize);
			SF_ASSERT_EQ(root->entryType, EntryType::Directory);
			SF_ASSERT_EQ(root->entryStatus, EntryStatus::Accessible);
			SF_ASSERT_EQ(root->scanStatus, EntryScanStatus::Complete);
			SF_ASSERT_EQ(root->parent == nullptr, true);
			SF_ASSERT_EQ(root->nextSibling == nullptr, true);

			const Node* file = root->firstChild;
			SF_ASSERT_EQ(file != nullptr, true);
			AssertNameEquals(*file, fileName);
			SF_ASSERT_EQ(file->logicalSize, logicalSize);
			SF_ASSERT_EQ(file->entryType, EntryType::File);
			SF_ASSERT_EQ(file->entryStatus, EntryStatus::Accessible);
			SF_ASSERT_EQ(file->scanStatus, EntryScanStatus::Complete);
			SF_ASSERT_EQ(file->parent == root, true);
			SF_ASSERT_EQ(file->firstChild == nullptr, true);
			SF_ASSERT_EQ(file->nextSibling == nullptr, true);
			SF_ASSERT_EQ(storage.GetNodesCount(), 2);
		}

		std::filesystem::path PrepareTestPath(const char* filename)
		{
			const std::filesystem::path directory = std::filesystem::current_path() / "space_fossils_snapshot_operations_tests";
			std::error_code ec;
			std::filesystem::create_directories(directory, ec);
			SF_ASSERT_EQ(bool(ec), false);

			const std::filesystem::path path = directory / filename;
			std::filesystem::remove(path, ec);
			SF_ASSERT_EQ(bool(ec), false);

			return path;
		}

		void RemoveTestPath(const std::filesystem::path& path)
		{
			std::error_code ec;
			std::filesystem::remove(path, ec);
			SF_ASSERT_EQ(bool(ec), false);

			std::filesystem::remove(path.parent_path(), ec);
			SF_ASSERT_EQ(bool(ec), false);
		}

		void AssertDefaultMetadata(const TreeMetadata& metadata)
		{
			SF_ASSERT_EQ(metadata.scanSourcePath.empty(), true);
			SF_ASSERT_EQ(metadata.treeSource, TreeSource::Unknown);
			SF_ASSERT_EQ(metadata.applicationVersion.empty(), true);
			SF_ASSERT_EQ(metadata.updatedAtUnixSeconds, 0);
		}
	}

	SF_TEST(file_tree_snapshot_operations, SavesAndLoadsSnapshotRoundTrip)
	{
		const std::filesystem::path snapshotPath = PrepareTestPath("round-trip.sfvb");
		Storage source;
		ApplyTestTree(source, "root", "file.txt", 42);
		TreeMetadata metadata;
		metadata.scanSourcePath = std::filesystem::path("source") / "root";
		metadata.treeSource = TreeSource::Scan;
		metadata.applicationVersion = "ignored-writer-version";
		metadata.updatedAtUnixSeconds = 123456;
		Operations operations;

		SavedSnapshotSummary saved = operations.TrySaveSnapshot(snapshotPath, source, metadata);

		SF_ASSERT_EQ(saved.isSuccessful, true);
		SF_ASSERT_EQ(saved.saveDuration.count() >= 0, true);

		Storage loaded;
		LoadedSnapshotSummary load = operations.TryLoadSnapshot(snapshotPath, loaded);

		SF_ASSERT_EQ(load.isSuccessful, true);
		SF_ASSERT_EQ(load.loadDuration.count() >= 0, true);
		SF_ASSERT_EQ(loaded.GetVersion(), 1);
		SF_ASSERT_EQ(load.treeMetadata.scanSourcePath == metadata.scanSourcePath, true);
		SF_ASSERT_EQ(load.treeMetadata.treeSource, TreeSource::Snapshot);
		SF_ASSERT_EQ(
			load.treeMetadata.applicationVersion,
			std::string(space_fossils::core::version::ApplicationVersion));
		SF_ASSERT_EQ(load.treeMetadata.updatedAtUnixSeconds, metadata.updatedAtUnixSeconds);
		AssertTestTree(loaded, "root", "file.txt", 42);

		RemoveTestPath(snapshotPath);
	}

	SF_TEST(file_tree_snapshot_operations, SaveOverwritesExistingSnapshot)
	{
		const std::filesystem::path snapshotPath = PrepareTestPath("overwrite.sfvb");
		Operations operations;

		Storage first;
		ApplyTestTree(first, "first", "old.txt", 10);
		SF_ASSERT_EQ(operations.TrySaveSnapshot(snapshotPath, first, {}).isSuccessful, true);

		Storage second;
		ApplyTestTree(second, "second", "new.txt", 20);
		SF_ASSERT_EQ(operations.TrySaveSnapshot(snapshotPath, second, {}).isSuccessful, true);

		Storage loaded;
		SF_ASSERT_EQ(operations.TryLoadSnapshot(snapshotPath, loaded).isSuccessful, true);
		AssertTestTree(loaded, "second", "new.txt", 20);

		RemoveTestPath(snapshotPath);
	}

	SF_TEST(file_tree_snapshot_operations, SaveEmptyStorageReturnsFailure)
	{
		const std::filesystem::path snapshotPath = PrepareTestPath("empty-storage.sfvb");
		Storage storage;
		Operations operations;

		SavedSnapshotSummary saved = operations.TrySaveSnapshot(snapshotPath, storage, {});

		SF_ASSERT_EQ(saved.isSuccessful, false);
		SF_ASSERT_EQ(saved.saveDuration.count() >= 0, true);

		RemoveTestPath(snapshotPath);
	}

	SF_TEST(file_tree_snapshot_operations, SaveToMissingParentReturnsFailure)
	{
		const std::filesystem::path snapshotPath = PrepareTestPath("missing-parent-anchor.sfvb");
		const std::filesystem::path missingDirectory = snapshotPath.parent_path() / "missing-parent";
		std::error_code ec;
		std::filesystem::remove_all(missingDirectory, ec);
		SF_ASSERT_EQ(bool(ec), false);

		Storage source;
		ApplyTestTree(source, "root", "file.txt", 42);
		Operations operations;

		SavedSnapshotSummary saved = operations.TrySaveSnapshot(missingDirectory / "snapshot.sfvb", source, {});

		SF_ASSERT_EQ(saved.isSuccessful, false);
		SF_ASSERT_EQ(saved.saveDuration.count() >= 0, true);

		RemoveTestPath(snapshotPath);
	}

	SF_TEST(file_tree_snapshot_operations, LoadMissingAndCorruptedSnapshotsLeaveFreshStorageUnchanged)
	{
		const std::filesystem::path missingPath = PrepareTestPath("missing.sfvb");
		const std::filesystem::path corruptedPath = PrepareTestPath("corrupted.sfvb");
		Operations operations;
		Storage storage;

		LoadedSnapshotSummary missing = operations.TryLoadSnapshot(missingPath, storage);

		SF_ASSERT_EQ(missing.isSuccessful, false);
		SF_ASSERT_EQ(missing.loadDuration.count() >= 0, true);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		SF_ASSERT_EQ(storage.GetVersion(), 0);
		AssertDefaultMetadata(missing.treeMetadata);

		std::ofstream corruptedOut(corruptedPath, std::ios::binary);
		corruptedOut.write("bad", 3);
		corruptedOut.close();
		SF_ASSERT_EQ(corruptedOut.fail(), false);

		LoadedSnapshotSummary corrupted = operations.TryLoadSnapshot(corruptedPath, storage);

		SF_ASSERT_EQ(corrupted.isSuccessful, false);
		SF_ASSERT_EQ(corrupted.loadDuration.count() >= 0, true);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		SF_ASSERT_EQ(storage.GetVersion(), 0);
		AssertDefaultMetadata(corrupted.treeMetadata);

		RemoveTestPath(corruptedPath);
	}

	SF_TEST(file_tree_snapshot_operations, LoadRejectsPopulatedStorageWithoutChangingIt)
	{
		const std::filesystem::path snapshotPath = PrepareTestPath("populated-destination.sfvb");
		Operations operations;
		Storage source;
		ApplyTestTree(source, "source", "source.txt", 10);
		SF_ASSERT_EQ(operations.TrySaveSnapshot(snapshotPath, source, {}).isSuccessful, true);

		Storage destination;
		Node* originalRoot = ApplyTestTree(destination, "destination", "destination.txt", 20);
		const StorageVersion originalVersion = destination.GetVersion();

		LoadedSnapshotSummary load = operations.TryLoadSnapshot(snapshotPath, destination);

		SF_ASSERT_EQ(load.isSuccessful, false);
		SF_ASSERT_EQ(load.loadDuration.count(), 0);
		AssertDefaultMetadata(load.treeMetadata);
		SF_ASSERT_EQ(destination.GetRoot() == originalRoot, true);
		SF_ASSERT_EQ(destination.GetVersion(), originalVersion);
		AssertTestTree(destination, "destination", "destination.txt", 20);

		RemoveTestPath(snapshotPath);
	}

	SF_TEST(file_tree_snapshot_operations, LoadRejectsPreviouslyUsedEmptyStorage)
	{
		const std::filesystem::path snapshotPath = PrepareTestPath("used-empty-destination.sfvb");
		Operations operations;
		Storage source;
		ApplyTestTree(source, "source", "source.txt", 10);
		SF_ASSERT_EQ(operations.TrySaveSnapshot(snapshotPath, source, {}).isSuccessful, true);

		Storage destination;
		Node* oldRoot = ApplyTestTree(destination, "old", "old.txt", 20);
		SF_ASSERT_EQ(destination.TryRemoveSubtree(oldRoot).has_value(), true);
		SF_ASSERT_EQ(destination.GetRoot() == nullptr, true);
		const StorageVersion originalVersion = destination.GetVersion();

		LoadedSnapshotSummary load = operations.TryLoadSnapshot(snapshotPath, destination);

		SF_ASSERT_EQ(load.isSuccessful, false);
		SF_ASSERT_EQ(load.loadDuration.count(), 0);
		AssertDefaultMetadata(load.treeMetadata);
		SF_ASSERT_EQ(destination.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(destination.GetNodesCount(), 0);
		SF_ASSERT_EQ(destination.GetVersion(), originalVersion);

		RemoveTestPath(snapshotPath);
	}
}
