#include "space_fossils/file_tree_navigator.hxx"
#include "space_fossils/file_tree.hxx"
#include "space_fossils/file_tree_node.hxx"

#include <string_view>

namespace space_fossils::core {

	FileTreeNavigator::FileTreeNavigator(const FileTree& fileTree)
		: fileTree(&fileTree)
		, currentFileTreeNode(fileTree.GetRoot())
		, observedStructureRevision(fileTree.GetStructureRevision())
	{ }

	bool FileTreeNavigator::IsValid() const
	{
		return fileTree != nullptr && observedStructureRevision == fileTree->GetStructureRevision() && currentFileTreeNode != nullptr;
	}

	const FileTreeNode* FileTreeNavigator::GetCurrentNode() const
	{
		return IsValid() ? currentFileTreeNode : nullptr;
	}

	bool FileTreeNavigator::GoToRoot()
	{
		if (!IsValid()) {
			return false;
		}

		const FileTreeNode* rootNode = fileTree->GetRoot();
		if (currentFileTreeNode == rootNode) {
			return false;
		}

		currentFileTreeNode = rootNode;

		return true;
	}

	bool FileTreeNavigator::CanGoToParent() const
	{
		if (!IsValid()) {
			return false;
		}

		return currentFileTreeNode->GetParent() != nullptr;
	}

	bool FileTreeNavigator::GoToParent()
	{
		if (!IsValid()) {
			return false;
		}

		const FileTreeNode* parent = currentFileTreeNode->GetParent();
		if (parent == nullptr) {
			return false;
		}

		currentFileTreeNode = parent;

		return true;
	}

	std::size_t FileTreeNavigator::GetChildCount() const
	{
		if (!IsValid()) {
			return 0;
		}

		return currentFileTreeNode->GetChildCount();
	}

	const FileTreeNode* FileTreeNavigator::GetChild(std::size_t index) const
	{
		if (!IsValid()) {
			return nullptr;
		}

		const FileTreeNode* child = currentFileTreeNode->GetChild(index);
		return child;
	}

	const FileTreeNode* FileTreeNavigator::GetChild(std::string_view name) const
	{
		if (!IsValid()) {
			return nullptr;
		}

		const FileTreeNode* child = currentFileTreeNode->GetChild(name);
		return child;
	}

	bool FileTreeNavigator::SetCurrentNode(const FileTreeNode* node)
	{
		if (node == nullptr) {
			return false;
		}

		currentFileTreeNode = node;

		return true;
	}

	bool FileTreeNavigator::GoToChild(std::size_t index)
	{
		if (!IsValid()) {
			return false;
		}

		const FileTreeNode* child = GetChild(index);
		return SetCurrentNode(child);
	}

	bool FileTreeNavigator::GoToChild(std::string_view name)
	{
		if (!IsValid()) {
			return false;
		}

		const FileTreeNode* child = GetChild(name);
		return SetCurrentNode(child);
	}
}
