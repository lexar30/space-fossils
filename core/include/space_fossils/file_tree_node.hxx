#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace space_fossils::core {

	class FileTreeNode
	{
	public:
		enum class FileTreeNodeType {
			  File
			, Directory
		};

	public:
		FileTreeNode(std::string name, FileTreeNodeType type, std::uintmax_t size);

		FileTreeNode(const FileTreeNode&) = delete;
		FileTreeNode& operator=(const FileTreeNode&) = delete;

		FileTreeNode(FileTreeNode&&) = delete;
		FileTreeNode& operator=(FileTreeNode&&) = delete;


		FileTreeNodeType GetType() const;

		std::string_view GetName() const;
		std::uintmax_t GetSize() const;

		FileTreeNode* GetParent();
		const FileTreeNode* GetParent() const;

		const std::vector<std::unique_ptr<FileTreeNode>>& GetChildren() const;
		
		FileTreeNode& AddDirectory(std::string name);
		FileTreeNode& AddFile(std::string name, std::uintmax_t size);

		std::uintmax_t RecalculateSizeRecursive();
		bool IsDirty() const;

		std::size_t GetChildCount() const;
		bool HasChildren() const;
		bool IsFile() const;
		bool IsDirectory() const;
		const FileTreeNode* GetChild(std::size_t index) const;

	private:
		void SetRecursiveDirty();

	private:
		FileTreeNodeType type = FileTreeNodeType::File;
		std::string name;
		std::uintmax_t size = 0;

		FileTreeNode* parent = nullptr;
		std::vector<std::unique_ptr<FileTreeNode>> children;

		bool isDirty = true;
	};
}
