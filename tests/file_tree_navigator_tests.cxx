#include "space_fossils/file_tree_navigator.hxx"
#include "space_fossils/file_tree.hxx"
#include "space_fossils/file_tree_node.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>
#include <utility>

namespace space_fossils::tests {

	SF_TEST(file_tree_navigator, InitializationByDirectory)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;
		using space_fossils::core::FileTreeNavigator;

		FileTree fileTree;

		FileTreeNode& firstRoot = fileTree.CreateRootDirectory("root");

		FileTreeNavigator navigator(fileTree);

		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), false);
		SF_ASSERT_EQ(navigator.GetChildCount(), 0);
		SF_ASSERT_EQ(navigator.GoToParent(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "root", true);

		SF_ASSERT_EQ(navigator.GoToRoot(), false);
		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), false);
		SF_ASSERT_EQ(navigator.GetChildCount(), 0);
		SF_ASSERT_EQ(navigator.GoToParent(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "root", true);
	}

	SF_TEST(file_tree_navigator, InitializationByFile)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;
		using space_fossils::core::FileTreeNavigator;

		FileTree fileTree;

		FileTreeNode& root = fileTree.CreateRootFile("root_file", 100);

		FileTreeNavigator navigator(fileTree);

		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), false);
		SF_ASSERT_EQ(navigator.GetChildCount(), 0);
		SF_ASSERT_EQ(navigator.GoToParent(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "root_file", true);

		SF_ASSERT_EQ(navigator.GoToRoot(), false);
		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), false);
		SF_ASSERT_EQ(navigator.GetChildCount(), 0);
		SF_ASSERT_EQ(navigator.GoToParent(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "root_file", true);
	}

	SF_TEST(file_tree_navigator, GoToChildAndParent)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;
		using space_fossils::core::FileTreeNavigator;

		FileTree fileTree;

		FileTreeNode& root = fileTree.CreateRootDirectory("root");
		FileTreeNode& subDirectory = root.AddDirectory("sub_directory");

		subDirectory.AddFile("test_name_1.txt", 200);
		subDirectory.AddFile("test_name_2.txt", 700);

		FileTreeNavigator navigator(fileTree);

		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.GetChildCount(), 1);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "root", true);

		SF_ASSERT_EQ(navigator.GoToChild(0), true);
		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), true);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "sub_directory", true);
		SF_ASSERT_EQ(navigator.GetChildCount(), 2);

		SF_ASSERT_EQ(navigator.GoToChild(0), true);
		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), true);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "test_name_1.txt", true);

		SF_ASSERT_EQ(navigator.GoToParent(), true);
		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), true);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "sub_directory", true);

		SF_ASSERT_EQ(navigator.GoToChild(0), true);
		SF_ASSERT_EQ(navigator.GoToRoot(), true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode()->GetName() == "root", true);

		const FileTreeNode* before = navigator.GetCurrentNode();

		SF_ASSERT_EQ(navigator.GoToChild(999), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode() == before, true);
	}

	SF_TEST(file_tree_navigator, EmptyTree)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;
		using space_fossils::core::FileTreeNavigator;

		FileTree fileTree;

		FileTreeNavigator navigator(fileTree);

		SF_ASSERT_EQ(navigator.IsValid(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode() == nullptr, true);
		SF_ASSERT_EQ(navigator.CanGoToParent(), false);
		SF_ASSERT_EQ(navigator.GetChildCount() == 0, true);
		SF_ASSERT_EQ(navigator.GoToParent(), false);
		SF_ASSERT_EQ(navigator.GoToRoot(), false);
		SF_ASSERT_EQ(navigator.IsValid(), false);
		SF_ASSERT_EQ(navigator.GoToChild(0), false);
	}

	SF_TEST(file_tree_navigator, CheckInvalidTree)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;
		using space_fossils::core::FileTreeNavigator;

		FileTree fileTree;

		FileTreeNode& root = fileTree.CreateRootDirectory("root");
		FileTreeNode& subDirectory = root.AddDirectory("sub_directory");

		subDirectory.AddFile("test_name_1.txt", 200);
		subDirectory.AddFile("test_name_2.txt", 700);

		FileTreeNavigator navigator(fileTree);

		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.GetCurrentNode() == nullptr, false);

		fileTree.CreateRootDirectory("root2");

		SF_ASSERT_EQ(navigator.IsValid(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode() == nullptr, true);
	}

	SF_TEST(file_tree_navigator, InvalidAfterClear)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;
		using space_fossils::core::FileTreeNavigator;

		FileTree fileTree;

		FileTreeNode& root = fileTree.CreateRootDirectory("root");
		root.AddFile("test_name_1.txt", 200);

		FileTreeNavigator navigator(fileTree);

		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.GetCurrentNode() == nullptr, false);

		fileTree.Clear();

		SF_ASSERT_EQ(navigator.IsValid(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode() == nullptr, true);
		SF_ASSERT_EQ(navigator.GetChildCount(), 0);
		SF_ASSERT_EQ(navigator.GoToRoot(), false);
		SF_ASSERT_EQ(navigator.GoToChild(0), false);
	}

	SF_TEST(file_tree_navigator, InvalidAfterMoveConstructionFromSourceTree)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;
		using space_fossils::core::FileTreeNavigator;

		FileTree sourceTree;

		FileTreeNode& root = sourceTree.CreateRootDirectory("root");
		root.AddFile("test_name_1.txt", 200);

		FileTreeNavigator navigator(sourceTree);

		SF_ASSERT_EQ(navigator.IsValid(), true);
		SF_ASSERT_EQ(navigator.GetCurrentNode() == nullptr, false);

		FileTree targetTree(std::move(sourceTree));

		SF_ASSERT_EQ(navigator.IsValid(), false);
		SF_ASSERT_EQ(navigator.GetCurrentNode() == nullptr, true);

		FileTreeNavigator targetNavigator(targetTree);

		SF_ASSERT_EQ(targetNavigator.IsValid(), true);
		SF_ASSERT_EQ(targetNavigator.GetCurrentNode()->GetName() == "root", true);
	}

	SF_TEST(file_tree_navigator, InvalidAfterMoveAssignmentFromSourceAndTargetTrees)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;
		using space_fossils::core::FileTreeNavigator;

		FileTree sourceTree;
		FileTreeNode& sourceRoot = sourceTree.CreateRootDirectory("source_root");
		sourceRoot.AddFile("test_name_1.txt", 200);

		FileTree targetTree;
		FileTreeNode& targetRoot = targetTree.CreateRootDirectory("target_root");
		targetRoot.AddFile("test_name_2.txt", 700);

		FileTreeNavigator sourceNavigator(sourceTree);
		FileTreeNavigator targetNavigator(targetTree);

		SF_ASSERT_EQ(sourceNavigator.IsValid(), true);
		SF_ASSERT_EQ(targetNavigator.IsValid(), true);

		targetTree = std::move(sourceTree);

		SF_ASSERT_EQ(sourceNavigator.IsValid(), false);
		SF_ASSERT_EQ(sourceNavigator.GetCurrentNode() == nullptr, true);
		SF_ASSERT_EQ(targetNavigator.IsValid(), false);
		SF_ASSERT_EQ(targetNavigator.GetCurrentNode() == nullptr, true);

		FileTreeNavigator movedTargetNavigator(targetTree);

		SF_ASSERT_EQ(movedTargetNavigator.IsValid(), true);
		SF_ASSERT_EQ(movedTargetNavigator.GetCurrentNode()->GetName() == "source_root", true);
	}


}
