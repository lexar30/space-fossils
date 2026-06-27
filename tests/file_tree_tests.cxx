#include "space_fossils/file_tree.hxx"
#include "space_fossils/file_tree_node.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>

namespace space_fossils::tests {

	SF_TEST(file_tree, EmptyTreeTests)
	{
		using space_fossils::core::FileTree;

		FileTree fileTree;

		SF_ASSERT_EQ(fileTree.IsEmpty(), true);
		SF_ASSERT_EQ(fileTree.GetSize(), 0);
		SF_ASSERT_EQ(fileTree.GetRoot(), nullptr);

		SF_ASSERT_EQ(fileTree.RecalculateSizeRecursive(), 0);

		SF_ASSERT_EQ(fileTree.IsEmpty(), true);
		SF_ASSERT_EQ(fileTree.GetSize(), 0);
		SF_ASSERT_EQ(fileTree.GetRoot(), nullptr);
	}

	SF_TEST(file_tree, CreateRootDirectoryTests)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTree fileTree;

		FileTreeNode& root = fileTree.CreateRootDirectory("root");

		SF_ASSERT_EQ(fileTree.IsEmpty(), false);
		SF_ASSERT_EQ(fileTree.GetRoot(), &root);
		SF_ASSERT_EQ(fileTree.GetSize(), 0);

		SF_ASSERT_EQ(root.GetType(), FileTreeNodeType::Directory);
		SF_ASSERT_EQ(root.GetName(), "root");
		SF_ASSERT_EQ(root.GetSize(), 0);
		SF_ASSERT_EQ(root.GetParent(), nullptr);
		SF_ASSERT_EQ(root.GetChildren().empty(), true);
	}

	SF_TEST(file_tree, CreateRootFileTests)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTree fileTree;

		FileTreeNode& root = fileTree.CreateRootFile("root.txt", 200);

		SF_ASSERT_EQ(fileTree.IsEmpty(), false);
		SF_ASSERT_EQ(fileTree.GetRoot(), &root);
		SF_ASSERT_EQ(fileTree.GetSize(), 200);

		SF_ASSERT_EQ(root.GetType(), FileTreeNodeType::File);
		SF_ASSERT_EQ(root.GetName(), "root.txt");
		SF_ASSERT_EQ(root.GetSize(), 200);
		SF_ASSERT_EQ(root.GetParent(), nullptr);
		SF_ASSERT_EQ(root.GetChildren().empty(), true);
	}

	SF_TEST(file_tree, ClearTreeTests)
	{
		using space_fossils::core::FileTree;

		FileTree fileTree;

		fileTree.CreateRootFile("root.txt", 200);

		SF_ASSERT_EQ(fileTree.IsEmpty(), false);
		SF_ASSERT_EQ(fileTree.GetSize(), 200);
		SF_ASSERT_EQ(fileTree.GetRoot() == nullptr, false);

		fileTree.Clear();

		SF_ASSERT_EQ(fileTree.IsEmpty(), true);
		SF_ASSERT_EQ(fileTree.GetSize(), 0);
		SF_ASSERT_EQ(fileTree.GetRoot(), nullptr);

		SF_ASSERT_EQ(fileTree.RecalculateSizeRecursive(), 0);
	}

	SF_TEST(file_tree, RecalculateRootDirectorySizeTests)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;

		FileTree fileTree;

		FileTreeNode& root = fileTree.CreateRootDirectory("root");
		FileTreeNode& subDirectory = root.AddDirectory("sub_directory");

		subDirectory.AddFile("test_name_1.txt", 200);
		subDirectory.AddFile("test_name_2.txt", 700);

		SF_ASSERT_EQ(fileTree.RecalculateSizeRecursive(), 900);
		SF_ASSERT_EQ(fileTree.GetSize(), 900);
		SF_ASSERT_EQ(root.GetSize(), 900);
		SF_ASSERT_EQ(subDirectory.GetSize(), 900);
	}

	SF_TEST(file_tree, ReplaceRootTests)
	{
		using space_fossils::core::FileTree;
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTree fileTree;

		FileTreeNode& firstRoot = fileTree.CreateRootDirectory("first_root");
		firstRoot.AddFile("a.txt", 200);

		fileTree.RecalculateSizeRecursive();

		SF_ASSERT_EQ(fileTree.GetSize(), 200);
		SF_ASSERT_EQ(fileTree.GetRoot(), &firstRoot);

		FileTreeNode& secondRoot = fileTree.CreateRootFile("second_root.txt", 700);

		SF_ASSERT_EQ(fileTree.GetSize(), 700);
		SF_ASSERT_EQ(fileTree.GetRoot(), &secondRoot);

		SF_ASSERT_EQ(secondRoot.GetType(), FileTreeNodeType::File);
		SF_ASSERT_EQ(secondRoot.GetName(), "second_root.txt");
		SF_ASSERT_EQ(secondRoot.GetParent(), nullptr);
	}
}