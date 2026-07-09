#include "space_fossils/file_tree/session.hxx"
#include "space_fossils/file_tree/storage.hxx"
#include "space_fossils/file_tree/tree_query.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <cstdint>
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

		IncomingChange MakeRemoveChange(Node* target)
		{
			IncomingChange change;
			change.type = IncomingChangeType::Remove;
			change.target = target;

			return change;
		}

		Node* ApplyAdoptRoot(Storage& storage, TreePoolBundle&& bundle)
		{
			std::optional<AppliedChange> appliedChange = storage.ApplyChange(MakeAdoptRootChange(std::move(bundle)));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, IncomingChangeType::AdoptRoot);

			return appliedChange->addedRoot;
		}

		bool ApplyRemoveSubtree(Storage& storage, Node* target)
		{
			std::optional<AppliedChange> appliedChange = storage.ApplyChange(MakeRemoveChange(target));
			return appliedChange.has_value();
		}

		void AssertNativeStringEquals(NativeStringView actual, NativeStringView expected)
		{
			SF_ASSERT_EQ(actual.size(), expected.size());
			for (std::size_t index = 0; index < expected.size(); ++index) {
				SF_ASSERT_EQ(actual[index] == expected[index], true);
			}
		}
	}

	SF_TEST(file_tree_session, StartsAtRootAndCachesAvailableChildren)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");

		TreePoolBundle bundle = MakeSubtree(rootName);
		Node* bundleChild = AppendBundleChild(bundle, bundle.root, childName);
		Node* root = ApplyAdoptRoot(storage, std::move(bundle));

		Session session(storage);

		SF_ASSERT_EQ(session.IsValid(), true);
		SF_ASSERT_EQ(session.GetCurrentNode() == root, true);
		AssertNativeStringEquals(session.GetCurrentNativePath(), TreeQuery::BuildNativePath(root));
		SF_ASSERT_EQ(session.GetAvailableChildren().size(), 1);
		SF_ASSERT_EQ(session.GetAvailableChildren()[0] == bundleChild, true);
	}

	SF_TEST(file_tree_session, TrySelectFromRootAcceptsFullNativePath)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("C:");
		NativeString folderName = MakeNativeString("folder");
		NativeString fileName = MakeNativeString("file.txt");

		TreePoolBundle bundle = MakeSubtree(rootName);
		Node* folder = AppendBundleChild(bundle, bundle.root, folderName);
		Node* file = AppendBundleChild(bundle, folder, fileName);
		ApplyAdoptRoot(storage, std::move(bundle));

		Session session(storage);
		NativeString filePath = TreeQuery::BuildNativePath(file);

		SF_ASSERT_EQ(session.TrySelectFromRoot(filePath), true);
		SF_ASSERT_EQ(session.GetCurrentNode() == file, true);
	}

	SF_TEST(file_tree_session, RestoresClosestNodeWhenSelectedPathLosesMiddleComponent)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("C:");
		NativeString folderOneName = MakeNativeString("folder1");
		NativeString folderTwoName = MakeNativeString("folder2");
		NativeString folderThreeName = MakeNativeString("folder3");

		TreePoolBundle bundle = MakeSubtree(rootName);
		Node* folderOne = AppendBundleChild(bundle, bundle.root, folderOneName);
		Node* folderTwo = AppendBundleChild(bundle, folderOne, folderTwoName);
		Node* folderThree = AppendBundleChild(bundle, folderTwo, folderThreeName);
		ApplyAdoptRoot(storage, std::move(bundle));

		Session session(storage);
		SF_ASSERT_EQ(session.TrySelectFromRoot(TreeQuery::BuildNativePath(folderThree)), true);

		SF_ASSERT_EQ(ApplyRemoveSubtree(storage, folderTwo), true);

		SF_ASSERT_EQ(session.GetCurrentNode() == folderOne, true);
		AssertNativeStringEquals(session.GetCurrentNativePath(), TreeQuery::BuildNativePath(folderOne));
		SF_ASSERT_EQ(session.GetAvailableChildren().empty(), true);
	}

	SF_TEST(file_tree_session, ResetToRootRecoversAfterStorageVersionChanges)
	{
		Storage storage(MakeConfig());
		NativeString oldRootName = MakeNativeString("old-root");
		NativeString childName = MakeNativeString("child");
		NativeString newRootName = MakeNativeString("new-root");

		TreePoolBundle oldBundle = MakeSubtree(oldRootName);
		Node* oldChild = AppendBundleChild(oldBundle, oldBundle.root, childName);
		ApplyAdoptRoot(storage, std::move(oldBundle));

		Session session(storage);
		SF_ASSERT_EQ(session.TrySelect(oldChild), true);

		Node* newRoot = ApplyAdoptRoot(storage, MakeSubtree(newRootName));

		SF_ASSERT_EQ(session.ResetToRoot(), true);
		SF_ASSERT_EQ(session.GetCurrentNode() == newRoot, true);
		AssertNativeStringEquals(session.GetCurrentNativePath(), TreeQuery::BuildNativePath(newRoot));
	}

	SF_TEST(file_tree_session, EmptyStorageDoesNotExposeStalePointersAfterClear)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("child");

		TreePoolBundle bundle = MakeSubtree(rootName);
		AppendBundleChild(bundle, bundle.root, childName);
		ApplyAdoptRoot(storage, std::move(bundle));

		Session session(storage);
		storage.Clear();

		SF_ASSERT_EQ(session.IsValid(), false);
		SF_ASSERT_EQ(session.HasTree(), false);
		SF_ASSERT_EQ(session.GetCurrentNode() == nullptr, true);
		SF_ASSERT_EQ(session.GetCurrentNativePath().empty(), true);
		SF_ASSERT_EQ(session.GetAvailableChildren().empty(), true);
	}

	SF_TEST(file_tree_session, KeepsCachedPathAcrossEmptyStorageAndRestoresAfterReload)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("C:");
		NativeString folderName = MakeNativeString("folder");

		TreePoolBundle oldBundle = MakeSubtree(rootName);
		Node* oldFolder = AppendBundleChild(oldBundle, oldBundle.root, folderName);
		ApplyAdoptRoot(storage, std::move(oldBundle));

		Session session(storage);
		SF_ASSERT_EQ(session.TrySelect(oldFolder), true);

		storage.Clear();
		SF_ASSERT_EQ(session.GetCurrentNode() == nullptr, true);

		TreePoolBundle newBundle = MakeSubtree(rootName);
		Node* newFolder = AppendBundleChild(newBundle, newBundle.root, folderName);
		ApplyAdoptRoot(storage, std::move(newBundle));

		SF_ASSERT_EQ(session.GetCurrentNode() == newFolder, true);
		AssertNativeStringEquals(session.GetCurrentNativePath(), TreeQuery::BuildNativePath(newFolder));
	}
}
