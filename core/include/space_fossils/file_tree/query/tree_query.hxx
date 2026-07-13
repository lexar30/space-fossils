#pragma once

#include "space_fossils/file_tree/model/types.hxx"

#include <cstddef>
#include <vector>

namespace space_fossils::core::file_tree {
	struct Node;

	class TreeQuery
	{
	public:
		static NativeString BuildNativePath(const Node* target);
		static std::vector<NativeStringView> CollectPathComponents(const Node* target);
		static std::size_t GetDepth(const Node* target);
		static std::vector<const Node*> CollectChildren(const Node* const parent);
		static bool NodeNameEquals(const Node* node, NativeStringView name);
		static const Node* FindChildByName(const Node* const parent, const NativeStringView name);
		static const Node* FindNodeByPath(const Node* start, NativeStringView path);
		static const Node* FindClosestNodeByPath(const Node* start, NativeStringView path);
		static std::size_t CountSubtreeNodes(const Node* node);
		static bool ContainsInSiblingChain(const Node* root, const Node* target);
		static bool ContainsInSubtree(const Node* root, const Node* target);

	private:
		static bool EndsWithSeparator(NativeStringView value);
		static bool IsPathSeparator(NativeChar value);
		static bool IsCurrentPathComponent(NativeStringView value);
		static bool IsParentPathComponent(NativeStringView value);
		static bool IsNodeNamePathComponent(const Node* node, NativeStringView component);
		static NativeStringView TrimTrailingSeparators(NativeStringView value);
	};
}
