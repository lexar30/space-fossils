#include "space_fossils/core/file_tree/session/session.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/storage/storage.hxx"
#include "space_fossils/core/file_tree/query/tree_query.hxx"

namespace space_fossils::core::file_tree {
	Session::Session(const Storage& storage)
		: storage(storage)
		, currentNode(storage.GetRoot())
		, currentNativePath(TreeQuery::BuildNativePath(storage.GetRoot()))
		, knownStorageVersion(storage.GetVersion())
	{
		RefreshAvailableChildren();
	}

	bool Session::IsValid()
	{
		Sync();
		return currentNode != nullptr;
	}

	void Session::Sync()
	{
		if (knownStorageVersion == storage.GetVersion()) {
			return;
		}

		focusedChildIndex = 0;

		const Node* root = storage.GetRoot();
		if (root == nullptr) {
			currentNode = nullptr;
			availableChildren.clear();
			knownStorageVersion = storage.GetVersion();
			return;
		}

		const Node* closestNode = TreeQuery::FindClosestNodeByPath(root, currentNativePath);
		if (closestNode == nullptr) {
			closestNode = root;
		}

		SelectKnownNode(closestNode);
	}

	const Storage& Session::GetStorage() const
	{
		return storage;
	}

	const Node* Session::GetRoot() const
	{
		return storage.GetRoot();
	}

	const Node* Session::GetCurrentNode()
	{
		Sync();
		return currentNode;
	}

	NativeString Session::GetCurrentNativePath()
	{
		Sync();
		if (currentNode == nullptr) {
			return {};
		}

		return currentNativePath;
	}

	bool Session::TrySetCurrentNode(const Node* node)
	{
		Sync();

		if (!TreeQuery::ContainsInSubtree(storage.GetRoot(), node))
		{
			return false;
		}

		SelectKnownNode(node);

		return true;
	}

	const std::vector<const Node*>& Session::GetAvailableChildren()
	{
		Sync();
		return availableChildren;
	}

	bool Session::HasTree()
	{
		Sync();
		return storage.GetRoot() != nullptr;
	}

	std::size_t Session::GetFocusedChildIndex()
	{
		Sync();
		return focusedChildIndex;
	}

	bool Session::TrySetFocusedChildIndex(std::size_t index)
	{
		Sync();

		if (index >= availableChildren.size()) {
			return false;
		}

		focusedChildIndex = index;
		return true;
	}

	void Session::SelectKnownNode(const Node* node)
	{
		currentNode = node;
		currentNativePath = TreeQuery::BuildNativePath(node);
		knownStorageVersion = storage.GetVersion();
		focusedChildIndex = 0;
		RefreshAvailableChildren();
	}

	void Session::RefreshAvailableChildren()
	{
		availableChildren = std::move(TreeQuery::CollectChildren(currentNode));
	}
}
