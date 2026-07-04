#pragma once

#include "space_fossils/file_tree/name_pool.hxx"
#include "space_fossils/file_tree/node.hxx"
#include "space_fossils/file_tree/node_pool.hxx"
#include "space_fossils/file_tree/types.hxx"

#include <cstddef>
#include <memory>
#include <optional>

namespace space_fossils::core::file_tree {
	struct StorageConfig
	{
		std::size_t nodeBlockSize = sizeof(Node) * 1024;
		std::size_t nameBlockSize = sizeof(NativeChar) * 4096;
	};

	struct TreePoolBundle
	{
		std::unique_ptr<NamePool> namePool;
		std::unique_ptr<NodePool> nodePool;
		Node* root = nullptr;
		std::size_t createdNodesCount = 0;
	};

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
