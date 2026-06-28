#include "space_fossils/file_tree_node.hxx"

#include <cassert>
#include <utility>

namespace space_fossils::core {
	FileTreeNode::FileTreeNode(std::string name, FileTreeNodeType type, std::uintmax_t size)
		: type(type)
		, name(std::move(name))
		, size(size)
	{ }

	FileTreeNode::FileTreeNodeType FileTreeNode::GetType() const
	{
		return type;
	}

	std::string_view FileTreeNode::GetName() const
	{
		return name;
	}

	std::uintmax_t FileTreeNode::GetSize() const
	{
		return size;
	}

	FileTreeNode* FileTreeNode::GetParent()
	{
		return parent;
	}

	const FileTreeNode* FileTreeNode::GetParent() const
	{
		return parent;
	}

	const std::vector<std::unique_ptr<FileTreeNode>>& FileTreeNode::GetChildren() const
	{
		return children;
	}

	FileTreeNode& FileTreeNode::AddDirectory(std::string name)
	{
		assert(type == FileTreeNodeType::Directory);

		auto child = std::make_unique<FileTreeNode>(std::move(name), FileTreeNodeType::Directory, 0);
		child->parent = this;

		FileTreeNode& childRef = *child;
		children.push_back(std::move(child));

		SetRecursiveDirty();

		return childRef;
	}

	FileTreeNode& FileTreeNode::AddFile(std::string name, std::uintmax_t size)
	{
		assert(type == FileTreeNodeType::Directory);

		auto child = std::make_unique<FileTreeNode>(std::move(name), FileTreeNodeType::File, size);
		child->parent = this;

		FileTreeNode& childRef = *child;
		children.push_back(std::move(child));

		SetRecursiveDirty();

		return childRef;
	}

	void FileTreeNode::SetRecursiveDirty()
	{
		isDirty = true;
		if (parent != nullptr) {
			parent->SetRecursiveDirty();
		}
	}

	bool FileTreeNode::IsDirty() const
	{
		return isDirty;
	}

	std::uintmax_t FileTreeNode::RecalculateSizeRecursive()
	{
		if (!isDirty) {
			return size;
		}

		if (type == FileTreeNodeType::File) {
			isDirty = false;
			return size;
		}

		std::uintmax_t totalSize = 0;

		for (const auto& child : children) {
			totalSize += child->RecalculateSizeRecursive();
		}

		size = totalSize;
		isDirty = false;

		return size;
	}

	std::size_t FileTreeNode::GetChildCount() const
	{
		return children.size();
	}

	bool FileTreeNode::HasChildren() const
	{
		return !children.empty();
	}


	bool FileTreeNode::IsFile() const
	{
		return type == FileTreeNodeType::File;
	}


	bool FileTreeNode::IsDirectory() const
	{
		return type == FileTreeNodeType::Directory;
	}


	const FileTreeNode* FileTreeNode::GetChild(std::size_t index) const
	{
		if (index >= children.size()) {
			return nullptr;
		}

		return children[index].get();
	}






}
