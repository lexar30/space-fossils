#include "space_fossils/file_tree/storage.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <memory>
#include <optional>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;

		StorageConfig MakeConfig()
		{
			return StorageConfig {
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

			return bundle;
		}

		IncomingChange MakeAdoptRootChange(TreePoolBundle&& bundle)
		{
			IncomingChange change;
			change.type = IncomingChangeType::AdoptRoot;
			change.bundle = std::move(bundle);

			return change;
		}

		IncomingChange MakeAttachChange(Node* parent, TreePoolBundle&& bundle)
		{
			IncomingChange change;
			change.type = IncomingChangeType::Attach;
			change.target = parent;
			change.bundle = std::move(bundle);

			return change;
		}

		IncomingChange MakeReplaceChange(Node* target, TreePoolBundle&& bundle)
		{
			IncomingChange change;
			change.type = IncomingChangeType::Replace;
			change.target = target;
			change.bundle = std::move(bundle);

			return change;
		}

		IncomingChange MakeRemoveChange(Node* target)
		{
			IncomingChange change;
			change.type = IncomingChangeType::Remove;
			change.target = target;

			return change;
		}

		std::optional<AppliedChange> TryAdoptRoot(Storage& storage, TreePoolBundle&& bundle)
		{
			return storage.ApplyChange(MakeAdoptRootChange(std::move(bundle)));
		}

		std::optional<AppliedChange> TryAttachChild(Storage& storage, Node* parent, TreePoolBundle&& bundle)
		{
			return storage.ApplyChange(MakeAttachChange(parent, std::move(bundle)));
		}

		std::optional<AppliedChange> TryReplaceSubtree(Storage& storage, Node* target, TreePoolBundle&& bundle)
		{
			return storage.ApplyChange(MakeReplaceChange(target, std::move(bundle)));
		}

		std::optional<AppliedChange> TryRemoveSubtree(Storage& storage, Node* target)
		{
			return storage.ApplyChange(MakeRemoveChange(target));
		}

		Node* ApplyAdoptRoot(Storage& storage, TreePoolBundle&& bundle)
		{
			std::size_t expectedAddedNodesCount = bundle.createdNodesCount;
			std::optional<AppliedChange> appliedChange = TryAdoptRoot(storage, std::move(bundle));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::AdoptRoot);
			SF_ASSERT_EQ(appliedChange->addedNodesCount, expectedAddedNodesCount);

			return appliedChange->addedRoot;
		}

		Node* ApplyAttachChild(Storage& storage, Node* parent, TreePoolBundle&& bundle)
		{
			std::size_t expectedAddedNodesCount = bundle.createdNodesCount;
			std::optional<AppliedChange> appliedChange = TryAttachChild(storage, parent, std::move(bundle));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::Attach);
			SF_ASSERT_EQ(appliedChange->target == parent, true);
			SF_ASSERT_EQ(appliedChange->addedNodesCount, expectedAddedNodesCount);

			return appliedChange->addedRoot;
		}

		Node* ApplyReplaceSubtree(Storage& storage, Node* target, TreePoolBundle&& bundle)
		{
			std::size_t expectedAddedNodesCount = bundle.createdNodesCount;
			std::optional<AppliedChange> appliedChange = TryReplaceSubtree(storage, target, std::move(bundle));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::Replace);
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

			SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::Remove);
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
	}

	SF_TEST(file_tree_storage, DefaultConfigStartsEmpty)
	{
		Storage storage;

		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, CustomConfigStartsEmpty)
	{
		Storage storage(MakeConfig());

		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, ApplyChangeRejectsUnknownType)
	{
		Storage storage(MakeConfig());
		IncomingChange change;

		std::optional<AppliedChange> appliedChange = storage.ApplyChange(std::move(change));

		SF_ASSERT_EQ(appliedChange.has_value(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}

	SF_TEST(file_tree_storage, AdoptRootMergesBundlePools)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		IncomingChange change = MakeAdoptRootChange(MakeSubtree(rootName));
		NamePool* sourceNamePool = change.bundle.namePool.get();
		NodePool* sourceNodePool = change.bundle.nodePool.get();
		rootName[0] = static_cast<NativeChar>('x');

		std::optional<AppliedChange> appliedChange = storage.ApplyChange(std::move(change));
		SF_ASSERT_EQ(appliedChange.has_value(), true);
		Node* root = appliedChange->addedRoot;

		SF_ASSERT_EQ(root != nullptr, true);
		SF_ASSERT_EQ(storage.GetRoot() == root, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);
		SF_ASSERT_EQ(root->parent == nullptr, true);
		SF_ASSERT_EQ(root->firstChild == nullptr, true);
		SF_ASSERT_EQ(root->nextSibling == nullptr, true);
		SF_ASSERT_EQ(ToStringView(root->name)[0] == static_cast<NativeChar>('r'), true);
		SF_ASSERT_EQ(change.bundle.namePool.get() == sourceNamePool, true);
		SF_ASSERT_EQ(change.bundle.nodePool.get() == sourceNodePool, true);
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
		SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::Remove);
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

	SF_TEST(file_tree_storage, ClearRemovesWholeTree)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");

		Node* root = ApplyAdoptRoot(storage, MakeSubtree(rootName));
		ApplyAttachChild(storage, root, MakeSubtree(childName));

		storage.Clear();

		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
	}
}
