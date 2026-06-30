#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace space_fossils::core {

	class FileTreeNode;

	class FileTree
	{
	public:
		FileTree();
		~FileTree();

		FileTree(const FileTree&) = delete;
		FileTree& operator=(const FileTree&) = delete;

		FileTree(FileTree&& other) noexcept;
		FileTree& operator=(FileTree&& other) noexcept;

		FileTreeNode* GetRoot();
		const FileTreeNode* GetRoot() const;
		
		bool IsEmpty() const;
		std::uintmax_t GetSize() const;

		FileTreeNode& CreateRootFile(std::string name, std::uintmax_t size);
		FileTreeNode& CreateRootDirectory(std::string name);

		std::uintmax_t RecalculateSizeRecursive();

		bool IsDirty() const;

		void Clear();

		std::uint64_t GetStructureRevision() const;

	private:
		void IncrementStructureRevision();

	private:
		std::unique_ptr<FileTreeNode> root;

		// Increment whenever an operation can invalidate existing FileTreeNode pointers
		std::uint64_t structureRevision = 0;
	};
}
