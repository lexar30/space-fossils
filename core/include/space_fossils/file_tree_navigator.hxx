#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace space_fossils::core {

	class FileTree;
	class FileTreeNode;

	class FileTreeNavigator
	{
	public:
		FileTreeNavigator(const FileTree& fileTree);

		bool IsValid() const;

		const FileTreeNode* GetCurrentNode() const;

		bool GoToRoot();

		bool CanGoToParent() const;
		bool GoToParent();

		std::size_t GetChildCount() const;

		const FileTreeNode* GetChild(std::size_t index) const;
		const FileTreeNode* GetChild(std::string_view name) const;

		bool GoToChild(std::size_t index);
		bool GoToChild(std::string_view name);

	private:
		bool SetCurrentNode(const FileTreeNode* node);

	private:
		const FileTree* fileTree = nullptr;
		const FileTreeNode* currentFileTreeNode = nullptr;
		std::uint64_t observedStructureRevision = -1;
	};
}
