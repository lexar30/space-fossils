#pragma once

#include "space_fossils/file_tree/name_pool.hxx"
#include "space_fossils/file_tree/node.hxx"
#include "space_fossils/file_tree/node_pool.hxx"

#include <cstddef>
#include <memory>

// TreePoolBundle is a detached subtree produced by Scanner.
// Storage assumes the subtree is acyclic, owned by bundle pools,
// and has correct parent/child/sibling links.
// Storage performs only cheap guard checks, not deep validation.

namespace space_fossils::core::file_tree {
	struct TreePoolBundle
	{
		std::unique_ptr<NamePool> namePool;
		std::unique_ptr<NodePool> nodePool;
		Node* root = nullptr;
		std::size_t createdNodesCount = 0;
	};
}
