#pragma once

#include "space_fossils/file_tree/node.hxx"
#include "space_fossils/file_tree/tree_pool_bundle.hxx"

#include <cstddef>

namespace space_fossils::core::file_tree {
	enum class IncomingChangeType
	{
		Unknown,
		AdoptRoot,
		Attach,
		Replace,
		Remove
	};

	struct IncomingChange
	{
		IncomingChangeType type = IncomingChangeType::Unknown;
		Node* target = nullptr;
		TreePoolBundle bundle;
	};

	struct AppliedChange
	{
		IncomingChangeType type = IncomingChangeType::Unknown;
		Node* target = nullptr;
		Node* addedRoot = nullptr;
		std::size_t addedNodesCount = 0;
		std::size_t removedNodesCount = 0;
	};
}
