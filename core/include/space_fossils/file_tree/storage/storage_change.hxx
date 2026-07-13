#pragma once

#include "space_fossils/file_tree/change_type.hxx"

#include <cstddef>

namespace space_fossils::core::file_tree {
	struct Node;

	struct AppliedChange
	{
		ChangeType type = ChangeType::Unknown;
		Node* target = nullptr;
		Node* addedRoot = nullptr;
		std::size_t addedNodesCount = 0;
		std::size_t removedNodesCount = 0;
	};
}
