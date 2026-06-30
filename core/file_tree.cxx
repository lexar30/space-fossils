#include "space_fossils/file_tree.hxx"
#include "space_fossils/file_tree_node.hxx"

#include <utility>

namespace space_fossils::core {

	FileTree::FileTree() = default;
	FileTree::~FileTree() = default;

	FileTree::FileTree(FileTree&& other) noexcept
	{
		root = std::move(other.root);

		IncrementStructureRevision();
		other.IncrementStructureRevision();
	}

	FileTree& FileTree::operator=(FileTree&& other) noexcept
	{
		if (this == &other) {
			return *this;
		}

		root = std::move(other.root);

		IncrementStructureRevision();
		other.IncrementStructureRevision();

		return *this;
	}

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

	std::uintmax_t FileTree::GetSize() const
	{
		return root == nullptr ? 0 : root->GetSize();
	}

	void FileTree::Clear()
	{
		root.reset();

		IncrementStructureRevision();
	}

	std::uintmax_t FileTree::RecalculateSizeRecursive()
	{
		return root == nullptr ? 0 : root->RecalculateSizeRecursive();
	}

	bool FileTree::IsDirty() const
	{
		return root != nullptr && root->IsDirty();
	}

	FileTreeNode& FileTree::CreateRootFile(std::string name, std::uintmax_t size)
	{
		root = std::make_unique<FileTreeNode>(
			  std::move(name)
			, FileTreeNode::FileTreeNodeType::File
			, size
		);

		IncrementStructureRevision();

		return *root;
	}

	FileTreeNode& FileTree::CreateRootDirectory(std::string name)
	{
		root = std::make_unique<FileTreeNode>(
			  std::move(name)
			, FileTreeNode::FileTreeNodeType::Directory
			, 0
		);

		IncrementStructureRevision();

		return *root;
	}

	std::uint64_t FileTree::GetStructureRevision() const
	{
		return structureRevision;
	}

	void FileTree::IncrementStructureRevision()
	{
		++structureRevision;
	}
}
