#include "space_fossils/file_tree_node.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>

namespace space_fossils::tests {

	SF_TEST(file_tree_node, StoringFileDataTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode file("test_name.txt", FileTreeNodeType::File, 150);

		SF_ASSERT_EQ(file.GetType(), FileTreeNodeType::File);
		SF_ASSERT_EQ(file.GetName(), "test_name.txt");
		SF_ASSERT_EQ(file.GetSize(), 150);
		SF_ASSERT_EQ(file.GetParent(), nullptr);
		SF_ASSERT_EQ(file.GetChildren().empty(), true);
	}

	SF_TEST(file_tree_node, EmptyDirectoryTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode directory("test_directory_name", FileTreeNodeType::Directory, 0);

		SF_ASSERT_EQ(directory.GetType(), FileTreeNodeType::Directory);
		SF_ASSERT_EQ(directory.GetName(), "test_directory_name");
		SF_ASSERT_EQ(directory.GetSize(), 0);
		SF_ASSERT_EQ(directory.GetParent(), nullptr);
		SF_ASSERT_EQ(directory.GetChildren().empty(), true);

		directory.RecalculateSizeRecursive();

		SF_ASSERT_EQ(directory.GetSize(), 0);
	}

	SF_TEST(file_tree_node, AddFileToDirectoryTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode directory("test_directory_name", FileTreeNodeType::Directory, 0);

		FileTreeNode& file = directory.AddFile("test_name_1.txt", 150);

		SF_ASSERT_EQ(directory.GetChildren().size(), 1);

		SF_ASSERT_EQ(file.GetType(), FileTreeNodeType::File);
		SF_ASSERT_EQ(file.GetName(), "test_name_1.txt");
		SF_ASSERT_EQ(file.GetSize(), 150);
		SF_ASSERT_EQ(file.GetParent(), &directory);
		SF_ASSERT_EQ(file.GetChildren().empty(), true);

		SF_ASSERT_EQ(directory.GetParent(), nullptr);
	}

	SF_TEST(file_tree_node, AddDirectoryToDirectoryTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode root("root", FileTreeNodeType::Directory, 0);

		FileTreeNode& childDirectory = root.AddDirectory("child_directory");

		SF_ASSERT_EQ(root.GetChildren().size(), 1);

		SF_ASSERT_EQ(childDirectory.GetType(), FileTreeNodeType::Directory);
		SF_ASSERT_EQ(childDirectory.GetName(), "child_directory");
		SF_ASSERT_EQ(childDirectory.GetSize(), 0);
		SF_ASSERT_EQ(childDirectory.GetParent(), &root);
		SF_ASSERT_EQ(childDirectory.GetChildren().empty(), true);
	}

	SF_TEST(file_tree_node, DirectorySizeIsSumOfChildrenTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode directory("test_directory_name", FileTreeNodeType::Directory, 0);

		directory.AddFile("test_name_1.txt", 150);
		directory.AddFile("test_name_2.txt", 350);

		directory.RecalculateSizeRecursive();

		SF_ASSERT_EQ(directory.GetSize(), 500);
	}

	SF_TEST(file_tree_node, NestedDirectorySizeTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode root("root", FileTreeNodeType::Directory, 0);

		FileTreeNode& subDirectory = root.AddDirectory("sub_directory");
		FileTreeNode& file1 = root.AddFile("test_name_1.txt", 200);
		FileTreeNode& file2 = subDirectory.AddFile("test_name_2.txt", 400);

		root.RecalculateSizeRecursive();

		SF_ASSERT_EQ(root.GetSize(), 600);
		SF_ASSERT_EQ(subDirectory.GetSize(), 400);

		SF_ASSERT_EQ(subDirectory.GetParent(), &root);
		SF_ASSERT_EQ(file1.GetParent(), &root);
		SF_ASSERT_EQ(file2.GetParent(), &subDirectory);
	}

