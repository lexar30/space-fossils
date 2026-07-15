#include "space_fossils/core/file_tree/storage/storage.hxx"

#include "space_fossils/core/file_tree/model/tree_pool_bundle.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <memory>
#include <optional>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;

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

		Node* CreateBundleNode(TreePoolBundle& bundle, const NativeString& name)
		{
			Node* node = bundle.nodePool->Create();
			SF_ASSERT_EQ(node != nullptr, true);

			node->name = bundle.namePool->Store(name);
			++bundle.createdNodesCount;

			return node;
		}

		Node* AppendBundleChild(TreePoolBundle& bundle, Node* parent, const NativeString& name)
		{
			Node* child = CreateBundleNode(bundle, name);
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
			bundle.root = CreateBundleNode(bundle, rootName);
			bundle.root->entryType = EntryType::Directory;

			return bundle;
		}

		std::optional<AppliedChange> TryAdoptRoot(Storage& storage, TreePoolBundle&& bundle)
		{
			return storage.TryAdoptRoot(std::move(bundle));
		}

		std::optional<AppliedChange> TryAttachChild(Storage& storage, Node* parent, TreePoolBundle&& bundle)
		{
			return storage.TryAttachChild(parent, std::move(bundle));
		}

		std::optional<AppliedChange> TryReplaceSubtree(Storage& storage, Node* target, TreePoolBundle&& bundle)
		{
			return storage.TryReplaceSubtree(target, std::move(bundle));
		}

		std::optional<AppliedChange> TryRemoveSubtree(Storage& storage, Node* target)
		{
			return storage.TryRemoveSubtree(target);
		}

		Node* ApplyAdoptRoot(Storage& storage, TreePoolBundle&& bundle)
		{
			std::size_t expectedAddedNodesCount = bundle.createdNodesCount;
			std::optional<AppliedChange> appliedChange = TryAdoptRoot(storage, std::move(bundle));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, ChangeType::AdoptRoot);
			SF_ASSERT_EQ(appliedChange->addedNodesCount, expectedAddedNodesCount);

			return appliedChange->addedRoot;
		}

		Node* ApplyAttachChild(Storage& storage, Node* parent, TreePoolBundle&& bundle)
		{
			std::size_t expectedAddedNodesCount = bundle.createdNodesCount;
			std::optional<AppliedChange> appliedChange = TryAttachChild(storage, parent, std::move(bundle));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, ChangeType::Attach);
			SF_ASSERT_EQ(appliedChange->target == parent, true);
			SF_ASSERT_EQ(appliedChange->addedNodesCount, expectedAddedNodesCount);

			return appliedChange->addedRoot;
		}

		Node* ApplyReplaceSubtree(Storage& storage, Node* target, TreePoolBundle&& bundle)
		{
			std::size_t expectedAddedNodesCount = bundle.createdNodesCount;
			std::optional<AppliedChange> appliedChange = TryReplaceSubtree(storage, target, std::move(bundle));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, ChangeType::Replace);
			SF_ASSERT_EQ(appliedChange->target == target, true);
			SF_ASSERT_EQ(appliedChange->addedNodesCount, expectedAddedNodesCount);

			return appliedChange->addedRoot;
		}

		bool ApplyRemoveSubtree(Storage& storage, Node* target)
		{
			std::optional<AppliedChange> appliedChange = TryRemoveSubtree(storage, target);
			if (!appliedChange.has_value()) {
				return false;
			}

			SF_ASSERT_EQ(appliedChange->type, ChangeType::Remove);
			SF_ASSERT_EQ(appliedChange->target == target, true);
			SF_ASSERT_EQ(appliedChange->removedNodesCount != 0, true);

			return true;
		}

		void AssertNameEquals(const Node& node, const NativeString& expectedName)
		{
			NativeStringView actualName = ToStringView(node.name);

			SF_ASSERT_EQ(actualName.size(), expectedName.size());
			for (std::size_t index = 0; index < expectedName.size(); ++index) {
				SF_ASSERT_EQ(actualName[index] == expectedName[index], true);
			}
		}

		void SetDirectory(Node& node, FileSize logicalSize, EntryScanStatus scanStatus = EntryScanStatus::Complete)
		{
			node.logicalSize = logicalSize;
			node.entryType = EntryType::Directory;
			node.entryStatus = EntryStatus::Accessible;
			node.scanStatus = scanStatus;
		}

		void SetFile(Node& node, FileSize logicalSize)
		{
			node.logicalSize = logicalSize;
			node.entryType = EntryType::File;
			node.entryStatus = EntryStatus::Accessible;
			node.scanStatus = EntryScanStatus::Complete;
		}
	}

	SF_TEST(file_tree_storage, DefaultConfigStartsEmpty)
	{
		Storage storage;

		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		SF_ASSERT_EQ(storage.GetVersion(), 0);
	}

	SF_TEST(file_tree_storage, CustomConfigStartsEmpty)
	{
		Storage storage(MakeConfig());

		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, VersionBumpsAfterSuccessfulApplyChanges)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");
		NativeString replacementName = MakeNativeString("replacement");

		SF_ASSERT_EQ(storage.GetVersion(), 0);

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		SF_ASSERT_EQ(storage.GetVersion(), 1);

		Node* child = ApplyAttachChild(storage, root, MakeSubtree(childName));
		SF_ASSERT_EQ(storage.GetVersion(), 2);

		Node* replacement = ApplyReplaceSubtree(storage, child, MakeSubtree(replacementName));
		SF_ASSERT_EQ(storage.GetVersion(), 3);

		SF_ASSERT_EQ(ApplyRemoveSubtree(storage, replacement), true);
		SF_ASSERT_EQ(storage.GetVersion(), 4);
	}

	SF_TEST(file_tree_storage, VersionDoesNotBumpAfterRejectedApplyChanges)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");
		NativeString replacementName = MakeNativeString("replacement");

		TreePoolBundle emptyBundle = MakeBundle();
		std::optional<AppliedChange> rejectedAdopt = TryAdoptRoot(storage, std::move(emptyBundle));
		SF_ASSERT_EQ(rejectedAdopt.has_value(), false);
		SF_ASSERT_EQ(storage.GetVersion(), 0);

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		SF_ASSERT_EQ(storage.GetVersion(), 1);

		std::optional<AppliedChange> rejectedAttach = TryAttachChild(storage, nullptr, MakeSubtree(childName));
		SF_ASSERT_EQ(rejectedAttach.has_value(), false);
		SF_ASSERT_EQ(storage.GetVersion(), 1);

		std::optional<AppliedChange> rejectedReplace = TryReplaceSubtree(storage, nullptr, MakeSubtree(replacementName));
		SF_ASSERT_EQ(rejectedReplace.has_value(), false);
		SF_ASSERT_EQ(storage.GetVersion(), 1);

		std::optional<AppliedChange> rejectedRemove = TryRemoveSubtree(storage, nullptr);
		SF_ASSERT_EQ(rejectedRemove.has_value(), false);
		SF_ASSERT_EQ(storage.GetVersion(), 1);

		SF_ASSERT_EQ(storage.GetRoot() == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
	}

	SF_TEST(file_tree_storage, GetRootTracksCurrentRoot)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString replacementName = MakeNativeString("replacement");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		SF_ASSERT_EQ(storage.GetRoot() == root, true);

		Node* replacement = ApplyReplaceSubtree(storage, root, MakeSubtree(replacementName));
		SF_ASSERT_EQ(storage.GetRoot() == replacement, true);
	}

	SF_TEST(file_tree_storage, AdoptRootMergesBundlePools)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		TreePoolBundle bundle = MakeSubtree(rootName);
		NamePool* sourceNamePool = bundle.namePool.get();
		NodePool* sourceNodePool = bundle.nodePool.get();
		rootName[0] = static_cast<NativeChar>('x');

		std::optional<AppliedChange> appliedChange = storage.TryAdoptRoot(std::move(bundle));
		SF_ASSERT_EQ(appliedChange.has_value(), true);
		Node* root = appliedChange->addedRoot;

		SF_ASSERT_EQ(root != nullptr, true);
		SF_ASSERT_EQ(storage.GetRoot() == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
		SF_ASSERT_EQ(root->parent == nullptr, true);
		SF_ASSERT_EQ(root->firstChild == nullptr, true);
		SF_ASSERT_EQ(root->nextSibling == nullptr, true);
		SF_ASSERT_EQ(ToStringView(root->name)[0] == static_cast<NativeChar>('r'), true);
		SF_ASSERT_EQ(sourceNamePool->GetBlocksCount(), 0);
		SF_ASSERT_EQ(sourceNamePool->GetUsedBytes(), 0);
		SF_ASSERT_EQ(sourceNodePool->GetBlocksCount(), 0);
		SF_ASSERT_EQ(sourceNodePool->GetLiveNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, AdoptRootPreservesNodeMetadata)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		TreePoolBundle bundle = MakeSubtree(rootName);

		bundle.root->logicalSize = 100;
		bundle.root->entryType = EntryType::Directory;
		bundle.root->entryStatus = EntryStatus::Accessible;
		bundle.root->scanStatus = EntryScanStatus::Partial;

		Node* root = ApplyAdoptRoot(storage, std::move(bundle));

		SF_ASSERT_EQ(root->logicalSize, 100);
		SF_ASSERT_EQ(root->entryType, EntryType::Directory);
		SF_ASSERT_EQ(root->entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(root->scanStatus, EntryScanStatus::Partial);
	}

	SF_TEST(file_tree_storage, AdoptRootReplacesPreviousTree)
	{
		Storage storage(MakeConfig());
		NativeString oldRootName = MakeNativeString("old-root");
		NativeString childName = MakeNativeString("child");
		NativeString newRootName = MakeNativeString("new-root");

		TreePoolBundle oldBundle = MakeSubtree(oldRootName);
		AppendBundleChild(oldBundle, oldBundle.root, childName);
		ApplyAdoptRoot(storage, std::move(oldBundle));

		std::optional<AppliedChange> appliedChange = TryAdoptRoot(storage, MakeSubtree(newRootName));
		SF_ASSERT_EQ(appliedChange.has_value(), true);
		SF_ASSERT_EQ(appliedChange->removedNodesCount, 2);
		Node* newRoot = appliedChange->addedRoot;

		SF_ASSERT_EQ(newRoot != nullptr, true);
		SF_ASSERT_EQ(storage.GetRoot() == newRoot, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
		SF_ASSERT_EQ(newRoot->parent == nullptr, true);
		SF_ASSERT_EQ(newRoot->firstChild == nullptr, true);
		AssertNameEquals(*newRoot, newRootName);
	}

	SF_TEST(file_tree_storage, AdoptRootRejectsEmptyBundle)
	{
		Storage storage(MakeConfig());
		TreePoolBundle emptyBundle = MakeBundle();

		std::optional<AppliedChange> appliedChange = TryAdoptRoot(storage, std::move(emptyBundle));

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, AdoptRootRejectsCheapInvalidBundles)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");

		TreePoolBundle bundleWithoutNamePool = MakeSubtree(rootName);
		bundleWithoutNamePool.namePool.reset();
		std::optional<AppliedChange> missingNamePoolChange = TryAdoptRoot(storage, std::move(bundleWithoutNamePool));
		SF_ASSERT_EQ(missingNamePoolChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);

		TreePoolBundle bundleWithoutNodePool = MakeSubtree(rootName);
		bundleWithoutNodePool.nodePool.reset();
		std::optional<AppliedChange> missingNodePoolChange = TryAdoptRoot(storage, std::move(bundleWithoutNodePool));
		SF_ASSERT_EQ(missingNodePoolChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);

		TreePoolBundle rootlessBundle = MakeBundle();
		rootlessBundle.createdNodesCount = 1;
		std::optional<AppliedChange> missingRootChange = TryAdoptRoot(storage, std::move(rootlessBundle));
		SF_ASSERT_EQ(missingRootChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);

		TreePoolBundle zeroCountBundle = MakeSubtree(rootName);
		zeroCountBundle.createdNodesCount = 0;
		std::optional<AppliedChange> zeroCountChange = TryAdoptRoot(storage, std::move(zeroCountBundle));
		SF_ASSERT_EQ(zeroCountChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, AttachChildLinksBundleRootToParent)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child.txt");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		Node* child = ApplyAttachChild(storage, root, MakeSubtree(childName));

		SF_ASSERT_EQ(child != nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 2);
		SF_ASSERT_EQ(root->firstChild == child, true);
		SF_ASSERT_EQ(child->parent == root, true);
		SF_ASSERT_EQ(child->nextSibling == nullptr, true);
		SF_ASSERT_EQ(child->firstChild == nullptr, true);
		AssertNameEquals(*child, childName);
	}

	SF_TEST(file_tree_storage, AttachChildAdoptsWholeSubtree)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");
		NativeString grandChildName = MakeNativeString("grand-child");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		TreePoolBundle childBundle = MakeSubtree(childName);
		Node* grandChild = AppendBundleChild(childBundle, childBundle.root, grandChildName);

		Node* child = ApplyAttachChild(storage, root, std::move(childBundle));

		SF_ASSERT_EQ(child != nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 3);
		SF_ASSERT_EQ(child->firstChild == grandChild, true);
		SF_ASSERT_EQ(grandChild->parent == child, true);
		AssertNameEquals(*grandChild, grandChildName);
	}

	SF_TEST(file_tree_storage, AttachChildAppendsSiblingsInInsertionOrder)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("first");
		NativeString secondName = MakeNativeString("second");
		NativeString thirdName = MakeNativeString("third");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		Node* first = ApplyAttachChild(storage, root, MakeSubtree(firstName));
		Node* second = ApplyAttachChild(storage, root, MakeSubtree(secondName));
		Node* third = ApplyAttachChild(storage, root, MakeSubtree(thirdName));

		SF_ASSERT_EQ(root->firstChild == first, true);
		SF_ASSERT_EQ(first->nextSibling == second, true);
		SF_ASSERT_EQ(second->nextSibling == third, true);
		SF_ASSERT_EQ(third->nextSibling == nullptr, true);
		SF_ASSERT_EQ(first->parent == root, true);
		SF_ASSERT_EQ(second->parent == root, true);
		SF_ASSERT_EQ(third->parent == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 4);
	}

	SF_TEST(file_tree_storage, AttachChildAddsLogicalSizeToDirectoryAncestors)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString folderName = MakeNativeString("folder");
		NativeString oldFileName = MakeNativeString("old.txt");
		NativeString newFileName = MakeNativeString("new.txt");

		TreePoolBundle rootBundle = MakeSubtree(rootName);
		SetDirectory(*rootBundle.root, 10);

		Node* bundleFolder = AppendBundleChild(rootBundle, rootBundle.root, folderName);
		SetDirectory(*bundleFolder, 10);

		Node* oldFile = AppendBundleChild(rootBundle, bundleFolder, oldFileName);
		SetFile(*oldFile, 10);

		Node* root = ApplyAdoptRoot(storage, std::move(rootBundle));
		Node* folder = root->firstChild;

		TreePoolBundle newFileBundle = MakeSubtree(newFileName);
		SetFile(*newFileBundle.root, 7);

		Node* newFile = ApplyAttachChild(storage, folder, std::move(newFileBundle));

		SF_ASSERT_EQ(newFile->logicalSize, 7);
		SF_ASSERT_EQ(folder->logicalSize, 17);
		SF_ASSERT_EQ(root->logicalSize, 17);
	}

	SF_TEST(file_tree_storage, AttachChildRejectsNullParent)
	{
		Storage storage(MakeConfig());
		NativeString childName = MakeNativeString("child");

		std::optional<AppliedChange> appliedChange = TryAttachChild(storage, nullptr, MakeSubtree(childName));

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
	}

	SF_TEST(file_tree_storage, AttachChildRejectsEmptyBundle)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		TreePoolBundle emptyBundle = MakeBundle();

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		std::optional<AppliedChange> appliedChange = TryAttachChild(storage, root, std::move(emptyBundle));

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(root->firstChild == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
	}

	SF_TEST(file_tree_storage, AttachChildRejectsDetachedParent)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString detachedName = MakeNativeString("detached");
		NativeString newChildName = MakeNativeString("new-child");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		Node* detached = ApplyAttachChild(storage, root, MakeSubtree(detachedName));
		SF_ASSERT_EQ(ApplyRemoveSubtree(storage, detached), true);

		std::optional<AppliedChange> appliedChange = TryAttachChild(storage, detached, MakeSubtree(newChildName));

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == root, true);
		SF_ASSERT_EQ(root->firstChild == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
	}

	SF_TEST(file_tree_storage, AttachChildRejectsFileParent)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString fileName = MakeNativeString("file.txt");
		NativeString newChildName = MakeNativeString("new-child");

		TreePoolBundle rootBundle = MakeSubtree(rootName);
		Node* file = AppendBundleChild(rootBundle, rootBundle.root, fileName);
		SetFile(*file, 10);

		Node* root = ApplyAdoptRoot(storage, std::move(rootBundle));
		Node* fileParent = root->firstChild;

		std::optional<AppliedChange> appliedChange = TryAttachChild(storage, fileParent, MakeSubtree(newChildName));

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(fileParent->firstChild == nullptr, true);
		SF_ASSERT_EQ(storage.GetRoot() == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 2);
	}

	SF_TEST(file_tree_storage, ReplaceSubtreeKeepsSiblingList)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("first");
		NativeString oldName = MakeNativeString("old");
		NativeString thirdName = MakeNativeString("third");
		NativeString replacementName = MakeNativeString("replacement");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		Node* first = ApplyAttachChild(storage, root, MakeSubtree(firstName));
		Node* old = ApplyAttachChild(storage, root, MakeSubtree(oldName));
		Node* third = ApplyAttachChild(storage, root, MakeSubtree(thirdName));

		Node* replacement = ApplyReplaceSubtree(storage, old, MakeSubtree(replacementName));

		SF_ASSERT_EQ(replacement != nullptr, true);
		SF_ASSERT_EQ(root->firstChild == first, true);
		SF_ASSERT_EQ(first->nextSibling == replacement, true);
		SF_ASSERT_EQ(replacement->nextSibling == third, true);
		SF_ASSERT_EQ(replacement->parent == root, true);
		SF_ASSERT_EQ(third->nextSibling == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 4);
		AssertNameEquals(*replacement, replacementName);
	}

	SF_TEST(file_tree_storage, ReplaceRootChangesRoot)
	{
		Storage storage(MakeConfig());
		NativeString oldRootName = MakeNativeString("old-root");
		NativeString newRootName = MakeNativeString("new-root");

		Node* oldRoot = ApplyAdoptRoot(storage, MakeSubtree(oldRootName));
		Node* newRoot = ApplyReplaceSubtree(storage, oldRoot, MakeSubtree(newRootName));

		SF_ASSERT_EQ(newRoot != nullptr, true);
		SF_ASSERT_EQ(storage.GetRoot() == newRoot, true);
		SF_ASSERT_EQ(newRoot->parent == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
		AssertNameEquals(*newRoot, newRootName);
	}

	SF_TEST(file_tree_storage, ReplaceSubtreeRefreshesAncestorScanStatus)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");

		TreePoolBundle rootBundle = MakeSubtree(rootName);
		rootBundle.root->entryType = EntryType::Directory;
		rootBundle.root->entryStatus = EntryStatus::Accessible;
		rootBundle.root->scanStatus = EntryScanStatus::Partial;

		Node* bundleChild = AppendBundleChild(rootBundle, rootBundle.root, childName);
		bundleChild->entryType = EntryType::Directory;
		bundleChild->entryStatus = EntryStatus::Accessible;
		bundleChild->scanStatus = EntryScanStatus::Pending;

		Node* root = ApplyAdoptRoot(storage, std::move(rootBundle));
		Node* oldChild = root->firstChild;

		TreePoolBundle replacementBundle = MakeSubtree(childName);
		replacementBundle.root->entryType = EntryType::Directory;
		replacementBundle.root->entryStatus = EntryStatus::Accessible;
		replacementBundle.root->scanStatus = EntryScanStatus::Complete;

		Node* replacement = ApplyReplaceSubtree(storage, oldChild, std::move(replacementBundle));

		SF_ASSERT_EQ(replacement->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(root->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(storage.GetNodesCount(), 2);
	}

	SF_TEST(file_tree_storage, ReplaceSubtreeAppliesLogicalSizeDeltaToDirectoryAncestors)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString oldName = MakeNativeString("old.bin");
		NativeString siblingName = MakeNativeString("sibling.bin");
		NativeString replacementName = MakeNativeString("replacement.bin");

		TreePoolBundle rootBundle = MakeSubtree(rootName);
		SetDirectory(*rootBundle.root, 25);

		Node* oldChild = AppendBundleChild(rootBundle, rootBundle.root, oldName);
		SetFile(*oldChild, 20);

		Node* sibling = AppendBundleChild(rootBundle, rootBundle.root, siblingName);
		SetFile(*sibling, 5);

		Node* root = ApplyAdoptRoot(storage, std::move(rootBundle));
		Node* old = root->firstChild;

		TreePoolBundle replacementBundle = MakeSubtree(replacementName);
		SetFile(*replacementBundle.root, 7);

		Node* replacement = ApplyReplaceSubtree(storage, old, std::move(replacementBundle));

		SF_ASSERT_EQ(replacement->logicalSize, 7);
		SF_ASSERT_EQ(root->logicalSize, 12);
	}

	SF_TEST(file_tree_storage, ReplaceSubtreeRejectsNullTarget)
	{
		Storage storage(MakeConfig());
		NativeString replacementName = MakeNativeString("replacement");

		std::optional<AppliedChange> appliedChange = TryReplaceSubtree(storage, nullptr, MakeSubtree(replacementName));

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, ReplaceSubtreeRejectsDetachedTarget)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString oldName = MakeNativeString("old");
		NativeString replacementName = MakeNativeString("replacement");
		NativeString secondReplacementName = MakeNativeString("second-replacement");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		Node* old = ApplyAttachChild(storage, root, MakeSubtree(oldName));
		Node* replacement = ApplyReplaceSubtree(storage, old, MakeSubtree(replacementName));

		std::optional<AppliedChange> appliedChange = TryReplaceSubtree(storage, old, MakeSubtree(secondReplacementName));

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == root, true);
		SF_ASSERT_EQ(root->firstChild == replacement, true);
		SF_ASSERT_EQ(replacement->parent == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 2);
	}

	SF_TEST(file_tree_storage, RemoveLeafDetachesItFromSiblingList)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("first");
		NativeString secondName = MakeNativeString("second");
		NativeString thirdName = MakeNativeString("third");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		Node* first = ApplyAttachChild(storage, root, MakeSubtree(firstName));
		Node* second = ApplyAttachChild(storage, root, MakeSubtree(secondName));
		Node* third = ApplyAttachChild(storage, root, MakeSubtree(thirdName));

		bool removed = ApplyRemoveSubtree(storage, second);

		SF_ASSERT_EQ(removed, true);
		SF_ASSERT_EQ(root->firstChild == first, true);
		SF_ASSERT_EQ(first->nextSibling == third, true);
		SF_ASSERT_EQ(third->nextSibling == nullptr, true);
		SF_ASSERT_EQ(third->parent == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 3);
	}

	SF_TEST(file_tree_storage, RemoveSubtreeDetachesDescendants)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");
		NativeString grandChildName = MakeNativeString("grand-child");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		TreePoolBundle childBundle = MakeSubtree(childName);
		AppendBundleChild(childBundle, childBundle.root, grandChildName);
		Node* child = ApplyAttachChild(storage, root, std::move(childBundle));

		std::optional<AppliedChange> appliedChange = TryRemoveSubtree(storage, child);
		SF_ASSERT_EQ(appliedChange.has_value(), true);
		SF_ASSERT_EQ(appliedChange->type, ChangeType::Remove);
		SF_ASSERT_EQ(appliedChange->target == child, true);
		SF_ASSERT_EQ(appliedChange->removedNodesCount, 2);
		bool removed = true;

		SF_ASSERT_EQ(removed, true);
		SF_ASSERT_EQ(root->firstChild == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
	}

	SF_TEST(file_tree_storage, RemoveRootClearsVisibleTree)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		ApplyAttachChild(storage, root, MakeSubtree(childName));

		bool removed = ApplyRemoveSubtree(storage, root);

		SF_ASSERT_EQ(removed, true);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, RemoveSubtreeRejectsNullNode)
	{
		Storage storage(MakeConfig());

		bool removed = ApplyRemoveSubtree(storage, nullptr);

		SF_ASSERT_EQ(removed, false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, RemoveSubtreeRejectsDetachedNode)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		Node* child = ApplyAttachChild(storage, root, MakeSubtree(childName));
		SF_ASSERT_EQ(ApplyRemoveSubtree(storage, child), true);

		bool removedAgain = ApplyRemoveSubtree(storage, child);

		SF_ASSERT_EQ(removedAgain, false);
		SF_ASSERT_EQ(storage.GetRoot() == root, true);
		SF_ASSERT_EQ(root->firstChild == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
	}

	SF_TEST(file_tree_storage, RemoveSubtreeSubtractsLogicalSizeAndRefreshesAncestorStatus)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("pending");

		TreePoolBundle rootBundle = MakeSubtree(rootName);
		SetDirectory(*rootBundle.root, 30, EntryScanStatus::Partial);

		Node* child = AppendBundleChild(rootBundle, rootBundle.root, childName);
		SetDirectory(*child, 30, EntryScanStatus::Pending);

		Node* root = ApplyAdoptRoot(storage, std::move(rootBundle));
		Node* pendingChild = root->firstChild;

		bool removed = ApplyRemoveSubtree(storage, pendingChild);

		SF_ASSERT_EQ(removed, true);
		SF_ASSERT_EQ(root->logicalSize, 0);
		SF_ASSERT_EQ(root->scanStatus, EntryScanStatus::Complete);
	}

}
