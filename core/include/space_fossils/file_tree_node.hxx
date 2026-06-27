#pragma once

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
		FileTreeNode(std::string name, FileTreeNodeType type, std::uint64_t size);

		FileTreeNode(const FileTreeNode&) = delete;
		FileTreeNode& operator=(const FileTreeNode&) = delete;

		FileTreeNode(FileTreeNode&&) = delete;
		FileTreeNode& operator=(FileTreeNode&&) = delete;


		FileTreeNodeType GetType() const;

		std::string_view GetName() const;
		std::uint64_t GetSize() const;

		FileTreeNode* GetParent();
		const FileTreeNode* GetParent() const;

		const std::vector<std::unique_ptr<FileTreeNode>>& GetChildren() const;
		
		FileTreeNode& AddDirectory(std::string name);
		FileTreeNode& AddFile(std::string name, std::uint64_t size);

		std::uint64_t RecalculateSizeRecursive();
		bool IsDirty() const;

	private:
		void SetRecursiveDirty();

	private:
		FileTreeNodeType type = FileTreeNodeType::File;
		std::string name;
		std::uint64_t size = 0;

		FileTreeNode* parent = nullptr;
		std::vector<std::unique_ptr<FileTreeNode>> children;

		bool isDirty = true;
	};
}