	SF_TEST(file_tree_node, RecalculateUpdatesStaleDirectorySizeTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode root("root", FileTreeNodeType::Directory, 0);

		FileTreeNode& subDirectory = root.AddDirectory("sub_directory");
		subDirectory.AddFile("test_name_1.txt", 200);

		root.RecalculateSizeRecursive();

		SF_ASSERT_EQ(root.GetSize(), 200);
		SF_ASSERT_EQ(subDirectory.GetSize(), 200);

		subDirectory.AddFile("test_name_2.txt", 700);

		root.RecalculateSizeRecursive();

		SF_ASSERT_EQ(root.GetSize(), 900);
		SF_ASSERT_EQ(subDirectory.GetSize(), 900);
	}

	SF_TEST(file_tree_node, RecalculateClearsDirtyFlagTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode root("root", FileTreeNodeType::Directory, 0);
		FileTreeNode& subDirectory = root.AddDirectory("sub_directory");
		FileTreeNode& file = subDirectory.AddFile("test_name_1.txt", 200);

		SF_ASSERT_EQ(root.IsDirty(), true);
		SF_ASSERT_EQ(subDirectory.IsDirty(), true);
		SF_ASSERT_EQ(file.IsDirty(), true);

		root.RecalculateSizeRecursive();

		SF_ASSERT_EQ(root.IsDirty(), false);
		SF_ASSERT_EQ(subDirectory.IsDirty(), false);
		SF_ASSERT_EQ(file.IsDirty(), false);
	}

	SF_TEST(file_tree_node, RecalculateCleanTreeKeepsSizeTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode root("root", FileTreeNodeType::Directory, 0);
		FileTreeNode& subDirectory = root.AddDirectory("sub_directory");
		subDirectory.AddFile("test_name_1.txt", 200);
		subDirectory.AddFile("test_name_2.txt", 700);

		root.RecalculateSizeRecursive();

		const std::uintmax_t rootSize = root.GetSize();
		const std::uintmax_t subDirectorySize = subDirectory.GetSize();

		root.RecalculateSizeRecursive();

		SF_ASSERT_EQ(root.GetSize(), rootSize);
		SF_ASSERT_EQ(subDirectory.GetSize(), subDirectorySize);
		SF_ASSERT_EQ(root.GetSize(), 900);
		SF_ASSERT_EQ(subDirectory.GetSize(), 900);
		SF_ASSERT_EQ(root.IsDirty(), false);
		SF_ASSERT_EQ(subDirectory.IsDirty(), false);
	}

	SF_TEST(file_tree_node, AddFileAndDirectoryMarkParentChainDirtyTests)
	{
		using space_fossils::core::FileTreeNode;
		using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

		FileTreeNode root("root", FileTreeNodeType::Directory, 0);
		FileTreeNode& subDirectory = root.AddDirectory("sub_directory");
		FileTreeNode& nestedDirectory = subDirectory.AddDirectory("nested_directory");

		root.RecalculateSizeRecursive();

		SF_ASSERT_EQ(root.IsDirty(), false);
		SF_ASSERT_EQ(subDirectory.IsDirty(), false);
		SF_ASSERT_EQ(nestedDirectory.IsDirty(), false);

		nestedDirectory.AddFile("test_name_1.txt", 200);

		SF_ASSERT_EQ(root.IsDirty(), true);
		SF_ASSERT_EQ(subDirectory.IsDirty(), true);
		SF_ASSERT_EQ(nestedDirectory.IsDirty(), true);

		root.RecalculateSizeRecursive();

		SF_ASSERT_EQ(root.IsDirty(), false);
		SF_ASSERT_EQ(subDirectory.IsDirty(), false);
		SF_ASSERT_EQ(nestedDirectory.IsDirty(), false);

		nestedDirectory.AddDirectory("child_directory");

		SF_ASSERT_EQ(root.IsDirty(), true);
		SF_ASSERT_EQ(subDirectory.IsDirty(), true);
		SF_ASSERT_EQ(nestedDirectory.IsDirty(), true);
	}
}
