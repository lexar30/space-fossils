#include "space_fossils/file_tree/storage.hxx"

#include <optional>
#include <utility>

namespace space_fossils::core::file_tree {
	namespace {
		bool IsValidBundle(const TreePoolBundle& subtree)
		{
			return subtree.namePool != nullptr
				&& subtree.nodePool != nullptr
				&& subtree.root != nullptr
				&& subtree.createdNodesCount > 0;
		}

		void MergeBundlePools(NamePool& targetNamePool, NodePool& targetNodePool, TreePoolBundle& subtree)
		{
			targetNamePool.MergeFrom(std::move(*subtree.namePool));
			targetNodePool.MergeFrom(std::move(*subtree.nodePool));
		}

		std::size_t CountSubtreeNodes(const Node* node)
		{
			if (node == nullptr) {
				return 0;
			}

			std::size_t count = 1;
			const Node* child = node->firstChild;
			while (child != nullptr) {
				count += CountSubtreeNodes(child);
				child = child->nextSibling;
			}

			return count;
		}

		bool ContainsNode(const Node* current, const Node* target)
		{
			while (current != nullptr) {
				if (current == target) {
					return true;
				}

				if (ContainsNode(current->firstChild, target)) {
					return true;
				}

				current = current->nextSibling;
			}

			return false;
		}

		bool FindDirectChild(Node* parent, Node* child, Node*& previous)
		{
			previous = nullptr;
			if (parent == nullptr || child == nullptr) {
				return false;
			}

			Node* current = parent->firstChild;
			while (current != nullptr) {
				if (current == child) {
					return true;
				}

				previous = current;
				current = current->nextSibling;
			}

			previous = nullptr;
			return false;
		}
	}

	Storage::Storage(StorageConfig config)
		: config(config)
		, namePool(config.nameBlockSize)
		, nodePool(config.nodeBlockSize)
	{
	}

	std::optional<AppliedChange> Storage::ApplyChange(IncomingChange&& change)
	{
		switch (change.type) {
		case IncomingChangeType::AdoptRoot:
			return AdoptRoot(std::move(change.bundle));
		case IncomingChangeType::Attach:
			return AttachChild(change.target, std::move(change.bundle));
		case IncomingChangeType::Replace:
			return ReplaceSubtree(change.target, std::move(change.bundle));
		case IncomingChangeType::Remove:
			return RemoveSubtree(change.target);
		default:
			return std::nullopt;
		}
	}

	std::optional<AppliedChange> Storage::AdoptRoot(TreePoolBundle&& subtree)
	{
		if (!IsValidBundle(subtree)) {
			return std::nullopt;
		}

		Node* oldRoot = root;
		Node* newRoot = subtree.root;
		std::size_t removedNodesCount = nodesCount;
		std::size_t addedNodesCount = subtree.createdNodesCount;

		MergeBundlePools(namePool, nodePool, subtree);

		newRoot->parent = nullptr;
		newRoot->nextSibling = nullptr;

		root = newRoot;
		nodesCount = addedNodesCount;

		return AppliedChange {
			IncomingChangeType::AdoptRoot,
			oldRoot,
			newRoot,
			addedNodesCount,
			removedNodesCount
		};
	}

	std::optional<AppliedChange> Storage::AttachChild(Node* parent, TreePoolBundle&& subtree)
	{
		if (parent == nullptr || !IsValidBundle(subtree) || !ContainsNode(root, parent)) {
			return std::nullopt;
		}

		Node* child = subtree.root;
		std::size_t addedNodesCount = subtree.createdNodesCount;

		MergeBundlePools(namePool, nodePool, subtree);

		child->parent = parent;
		child->nextSibling = nullptr;

		if (parent->firstChild == nullptr) {
			parent->firstChild = child;
		}
		else {
			Node* lastChild = parent->firstChild;
			while (lastChild->nextSibling != nullptr) {
				lastChild = lastChild->nextSibling;
			}

			lastChild->nextSibling = child;
		}

		nodesCount += addedNodesCount;

		return AppliedChange {
			IncomingChangeType::Attach,
			parent,
			child,
			addedNodesCount,
			0
		};
	}

	std::optional<AppliedChange> Storage::ReplaceSubtree(Node* target, TreePoolBundle&& subtree)
	{
		if (target == nullptr || !IsValidBundle(subtree) || !ContainsNode(root, target)) {
			return std::nullopt;
		}

		Node* replacement = subtree.root;
		Node* parent = target->parent;
		Node* nextSibling = target->nextSibling;
		std::size_t removedNodesCount = CountSubtreeNodes(target);
		std::size_t addedNodesCount = subtree.createdNodesCount;

		if (target == root) {
			MergeBundlePools(namePool, nodePool, subtree);
			replacement->parent = nullptr;
			replacement->nextSibling = nullptr;
			root = replacement;
		}
		else {
			Node* previousSibling = nullptr;
			if (!FindDirectChild(parent, target, previousSibling)) {
				return std::nullopt;
			}

			MergeBundlePools(namePool, nodePool, subtree);
			replacement->parent = parent;
			replacement->nextSibling = nextSibling;

			if (previousSibling == nullptr) {
				parent->firstChild = replacement;
			}
			else {
				previousSibling->nextSibling = replacement;
			}
		}

		target->parent = nullptr;
		target->nextSibling = nullptr;

		nodesCount = nodesCount - removedNodesCount + addedNodesCount;

		return AppliedChange {
			IncomingChangeType::Replace,
			target,
			replacement,
			addedNodesCount,
			removedNodesCount
		};
	}

	std::optional<AppliedChange> Storage::RemoveSubtree(Node* node)
	{
		if (node == nullptr || !ContainsNode(root, node)) {
			return std::nullopt;
		}

		std::size_t removedNodesCount = CountSubtreeNodes(node);

		if (node == root) {
			root = nullptr;
		}
		else {
			Node* previousSibling = nullptr;
			if (!FindDirectChild(node->parent, node, previousSibling)) {
				return std::nullopt;
			}

			if (previousSibling == nullptr) {
				node->parent->firstChild = node->nextSibling;
			}
			else {
				previousSibling->nextSibling = node->nextSibling;
			}
		}

		node->parent = nullptr;
		node->nextSibling = nullptr;
		nodesCount -= removedNodesCount;

		return AppliedChange {
			IncomingChangeType::Remove,
			node,
			nullptr,
			0,
			removedNodesCount
		};
	}

	void Storage::Clear()
	{
		nodesCount = 0;
		root = nullptr;
		nodePool.Release();
		namePool.Release();
	}

	Node* Storage::GetRoot()
	{
		return root;
	}

	const Node* Storage::GetRoot() const
	{
		return root;
	}

	std::size_t Storage::GetNodesCount() const
	{
		return nodesCount;
	}
}
