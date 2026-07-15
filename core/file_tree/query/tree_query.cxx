#include "space_fossils/core/file_tree/query/tree_query.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/model/types.hxx"

#include <ranges>

namespace space_fossils::core::file_tree {
	NativeString TreeQuery::BuildNativePath(const Node* const node)
	{
		if (node == nullptr) {
			return {};
		}

		const std::vector<NativeStringView> pathComponents = CollectPathComponents(node);
		if (pathComponents.empty()) {
			return {};
		}

		NativeString nativePath{};
		for (std::size_t componentIndex = 0; componentIndex < pathComponents.size(); ++componentIndex) {
			if (componentIndex != 0 && !EndsWithSeparator(nativePath)) {
				nativePath.push_back(static_cast<NativeChar>('\\'));
			}

			nativePath.append(pathComponents[componentIndex]);
		}

		return nativePath;
	}

	std::vector<NativeStringView> TreeQuery::CollectPathComponents(const Node* node)
	{
		if (node == nullptr) {
			return {};
		}

		std::vector<NativeStringView> components;
		components.reserve(8);

		components.push_back(ToStringView(node->name));

		while (node->parent != nullptr) {
			node = node->parent;
			components.push_back(ToStringView(node->name));
		}

		std::ranges::reverse(components);
		return components;
	}

	std::size_t TreeQuery::GetDepth(const Node* target)
	{
		if (target == nullptr) {
			return 0;
		}

		std::size_t depth = 0;

		while (target->parent != nullptr) {
			target = target->parent;
			++depth;
		}

		return depth;
	}

	std::vector<const Node*> TreeQuery::CollectChildren(const Node* const parent)
	{
		if (parent == nullptr) {
			return {};
		}

		std::vector<const Node*> children;
		children.reserve(32);

		const Node* nextChild = parent->firstChild;
		while (nextChild != nullptr) {
			children.push_back(nextChild);
			nextChild = nextChild->nextSibling;
		}

		return children;
	}

	bool TreeQuery::NodeNameEquals(const Node* node, NativeStringView name)
	{
		if (node == nullptr) {
			return false;
		}

		return ToStringView(node->name) == name;
	}

	const Node* TreeQuery::FindChildByName(const Node* const parent, const NativeStringView name)
	{
		if (parent == nullptr) {
			return nullptr;
		}

		const Node* child = parent->firstChild;
		while (child != nullptr) {
			if (NodeNameEquals(child, name)) {
				break;
			}

			child = child->nextSibling;
		}

		return child;
	}

	const Node* TreeQuery::FindNodeByPath(const Node* start, NativeStringView path)
	{
		if (start == nullptr) {
			return nullptr;
		}

		const Node* current = start;
		bool isFirstComponent = true;
		std::size_t componentStart = 0;
		while (componentStart < path.size()) {
			while (componentStart < path.size() && IsPathSeparator(path[componentStart])) {
				++componentStart;
			}

			if (componentStart >= path.size()) {
				break;
			}

			std::size_t componentEnd = componentStart;
			while (componentEnd < path.size() && !IsPathSeparator(path[componentEnd])) {
				++componentEnd;
			}

			const NativeStringView component = path.substr(componentStart, componentEnd - componentStart);
			if (IsCurrentPathComponent(component)) {
				componentStart = componentEnd;
				isFirstComponent = false;
				continue;
			}

			if (isFirstComponent && current->parent == nullptr && IsNodeNamePathComponent(current, component)) {
				componentStart = componentEnd;
				isFirstComponent = false;
				continue;
			}

			if (IsParentPathComponent(component)) {
				if (current->parent == nullptr) {
					return nullptr;
				}

				current = current->parent;
				componentStart = componentEnd;
				isFirstComponent = false;
				continue;
			}

			current = FindChildByName(current, component);
			if (current == nullptr) {
				return nullptr;
			}

			componentStart = componentEnd;
			isFirstComponent = false;
		}

		return current;
	}

