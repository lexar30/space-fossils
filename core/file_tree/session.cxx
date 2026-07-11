#include "space_fossils/file_tree/session.hxx"
#include "space_fossils/file_tree/node.hxx"
#include "space_fossils/file_tree/storage.hxx"
#include "space_fossils/file_tree/tree_query.hxx"

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
		RefreshIfStale();
		return currentNode != nullptr;
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
		RefreshIfStale();
		return currentNode;
	}

	NativeString Session::GetCurrentNativePath()
	{
		RefreshIfStale();
		if (currentNode == nullptr) {
			return {};
		}

		return currentNativePath;
	}

	bool Session::ResetToRoot()
	{
		SelectKnownNode(storage.GetRoot());
		return currentNode != nullptr;
	}

	bool Session::TrySelect(const Node* node)
	{
		RefreshIfStale();

		if (!TreeQuery::ContainsInSubtree(storage.GetRoot(), node))
		{
			return false;
		}

		SelectKnownNode(node);

		return true;
	}

	bool Session::TrySelectChild(NativeStringView name)
	{
		RefreshIfStale();

		if (name.empty()) {
			return false;
		}

		const Node* foundChild = TreeQuery::FindChildByName(currentNode, name);
		if (foundChild == nullptr) {
			return false;
		}

		return TrySelect(foundChild);
	}

	bool Session::TrySelectChild(std::size_t index)
	{
		RefreshIfStale();

		if (index >= availableChildren.size()) {
			return false;
		}

		return TrySelect(availableChildren[index]);
	}

	bool Session::TrySelectFromRoot(NativeStringView nativePath)
	{
		RefreshIfStale();

		if (nativePath.empty()) {
			return false;
		}

		const Node* foundNode = TreeQuery::FindNodeByPath(storage.GetRoot(), nativePath);
		if (foundNode == nullptr) {
			return false;
		}

		return TrySelect(foundNode);
	}

	bool Session::TrySelectRelative(NativeStringView nativePath)
	{
		RefreshIfStale();

		const Node* foundNode = TreeQuery::FindNodeByPath(currentNode, nativePath);
		if (foundNode == nullptr) {
			return false;
		}

		return TrySelect(foundNode);
	}

	bool Session::TrySelectParent()
	{
		RefreshIfStale();

		if (currentNode == nullptr || currentNode->parent == nullptr) {
			return false;
		}

		return TrySelect(currentNode->parent);
	}

	const std::vector<const Node*>& Session::GetAvailableChildren()
	{
		RefreshIfStale();
		return availableChildren;
	}

	bool Session::HasTree()
	{
		RefreshIfStale();
		return storage.GetRoot() != nullptr;
	}

	std::size_t Session::GetFocusedChildIndex()
	{
		RefreshIfStale();

		return focusedChildIndex;
	}

	bool Session::TrySetFocusedChildIndex(std::size_t index)
	{
		RefreshIfStale();

		if (index >= availableChildren.size()) {
			return false;
		}

		focusedChildIndex = index;
		return true;
	}

	void Session::MoveFocusedChildIndex(std::ptrdiff_t delta)
	{
		RefreshIfStale();

		if (delta == 0 || availableChildren.empty()) {
			return;
		}

		const std::size_t limit = availableChildren.size();

		if (delta > 0) {
			const std::size_t shift = static_cast<std::size_t>(delta) % limit;
			if (shift >= limit - focusedChildIndex) {
				focusedChildIndex = shift - (limit - focusedChildIndex);
			}
			else {
				focusedChildIndex += shift;
			}
		}
		else {
			const std::size_t shift = (static_cast<std::size_t>(-(delta + 1)) + 1) % limit;
			if (shift > focusedChildIndex) {
				focusedChildIndex = limit - (shift - focusedChildIndex);
			}
			else {
				focusedChildIndex -= shift;
			}
		}
	}

	void Session::RefreshIfStale()
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
