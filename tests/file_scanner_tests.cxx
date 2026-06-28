#include "space_fossils/file_scanner.hxx"
#include "space_fossils/file_tree.hxx"
#include "space_fossils/file_tree_node.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string_view>


namespace space_fossils::tests {

	using space_fossils::core::FileTree;
	using space_fossils::core::FileTreeNode;
	using FileTreeNodeType = FileTreeNode::FileTreeNodeType;

	const FileTreeNode* FindChildByName(const FileTreeNode& parent, std::string_view name)
	{
		auto it = std::find_if(
			  parent.GetChildren().begin()
			, parent.GetChildren().end()
			, [name](const std::unique_ptr<FileTreeNode>& child) 
			{
				return child->GetName() == name;
			}
		);

		return it == parent.GetChildren().end() ? nullptr : it->get();
	}

	bool AreNodesEqual(const FileTreeNode& left, const FileTreeNode& right)
	{
		if (left.GetType() != right.GetType()) {
			return false;
		}

		if (left.GetName() != right.GetName()) {
			return false;
		}

		if (left.GetSize() != right.GetSize()) {
			return false;
		}

		if(left.GetChildren().size() != right.GetChildren().size()) {
			return false;
		}

		for (const auto& lChild : left.GetChildren()) {
			const auto& rChild = FindChildByName(right, lChild->GetName());

			if (rChild == nullptr) {
				return false;
			}

			if (!AreNodesEqual(*lChild, *rChild)) {
				return false;
			}
		}

		return true;
	}

	bool AreTreesEqual(const FileTree& left, const FileTree& right)
	{
		const FileTreeNode* leftRoot = left.GetRoot();
		const FileTreeNode* rightRoot = right.GetRoot();

		if (leftRoot == nullptr && rightRoot == nullptr) {
			return true;
		}

		if (leftRoot == nullptr || rightRoot == nullptr) {
			return false;
		}

		return AreNodesEqual(*left.GetRoot(), *right.GetRoot());
	}

