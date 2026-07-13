#include "space_fossils/file_tree/session/session_operations.hxx"

#include "space_fossils/file_tree/model/tree_pool_bundle.hxx"
#include "space_fossils/file_tree/session/session.hxx"
#include "space_fossils/file_tree/storage/storage.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <limits>
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

		Node* ApplyAdoptRoot(Storage& storage, TreePoolBundle&& bundle)
		{
			std::optional<AppliedChange> appliedChange = storage.TryAdoptRoot(std::move(bundle));

			SF_ASSERT_EQ(appliedChange.has_value(), true);
			SF_ASSERT_EQ(appliedChange->type, ChangeType::AdoptRoot);

			return appliedChange->addedRoot;
		}

		const Node* CurrentNode(Session& session)
		{
			return session.GetCurrentNode();
		}
	}

	SF_TEST(file_tree_session_operations, NavigationOperationsChangeCurrentNode)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString folderName = MakeNativeString("folder");
		NativeString fileName = MakeNativeString("file.txt");

		TreePoolBundle bundle = MakeSubtree(rootName);
		Node* folder = AppendBundleChild(bundle, bundle.root, folderName, EntryType::Directory);
		Node* file = AppendBundleChild(bundle, bundle.root, fileName, EntryType::File);
		Node* root = ApplyAdoptRoot(storage, std::move(bundle));

		Session session(storage);
		SessionOperations operations;

		SF_ASSERT_EQ(CurrentNode(session) == root, true);

		SF_ASSERT_EQ(operations.TrySelectChild(session, 0), true);
		SF_ASSERT_EQ(CurrentNode(session) == folder, true);

		SF_ASSERT_EQ(operations.TrySelectParent(session), true);
		SF_ASSERT_EQ(CurrentNode(session) == root, true);

		SF_ASSERT_EQ(operations.TrySelectChild(session, fileName), true);
		SF_ASSERT_EQ(CurrentNode(session) == file, true);

		SF_ASSERT_EQ(operations.TryChangeDirectory(session, file), false);
		SF_ASSERT_EQ(CurrentNode(session) == file, true);

		SF_ASSERT_EQ(operations.TryResetToRoot(session), true);
		SF_ASSERT_EQ(CurrentNode(session) == root, true);

		SF_ASSERT_EQ(operations.TryChangeDirectory(session, folder), true);
		SF_ASSERT_EQ(CurrentNode(session) == folder, true);

		SF_ASSERT_EQ(operations.TryChangeNode(session, file), true);
		SF_ASSERT_EQ(CurrentNode(session) == file, true);
	}

	SF_TEST(file_tree_session_operations, NavigationRejectsInvalidTargets)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString folderName = MakeNativeString("folder");

		TreePoolBundle bundle = MakeSubtree(rootName);
		AppendBundleChild(bundle, bundle.root, folderName, EntryType::Directory);
		Node* root = ApplyAdoptRoot(storage, std::move(bundle));

		Node foreignNode;
		foreignNode.entryType = EntryType::Directory;

		Session session(storage);
		SessionOperations operations;

		SF_ASSERT_EQ(operations.TrySelectChild(session, 9), false);
		SF_ASSERT_EQ(operations.TrySelectChild(session, MakeNativeString("missing")), false);
		SF_ASSERT_EQ(operations.TrySelectParent(session), false);
		SF_ASSERT_EQ(operations.TryChangeNode(session, nullptr), false);
		SF_ASSERT_EQ(operations.TryChangeNode(session, &foreignNode), false);
		SF_ASSERT_EQ(operations.TryChangeDirectory(session, &foreignNode), false);
		SF_ASSERT_EQ(CurrentNode(session) == root, true);
	}

	SF_TEST(file_tree_session_operations, FocusOperationsWrapForwardBackward)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("first");
		NativeString secondName = MakeNativeString("second");
		NativeString thirdName = MakeNativeString("third");

		TreePoolBundle bundle = MakeSubtree(rootName);
		AppendBundleChild(bundle, bundle.root, firstName, EntryType::Directory);
		AppendBundleChild(bundle, bundle.root, secondName, EntryType::Directory);
		AppendBundleChild(bundle, bundle.root, thirdName, EntryType::Directory);
		ApplyAdoptRoot(storage, std::move(bundle));

		Session session(storage);
		SessionOperations operations;

		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 0);

		SF_ASSERT_EQ(operations.TryFocusPreviousChild(session), true);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 2);

		SF_ASSERT_EQ(operations.TryFocusNextChild(session), true);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 0);

		SF_ASSERT_EQ(operations.TryMoveFocusedChildIndexByDelta(session, 4), true);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 1);

		SF_ASSERT_EQ(operations.TryMoveFocusedChildIndexByDelta(session, -5), true);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 2);

		SF_ASSERT_EQ(operations.TryFocusChildByIndex(session, 0), true);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 0);

		SF_ASSERT_EQ(operations.TryFocusChildByName(session, secondName), true);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 1);

		SF_ASSERT_EQ(operations.TryFocusChildByName(session, {}), false);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 1);
	}

	SF_TEST(file_tree_session_operations, FocusOperationsRejectEmptyAndZeroMovement)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");

		ApplyAdoptRoot(storage, MakeSubtree(rootName));

		Session session(storage);
		SessionOperations operations;
		SF_ASSERT_EQ(session.GetAvailableChildren().empty(), true);

		SF_ASSERT_EQ(operations.TryFocusNextChild(session), false);
		SF_ASSERT_EQ(operations.TryFocusPreviousChild(session), false);
		SF_ASSERT_EQ(operations.TryMoveFocusedChildIndexByDelta(session, 0), false);
		SF_ASSERT_EQ(operations.TryMoveFocusedChildIndexByDelta(session, std::numeric_limits<std::ptrdiff_t>::min()), false);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 0);
	}

	SF_TEST(file_tree_session_operations, FocusOperationsHandleExtremeDelta)
	{
		Storage storage(MakeConfig());
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("first");
		NativeString secondName = MakeNativeString("second");
		NativeString thirdName = MakeNativeString("third");

		TreePoolBundle bundle = MakeSubtree(rootName);
		AppendBundleChild(bundle, bundle.root, firstName, EntryType::Directory);
		AppendBundleChild(bundle, bundle.root, secondName, EntryType::Directory);
		AppendBundleChild(bundle, bundle.root, thirdName, EntryType::Directory);
		ApplyAdoptRoot(storage, std::move(bundle));

		Session session(storage);
		SessionOperations operations;

		SF_ASSERT_EQ(operations.TryMoveFocusedChildIndexByDelta(session, std::numeric_limits<std::ptrdiff_t>::min()), true);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 1);

		SF_ASSERT_EQ(session.TrySetFocusedChildIndex(0), true);
		SF_ASSERT_EQ(operations.TryMoveFocusedChildIndexByDelta(session, std::numeric_limits<std::ptrdiff_t>::max()), true);
		SF_ASSERT_EQ(session.GetFocusedChildIndex(), 1);
	}
}
