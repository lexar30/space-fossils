#pragma once

#include "space_fossils/file_tree/name_pool.hxx"
#include "space_fossils/file_tree/node_pool.hxx"
#include "space_fossils/file_tree/storage_change.hxx"

#include <cstddef>
#include <optional>

// Stale, detached, or foreign targets are rejected with std::nullopt.
// Rejected changes must not mutate storage.

namespace space_fossils::core::file_tree {
	struct StorageConfig
	{
		std::size_t nodeBlockSize = sizeof(Node) * 1024;
		std::size_t nameBlockSize = sizeof(NativeChar) * 4096;
	};

	class Storage
	{
	public:
		explicit Storage(StorageConfig config = {});
		~Storage() = default;

		Node* GetRoot();
		const Node* GetRoot() const;

		Storage(const Storage&) = delete;
		Storage& operator=(const Storage&) = delete;

		void Clear();

		std::size_t GetNodesCount() const;

		std::optional<AppliedChange> ApplyChange(IncomingChange&& change);

		FileSize GetRootSize() const;

		std::size_t GetNamePoolAllocatedBytes() const;
		std::size_t GetNamePoolUsedBytes() const;
		std::size_t GetNamePoolBlocksCount() const;
		std::size_t GetNamePoolBlockSize() const;

		std::size_t GetNodePoolLiveNodesCount() const;
		std::size_t GetNodePoolAllocatedBytes() const;
		std::size_t GetNodePoolUsedBytes() const;
		std::size_t GetNodePoolBlocksCount() const;
		std::size_t GetNodePoolBlockSize() const;

	private:
		std::optional<AppliedChange> AdoptRoot(TreePoolBundle&& subtree);
		std::optional<AppliedChange> AttachChild(Node* parent, TreePoolBundle&& subtree);
		std::optional<AppliedChange> ReplaceSubtree(Node* target, TreePoolBundle&& subtree);
		std::optional<AppliedChange> RemoveSubtree(Node* node);

		static bool IsValidBundle(const TreePoolBundle& subtree);
		static void MergeBundlePools(NamePool& targetNamePool, NodePool& targetNodePool, TreePoolBundle& subtree);
		static std::size_t CountSubtreeNodes(const Node* node);
		static bool ContainsNode(const Node* current, const Node* target);
		static bool FindDirectChild(Node* parent, Node* child, Node*& previous);
		static EntryScanStatus ResolveDirectoryScanStatus(const Node& node);
		static void ApplyLogicalSizeDelta(Node& node, FileSize removedSize, FileSize addedSize);
		static void RefreshAncestorMetadata(Node* node, FileSize removedSize, FileSize addedSize);

	private:
		const StorageConfig config;
		NamePool namePool;
		NodePool nodePool;

		Node* root = nullptr;
		std::size_t nodesCount = 0;
	};
}