	const Node* TreeQuery::FindClosestNodeByPath(const Node* start, NativeStringView path)
	{
		if (start == nullptr) {
			return nullptr;
		}

		const Node* current = start;
		bool isFirstComponent = true;
		std::size_t componentStart = 0;
		while (componentStart < path.size()) {
			while (componentStart < path.size() && IsPathSeparator(path[componentStart])) {
				++componentStart;
			}

			if (componentStart >= path.size()) {
				break;
			}

			std::size_t componentEnd = componentStart;
			while (componentEnd < path.size() && !IsPathSeparator(path[componentEnd])) {
				++componentEnd;
			}

			const NativeStringView component = path.substr(componentStart, componentEnd - componentStart);
			if (IsCurrentPathComponent(component)) {
				componentStart = componentEnd;
				isFirstComponent = false;
				continue;
			}

			if (isFirstComponent && current->parent == nullptr && IsNodeNamePathComponent(current, component)) {
				componentStart = componentEnd;
				isFirstComponent = false;
				continue;
			}

			if (IsParentPathComponent(component)) {
				if (current->parent == nullptr) {
					return current;
				}

				current = current->parent;
				componentStart = componentEnd;
				isFirstComponent = false;
				continue;
			}

			const Node* next = FindChildByName(current, component);
			if (next == nullptr) {
				return current;
			}

			current = next;
			componentStart = componentEnd;
			isFirstComponent = false;
		}

		return current;
	}

	std::size_t TreeQuery::CountSubtreeNodes(const Node* node)
	{
		if (node == nullptr) {
			return 0;
		}

		std::size_t count = 1;
		const Node* child = node->firstChild;
		while (child != nullptr) {
			count += CountSubtreeNodes(child);
			child = child->nextSibling;
		}

		return count;
	}

	bool TreeQuery::ContainsInSiblingChain(const Node* root, const Node* target)
	{
		if (root == nullptr || target == nullptr) {
			return false;
		}

		while (root != nullptr) {
			if (root == target) {
				return true;
			}

			// TODO: exceptionally expensive!
			if (ContainsInSiblingChain(root->firstChild, target)) {
				return true;
			}

			root = root->nextSibling;
		}

		return false;
	}

	bool TreeQuery::ContainsInSubtree(const Node* root, const Node* target)
	{
		if (root == nullptr || target == nullptr) {
			return false;
		}

		if (root == target) {
			return true;
		}

		const Node* child = root->firstChild;
		while (child != nullptr) {
			if (ContainsInSubtree(child, target)) {
				return true;
			}

			child = child->nextSibling;
		}

		return false;
	}

	bool TreeQuery::EndsWithSeparator(NativeStringView value)
	{
		return !value.empty() && IsPathSeparator(value.back());
	}

	bool TreeQuery::IsPathSeparator(NativeChar value)
	{
		return value == static_cast<NativeChar>('\\')
			|| value == static_cast<NativeChar>('/');
	}

	bool TreeQuery::IsCurrentPathComponent(NativeStringView value)
	{
		return value.size() == 1 && value[0] == static_cast<NativeChar>('.');
	}

	bool TreeQuery::IsParentPathComponent(NativeStringView value)
	{
		return value.size() == 2
			&& value[0] == static_cast<NativeChar>('.')
			&& value[1] == static_cast<NativeChar>('.');
	}

	bool TreeQuery::IsNodeNamePathComponent(const Node* node, NativeStringView component)
	{
		if (node == nullptr) {
			return false;
		}

		return TrimTrailingSeparators(ToStringView(node->name)) == TrimTrailingSeparators(component);
	}

	NativeStringView TreeQuery::TrimTrailingSeparators(NativeStringView value)
	{
		while (!value.empty() && IsPathSeparator(value.back())) {
			value.remove_suffix(1);
		}

		return value;
	}
}
