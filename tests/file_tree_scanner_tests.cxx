#include "space_fossils/file_tree/scanner.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <filesystem>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;

		NativeString MakeNativeString(const char* value)
		{
			NativeString result;
			while (*value != '\0') {
				result.push_back(static_cast<NativeChar>(*value));
				++value;
			}

			return result;
		}

		void AssertNameEquals(const Node& node, const char* expectedValue)
		{
			NativeString expectedName = MakeNativeString(expectedValue);
			NativeStringView actualName = ToStringView(node.name);

			SF_ASSERT_EQ(actualName.size(), expectedName.size());
			for (std::size_t index = 0; index < expectedName.size(); ++index) {
				SF_ASSERT_EQ(actualName[index] == expectedName[index], true);
			}
		}

		bool NameEquals(const Node& node, const char* expectedValue)
		{
			NativeString expectedName = MakeNativeString(expectedValue);
			NativeStringView actualName = ToStringView(node.name);

			if (actualName.size() != expectedName.size()) {
				return false;
			}

			for (std::size_t index = 0; index < expectedName.size(); ++index) {
				if (actualName[index] != expectedName[index]) {
					return false;
				}
			}

			return true;
		}

		std::size_t CountChildren(const Node& parent)
		{
			std::size_t count = 0;
			const Node* child = parent.firstChild;

			while (child != nullptr) {
				++count;
				child = child->nextSibling;
			}

			return count;
		}

		Node* FindChild(Node& parent, const char* name)
		{
			Node* child = parent.firstChild;
			while (child != nullptr) {
				if (NameEquals(*child, name)) {
					return child;
				}

				child = child->nextSibling;
			}

			return nullptr;
		}

		Node* RequireChild(Node& parent, const char* name)
		{
			Node* child = FindChild(parent, name);
			SF_ASSERT_EQ(child != nullptr, true);
			SF_ASSERT_EQ(child->parent == &parent, true);

			return child;
		}

		TreePoolBundle ScanPath(const char* path, std::size_t maxDepth)
		{
			Scanner scanner;

			ScanRequest request;
			request.path = std::filesystem::path(path);
			request.maxDepth = maxDepth;

			return scanner.Scan(request);
		}

		void AssertSingleRootBundle(const TreePoolBundle& bundle)
		{
			SF_ASSERT_EQ(bundle.root != nullptr, true);
			SF_ASSERT_EQ(bundle.createdNodesCount, 1);
			SF_ASSERT_EQ(bundle.root->parent == nullptr, true);
			SF_ASSERT_EQ(bundle.root->firstChild == nullptr, true);
			SF_ASSERT_EQ(bundle.root->nextSibling == nullptr, true);
		}
	}

	SF_TEST(file_tree_scanner, ScanFileCreatesCompleteFileRoot)
	{
		TreePoolBundle bundle = ScanPath(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_TEST_FILE, 1);

		AssertSingleRootBundle(bundle);
		AssertNameEquals(*bundle.root, "test_file.txt");
		SF_ASSERT_EQ(bundle.root->entryType, EntryType::File);
		SF_ASSERT_EQ(bundle.root->entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(bundle.root->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(bundle.root->logicalSize, std::filesystem::file_size(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_TEST_FILE));
	}

	SF_TEST(file_tree_scanner, ScanMissingPathCreatesNotFoundRoot)
	{
		TreePoolBundle bundle = ScanPath(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH, 1);

		AssertSingleRootBundle(bundle);
		AssertNameEquals(*bundle.root, "root");
		SF_ASSERT_EQ(bundle.root->entryType, EntryType::Unknown);
		SF_ASSERT_EQ(bundle.root->entryStatus, EntryStatus::NotFound);
		SF_ASSERT_EQ(bundle.root->scanStatus, EntryScanStatus::Error);
	}

	SF_TEST(file_tree_scanner, ScanEmptyDirectoryCreatesCompleteDirectoryRoot)
	{
		TreePoolBundle bundle = ScanPath(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT_EMPTY, 1);

		AssertSingleRootBundle(bundle);
		AssertNameEquals(*bundle.root, "root");
		SF_ASSERT_EQ(bundle.root->entryType, EntryType::Directory);
		SF_ASSERT_EQ(bundle.root->entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(bundle.root->scanStatus, EntryScanStatus::Complete);
	}

	SF_TEST(file_tree_scanner, MaxDepthZeroLeavesDirectoryPendingWithoutChildren)
	{
		TreePoolBundle bundle = ScanPath(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 0);

		AssertSingleRootBundle(bundle);
		AssertNameEquals(*bundle.root, "root");
		SF_ASSERT_EQ(bundle.root->entryType, EntryType::Directory);
		SF_ASSERT_EQ(bundle.root->entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(bundle.root->scanStatus, EntryScanStatus::Pending);
	}

	SF_TEST(file_tree_scanner, MaxDepthOneAddsDirectChildrenAndMarksNestedDirectoriesPending)
	{
		TreePoolBundle bundle = ScanPath(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 1);
		SF_ASSERT_EQ(bundle.root != nullptr, true);
		Node& root = *bundle.root;

		AssertNameEquals(root, "root");
		SF_ASSERT_EQ(bundle.createdNodesCount, 4);
		SF_ASSERT_EQ(root.entryType, EntryType::Directory);
		SF_ASSERT_EQ(root.entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(root.scanStatus, EntryScanStatus::Partial);
		SF_ASSERT_EQ(CountChildren(root), 3);

		Node* file = RequireChild(root, "test_name_1.txt");
		SF_ASSERT_EQ(file->entryType, EntryType::File);
		SF_ASSERT_EQ(file->entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(file->scanStatus, EntryScanStatus::Complete);

		Node* firstDirectory = RequireChild(root, "sub_directory_1");
		SF_ASSERT_EQ(firstDirectory->entryType, EntryType::Directory);
		SF_ASSERT_EQ(firstDirectory->entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(firstDirectory->scanStatus, EntryScanStatus::Pending);
		SF_ASSERT_EQ(firstDirectory->firstChild == nullptr, true);

		Node* secondDirectory = RequireChild(root, "sub_directory_2");
		SF_ASSERT_EQ(secondDirectory->entryType, EntryType::Directory);
		SF_ASSERT_EQ(secondDirectory->entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(secondDirectory->scanStatus, EntryScanStatus::Pending);
		SF_ASSERT_EQ(secondDirectory->firstChild == nullptr, true);
	}

	SF_TEST(file_tree_scanner, MaxDepthThreeScansWholeFixtureTree)
	{
		TreePoolBundle bundle = ScanPath(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 3);
		SF_ASSERT_EQ(bundle.root != nullptr, true);
		Node& root = *bundle.root;

		SF_ASSERT_EQ(bundle.createdNodesCount, 8);
		SF_ASSERT_EQ(root.entryType, EntryType::Directory);
		SF_ASSERT_EQ(root.entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(root.scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(CountChildren(root), 3);

		Node* rootFile = RequireChild(root, "test_name_1.txt");
		SF_ASSERT_EQ(rootFile->entryType, EntryType::File);
		SF_ASSERT_EQ(rootFile->scanStatus, EntryScanStatus::Complete);

		Node* firstDirectory = RequireChild(root, "sub_directory_1");
		SF_ASSERT_EQ(firstDirectory->entryType, EntryType::Directory);
		SF_ASSERT_EQ(firstDirectory->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(CountChildren(*firstDirectory), 2);

		Node* firstFile = RequireChild(*firstDirectory, "test_name_2.txt");
		SF_ASSERT_EQ(firstFile->entryType, EntryType::File);
		SF_ASSERT_EQ(firstFile->scanStatus, EntryScanStatus::Complete);

		Node* secondFile = RequireChild(*firstDirectory, "test_name_3.txt");
		SF_ASSERT_EQ(secondFile->entryType, EntryType::File);
		SF_ASSERT_EQ(secondFile->scanStatus, EntryScanStatus::Complete);

		Node* secondDirectory = RequireChild(root, "sub_directory_2");
		SF_ASSERT_EQ(secondDirectory->entryType, EntryType::Directory);
		SF_ASSERT_EQ(secondDirectory->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(CountChildren(*secondDirectory), 1);

		Node* nestedDirectory = RequireChild(*secondDirectory, "sub_directory_3");
		SF_ASSERT_EQ(nestedDirectory->entryType, EntryType::Directory);
		SF_ASSERT_EQ(nestedDirectory->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(CountChildren(*nestedDirectory), 1);

		Node* nestedFile = RequireChild(*nestedDirectory, "test_name_4.txt");
		SF_ASSERT_EQ(nestedFile->entryType, EntryType::File);
		SF_ASSERT_EQ(nestedFile->scanStatus, EntryScanStatus::Complete);
	}
}
