#include "space_fossils/file_tree/session/session.hxx"

#include "space_fossils/file_tree/model/node.hxx"
#include "space_fossils/file_tree/storage/storage.hxx"
#include "space_fossils/file_tree/query/tree_query.hxx"

namespace space_fossils::core::file_tree {
	Session::Session(const Storage& storage)
		: storage(storage)
	{
		nodeHandle = { storage.GetRoot(), TreeQuery::BuildNativePath(storage.GetRoot()), storage.GetVersion() };
		RefreshAvailableChildren();
	}

	bool Session::IsValid()
	{
		Sync();
		return nodeHandle.cachedNode != nullptr;
	}

	void Session::Sync()
	{
		if (nodeHandle.storageVersion == storage.GetVersion()) {
			return;
		}

		focusedChildIndex = 0;

		const Node* root = storage.GetRoot();
		if (root == nullptr) {
			nodeHandle.cachedNode = nullptr;
			availableChildren.clear();
			nodeHandle.storageVersion = storage.GetVersion();
			return;
		}

		const Node* closestNode = TreeQuery::FindClosestNodeByPath(root, nodeHandle.nativePath);
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

	NodeHandle Session::GetCurrentNodeHandle()
	{
		Sync();
		return nodeHandle;
	}

	NativeString Session::GetCurrentNativePath()
	{
		Sync();
		if (nodeHandle.cachedNode == nullptr) {
			return {};
		}

		return nodeHandle.nativePath;
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
		nodeHandle.cachedNode = node;
		nodeHandle.nativePath = TreeQuery::BuildNativePath(node);
		nodeHandle.storageVersion = storage.GetVersion();
		focusedChildIndex = 0;
		RefreshAvailableChildren();
	}

	void Session::RefreshAvailableChildren()
	{
		availableChildren = std::move(TreeQuery::CollectChildren(nodeHandle.cachedNode));
	}
}
