#include "space_fossils/file_tree/node_pool.hxx"

#include "space_fossils/file_tree/node.hxx"

#include <algorithm>
#include <memory>
#include <utility>

namespace space_fossils::core::file_tree {
	NodePool::NodePool(std::size_t blockSize)
		: arena(blockSize)
	{
	}

	NodePool::~NodePool()
	{
		DestroyLiveNodes();
	}

	Node* NodePool::Create()
	{
		void* allocation = arena.Allocate(sizeof(Node), alignof(Node));
		if (allocation == nullptr) {
			return nullptr;
		}

		Node* node = static_cast<Node*>(allocation);
		liveNodes.push_back(node);

		try {
			std::construct_at(node);
		}
		catch (...) {
			liveNodes.pop_back();
			throw;
		}

		return node;
	}

	bool NodePool::Destroy(Node* node)
	{
		if (node == nullptr) {
			return false;
		}

		auto nodeIt = std::find(liveNodes.begin(), liveNodes.end(), node);
		if (nodeIt == liveNodes.end()) {
			return false;
		}

		std::destroy_at(node);
		*nodeIt = liveNodes.back();
		liveNodes.pop_back();

		return true;
	}

	void NodePool::MergeFrom(NodePool&& other)
	{
		if (this == &other) {
			return;
		}

		liveNodes.reserve(liveNodes.size() + other.liveNodes.size());
		arena.MergeFrom(std::move(other.arena));

		for (Node* nodeIt : other.liveNodes) {
			liveNodes.push_back(nodeIt);
		}

		other.liveNodes.clear();
	}

	void NodePool::Reset()
	{
		DestroyLiveNodes();
		arena.Reset();
	}

	void NodePool::Release()
	{
		DestroyLiveNodes();
		arena.Release();
	}

	std::size_t NodePool::GetLiveNodesCount() const
	{
		return liveNodes.size();
	}

	std::size_t NodePool::GetAllocatedBytes() const
	{
		return arena.GetAllocatedBytes();
	}

	std::size_t NodePool::GetUsedBytes() const
	{
		return arena.GetUsedSize();
	}

	std::size_t NodePool::GetBlocksCount() const
	{
		return arena.GetBlocksCount();
	}

	std::size_t NodePool::GetBlockSize() const
	{
		return arena.GetBlockSize();
	}

	void NodePool::DestroyLiveNodes()
	{
		for (Node* nodeIt : liveNodes) {
			std::destroy_at(nodeIt);
		}

		liveNodes.clear();
	}
}
