#pragma once

#include "space_fossils/file_tree/name_pool.hxx"
#include "space_fossils/file_tree/node.hxx"
#include "space_fossils/file_tree/node_pool.hxx"

#include <cstddef>
#include <memory>

namespace space_fossils::core::file_tree {
	struct TreePoolBundle
	{
		std::unique_ptr<NamePool> namePool;
		std::unique_ptr<NodePool> nodePool;
		Node* root = nullptr;
		std::size_t createdNodesCount = 0;
	};
}
