#pragma once

#include "space_fossils/memory/memory_arena.hxx"

#include <cstddef>
#include <vector>

namespace space_fossils::core::file_tree {
	struct Node;

	class NodePool
	{
	public:
		explicit NodePool(std::size_t blockSize);
		~NodePool();

		NodePool(const NodePool&) = delete;
		NodePool& operator=(const NodePool&) = delete;

		Node* Create();
		bool Destroy(Node* node);
		void MergeFrom(NodePool&& other);

		void Reset();
		void Release();

		std::size_t GetLiveNodesCount() const;
		std::size_t GetAllocatedBytes() const;
		std::size_t GetUsedBytes() const;
		std::size_t GetBlocksCount() const;
		std::size_t GetBlockSize() const;

	private:
		void DestroyLiveNodes();

	private:
		memory::MemoryArena arena;
		std::vector<Node*> liveNodes;
	};
}
