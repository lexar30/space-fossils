#pragma once

#include "space_fossils/file_tree/model/default_constants.hxx"
#include "space_fossils/file_tree/memory/name_pool.hxx"
#include "space_fossils/file_tree/memory/node_pool.hxx"
#include "space_fossils/file_tree/storage/storage_change.hxx"

#include <cstddef>
#include <optional>

// Stale, detached, or foreign targets are rejected with std::nullopt.
// Rejected changes must not mutate storage.

namespace space_fossils::core::file_tree {
	struct TreePoolBundle;

	struct StorageConfig
	{
		std::size_t nodeBlockSize = DefaultNodeBlockSize;
		std::size_t nameBlockSize = DefaultNameBlockSize;
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

		std::size_t GetNodesCount() const;

		std::optional<AppliedChange> TryAdoptRoot(TreePoolBundle&& subtree);
		std::optional<AppliedChange> TryAttachChild(Node* parent, TreePoolBundle&& subtree);
		std::optional<AppliedChange> TryReplaceSubtree(Node* target, TreePoolBundle&& subtree);
		std::optional<AppliedChange> TryRemoveSubtree(Node* node);

		FileSize GetRootSize() const;

		StorageVersion GetVersion() const;

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
		static bool IsValidBundle(const TreePoolBundle& subtree);
		static void MergeBundlePools(NamePool& targetNamePool, NodePool& targetNodePool, TreePoolBundle& subtree);
		static bool FindDirectChild(Node* parent, Node* child, Node*& previous);
		static EntryScanStatus ResolveDirectoryScanStatus(const Node& node);
		static void ApplyLogicalSizeDelta(Node& node, FileSize removedSize, FileSize addedSize);
		static void RefreshAncestorMetadata(Node* node, FileSize removedSize, FileSize addedSize);

	private:
		const StorageConfig config;
		StorageVersion version = 0;

		NamePool namePool;
		NodePool nodePool;

		Node* root = nullptr;
		std::size_t nodesCount = 0;
	};
}