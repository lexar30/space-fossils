#include "space_fossils/file_tree_node.hxx"

#include <cassert>
#include <utility>

namespace space_fossils::core {
	space_fossils::core::FileTreeNode::FileTreeNode(std::string name, FileTreeNodeType type, std::uint64_t size)
		: type(type)
		, name(std::move(name))
		, size(size)
	{ }

	FileTreeNode::FileTreeNodeType space_fossils::core::FileTreeNode::GetType() const
	{
		return type;
	}

	std::string_view space_fossils::core::FileTreeNode::GetName() const
	{
		return name;
	}

	std::uint64_t space_fossils::core::FileTreeNode::GetSize() const
	{
		return size;
	}

	FileTreeNode* space_fossils::core::FileTreeNode::GetParent()
	{
		return parent;
	}

	const FileTreeNode* space_fossils::core::FileTreeNode::GetParent() const
	{
		return parent;
	}

	const std::vector<std::unique_ptr<FileTreeNode>>& space_fossils::core::FileTreeNode::GetChildren() const
	{
		return children;
	}

	FileTreeNode& space_fossils::core::FileTreeNode::AddDirectory(std::string name)
	{
		assert(type == FileTreeNodeType::Directory);

		auto child = std::make_unique<FileTreeNode>(std::move(name), FileTreeNodeType::Directory, 0);
		child->parent = this;

		FileTreeNode& childRef = *child;
		children.push_back(std::move(child));

		SetRecursiveDirty();

		return childRef;
	}

	FileTreeNode& space_fossils::core::FileTreeNode::AddFile(std::string name, std::uint64_t size)
	{
		assert(type == FileTreeNodeType::Directory);

		auto child = std::make_unique<FileTreeNode>(std::move(name), FileTreeNodeType::File, size);
		child->parent = this;

		FileTreeNode& childRef = *child;
		children.push_back(std::move(child));

		SetRecursiveDirty();

		return childRef;
	}

	void space_fossils::core::FileTreeNode::SetRecursiveDirty()
	{
		isDirty = true;
		if (parent != nullptr) {
			parent->SetRecursiveDirty();
		}
	}

	bool space_fossils::core::FileTreeNode::IsDirty() const
	{
		return isDirty;
	}

	std::uint64_t space_fossils::core::FileTreeNode::RecalculateSizeRecursive()
	{
		if (!isDirty) {
			return size;
		}

		if (type == FileTreeNodeType::File) {
			isDirty = false;
			return size;
		}

		std::uint64_t totalSize = 0;

		for (const auto& child : children) {
			totalSize += child->RecalculateSizeRecursive();
		}

		size = totalSize;
		isDirty = false;

		return size;
	}
}