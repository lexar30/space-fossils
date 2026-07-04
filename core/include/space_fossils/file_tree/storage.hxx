#pragma once

#include "space_fossils/file_tree/name_pool.hxx"
#include "space_fossils/file_tree/node_pool.hxx"
#include "space_fossils/file_tree/storage_change.hxx"

#include <cstddef>
#include <optional>

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

	private:
		std::optional<AppliedChange> AdoptRoot(TreePoolBundle&& subtree);
		std::optional<AppliedChange> AttachChild(Node* parent, TreePoolBundle&& subtree);
		std::optional<AppliedChange> ReplaceSubtree(Node* target, TreePoolBundle&& subtree);
		std::optional<AppliedChange> RemoveSubtree(Node* node);

	private:
		const StorageConfig config;
		NamePool namePool;
		NodePool nodePool;

		Node* root = nullptr;
		std::size_t nodesCount = 0;
	};
}
