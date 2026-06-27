#include "space_fossils/file_tree.hxx"
#include "space_fossils/file_tree_node.hxx"

#include <utility>

namespace space_fossils::core {

	FileTree::FileTree() = default;
	FileTree::~FileTree() = default;
	FileTree::FileTree(FileTree&& other) noexcept = default;
	FileTree& FileTree::operator=(FileTree&& other) noexcept = default;

	FileTreeNode* FileTree::GetRoot()
	{
		return root.get();
	}

	const FileTreeNode* FileTree::GetRoot() const
	{
		return root.get();
	}

	bool FileTree::IsEmpty() const
	{
		return root == nullptr;
	}

	std::uint64_t FileTree::GetSize() const
	{
		return root == nullptr ? 0 : root->GetSize();
	}

	void FileTree::Clear()
	{
		root.reset();
	}

	std::uint64_t FileTree::RecalculateSizeRecursive()
	{
		return root == nullptr ? 0 : root->RecalculateSizeRecursive();
	}

	bool FileTree::IsDirty() const
	{
		return root != nullptr && root->IsDirty();
	}

	FileTreeNode& FileTree::CreateRootFile(std::string name, std::uint64_t size)
	{
		root = std::make_unique<FileTreeNode>(
			  std::move(name)
			, FileTreeNode::FileTreeNodeType::File
			, size
		);

		return *root;
	}

	FileTreeNode& FileTree::CreateRootDirectory(std::string name)
	{
		root = std::make_unique<FileTreeNode>(
			  std::move(name)
			, FileTreeNode::FileTreeNodeType::Directory
			, 0
		);

		return *root;
	}
}