	SF_TEST(file_scanner, TestHelperFuncs_EqualTrees)
	{
		{
			FileTree fileTreeExpected;

			{
				FileTreeNode& root = fileTreeExpected.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			FileTree fileTreeActual;

			{
				FileTreeNode& root = fileTreeActual.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			fileTreeExpected.RecalculateSizeRecursive();
			fileTreeActual.RecalculateSizeRecursive();

			const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
			SF_ASSERT_EQ(areTreesEqual, true);
		}
	}

	SF_TEST(file_scanner, TestHelperFuncs_SubFileHasDifferentName)
	{
		{
			FileTree fileTreeExpected;

			{
				FileTreeNode& root = fileTreeExpected.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("NO.txt", 200);
			}

			FileTree fileTreeActual;

			{
				FileTreeNode& root = fileTreeActual.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			fileTreeExpected.RecalculateSizeRecursive();
			fileTreeActual.RecalculateSizeRecursive();

			const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
			SF_ASSERT_EQ(areTreesEqual, false);
		}
	}

	SF_TEST(file_scanner, TestHelperFuncs_SubDirHasDifferentName_1)
	{
		{
			FileTree fileTreeExpected;

			{
				FileTreeNode& root = fileTreeExpected.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("NO");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			FileTree fileTreeActual;

			{
				FileTreeNode& root = fileTreeActual.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			fileTreeExpected.RecalculateSizeRecursive();
			fileTreeActual.RecalculateSizeRecursive();

			const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
			SF_ASSERT_EQ(areTreesEqual, false);
		}
	}

	SF_TEST(file_scanner, TestHelperFuncs_SubDirHasDifferentName_2)
	{
		{
			FileTree fileTreeExpected;

			{
				FileTreeNode& root = fileTreeExpected.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			FileTree fileTreeActual;

			{
				FileTreeNode& root = fileTreeActual.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("NO");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			fileTreeExpected.RecalculateSizeRecursive();
			fileTreeActual.RecalculateSizeRecursive();

			const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
			SF_ASSERT_EQ(areTreesEqual, false);
		}
	}

	SF_TEST(file_scanner, TestHelperFuncs_FileHasDifferentSize)
	{
		{
			FileTree fileTreeExpected;

			{
				FileTreeNode& root = fileTreeExpected.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 111);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			FileTree fileTreeActual;

			{
				FileTreeNode& root = fileTreeActual.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}

			fileTreeExpected.RecalculateSizeRecursive();
			fileTreeActual.RecalculateSizeRecursive();

			const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
			SF_ASSERT_EQ(areTreesEqual, false);
		}
	}

	SF_TEST(file_scanner, TestHelperFuncs_EmptyFileTree)
	{
		{
			FileTree fileTreeExpected;

			{
				FileTreeNode& root = fileTreeExpected.CreateRootDirectory("root");

				FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

				FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
				FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
				FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


				FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
				FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
				FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
			}
			
			FileTree fileTreeActual;

			fileTreeExpected.RecalculateSizeRecursive();

			const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
			SF_ASSERT_EQ(areTreesEqual, false);
		}
	}

	SF_TEST(file_scanner, ScanDirectory)
	{
		using space_fossils::core::FileScanner;

		FileTree fileTreeExpected;

		{
			FileTreeNode& root = fileTreeExpected.CreateRootDirectory("root");

			FileTreeNode& rootFile1 = root.AddFile("test_name_1.txt", 100);

			FileTreeNode& subDirectory1 = root.AddDirectory("sub_directory_1");
			FileTreeNode& file2 = subDirectory1.AddFile("test_name_2.txt", 100);
			FileTreeNode& file3 = subDirectory1.AddFile("test_name_3.txt", 200);


			FileTreeNode& subDirectory2 = root.AddDirectory("sub_directory_2");
			FileTreeNode& subDirectory3 = subDirectory2.AddDirectory("sub_directory_3");
			FileTreeNode& file4 = subDirectory3.AddFile("test_name_4.txt", 200);
		}
			
		FileScanner fs;

		const std::filesystem::path fixtureRoot{ SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT };

		FileTree fileTreeActual(std::move(fs.Scan(fixtureRoot)));

		fileTreeExpected.RecalculateSizeRecursive();

		const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
		SF_ASSERT_EQ(areTreesEqual, true);
	}

	SF_TEST(file_scanner, SingleFile)
	{
		using space_fossils::core::FileScanner;

		FileTree fileTreeExpected;

		FileTreeNode& rootFile1 = fileTreeExpected.CreateRootFile("test_file.txt", 123);
			
		FileScanner fs;

		const std::filesystem::path fixtureRoot{ SPACE_FOSSILS_FILE_SCANNER_FIXTURE_TEST_FILE };

		FileTree fileTreeActual(std::move(fs.Scan(fixtureRoot)));

		fileTreeExpected.RecalculateSizeRecursive();

		const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
		SF_ASSERT_EQ(areTreesEqual, true);
	}

	SF_TEST(file_scanner, EmptyDirectory)
	{
		using space_fossils::core::FileScanner;

		FileTree fileTreeExpected;

		{
			FileTreeNode& root = fileTreeExpected.CreateRootDirectory("root");
		}

		FileScanner fs;

		const std::filesystem::path fixtureRoot{ SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT_EMPTY };

		FileTree fileTreeActual(std::move(fs.Scan(fixtureRoot)));

		fileTreeExpected.RecalculateSizeRecursive();

		const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
		SF_ASSERT_EQ(areTreesEqual, true);
	}

	SF_TEST(file_scanner, NonExistentPath)
	{
		using space_fossils::core::FileScanner;

		FileTree fileTreeExpected;

		FileScanner fs;

		const std::filesystem::path fixtureRoot{ SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH };

		FileTree fileTreeActual(std::move(fs.Scan(fixtureRoot)));

		fileTreeExpected.RecalculateSizeRecursive();

		const bool areTreesEqual = AreTreesEqual(fileTreeExpected, fileTreeActual);
		SF_ASSERT_EQ(areTreesEqual, true);
	}
}