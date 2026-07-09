#include "space_fossils/file_tree/node.hxx"
#include "space_fossils/file_tree/tree_query.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <vector>

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

		NativeString MakeNativePath(std::initializer_list<const char*> components)
		{
			NativeString result;
			bool firstComponent = true;

			for (const char* component : components) {
				if (!firstComponent) {
					result.push_back(std::filesystem::path::preferred_separator);
				}

				result.append(MakeNativeString(component));
				firstComponent = false;
			}

			return result;
		}

		NameRef MakeNameRef(const NativeString& value)
		{
			return NameRef{
				value.data(),
				static_cast<std::uint32_t>(value.size())
			};
		}

		void AssertNativeStringEquals(NativeStringView actual, NativeStringView expected)
		{
			SF_ASSERT_EQ(actual.size(), expected.size());
			for (std::size_t index = 0; index < expected.size(); ++index) {
				SF_ASSERT_EQ(actual[index] == expected[index], true);
			}
		}

		void AppendChild(Node& parent, Node& child)
		{
			child.parent = &parent;
			child.nextSibling = nullptr;

			if (parent.firstChild == nullptr) {
				parent.firstChild = &child;
				return;
			}

			Node* lastChild = parent.firstChild;
			while (lastChild->nextSibling != nullptr) {
				lastChild = lastChild->nextSibling;
			}

			lastChild->nextSibling = &child;
		}
	}

	SF_TEST(file_tree_tree_query, NullInputsReturnEmptyResults)
	{
		NativeString missingName = MakeNativeString("missing");

		SF_ASSERT_EQ(TreeQuery::BuildNativePath(nullptr).empty(), true);
		SF_ASSERT_EQ(TreeQuery::CollectPathComponents(nullptr).empty(), true);
		SF_ASSERT_EQ(TreeQuery::GetDepth(nullptr), 0);
		SF_ASSERT_EQ(TreeQuery::CollectChildren(nullptr).empty(), true);
		SF_ASSERT_EQ(TreeQuery::FindChildByName(nullptr, missingName) == nullptr, true);
		SF_ASSERT_EQ(TreeQuery::CountSubtreeNodes(nullptr), 0);
		SF_ASSERT_EQ(TreeQuery::ContainsInSiblingChain(nullptr, nullptr), false);
		SF_ASSERT_EQ(TreeQuery::ContainsInSubtree(nullptr, nullptr), false);
	}

	SF_TEST(file_tree_tree_query, CollectsPathComponentsFromRootToTarget)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString folderName = MakeNativeString("folder");
		NativeString fileName = MakeNativeString("file.txt");

		Node root;
		root.name = MakeNameRef(rootName);

		Node folder;
		folder.name = MakeNameRef(folderName);

		Node file;
		file.name = MakeNameRef(fileName);

		AppendChild(root, folder);
		AppendChild(folder, file);

		const std::vector<NativeStringView> components = TreeQuery::CollectPathComponents(&file);

		SF_ASSERT_EQ(components.size(), 3);
		AssertNativeStringEquals(components[0], rootName);
		AssertNativeStringEquals(components[1], folderName);
		AssertNativeStringEquals(components[2], fileName);
		SF_ASSERT_EQ(TreeQuery::GetDepth(&file), 2);
	}

	SF_TEST(file_tree_tree_query, RootOnlyNodeQueriesReturnSingleNodeValues)
	{
		NativeString rootName = MakeNativeString("root");

		Node root;
		root.name = MakeNameRef(rootName);

		const std::vector<NativeStringView> components = TreeQuery::CollectPathComponents(&root);

		SF_ASSERT_EQ(components.size(), 1);
		AssertNativeStringEquals(components[0], rootName);
		AssertNativeStringEquals(TreeQuery::BuildNativePath(&root), rootName);
		SF_ASSERT_EQ(TreeQuery::GetDepth(&root), 0);
		SF_ASSERT_EQ(TreeQuery::CollectChildren(&root).empty(), true);
		SF_ASSERT_EQ(TreeQuery::CountSubtreeNodes(&root), 1);
		SF_ASSERT_EQ(TreeQuery::ContainsInSiblingChain(&root, &root), true);
		SF_ASSERT_EQ(TreeQuery::ContainsInSubtree(&root, &root), true);
	}

	SF_TEST(file_tree_tree_query, BuildNativePathJoinsComponents)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString folderName = MakeNativeString("folder");
		NativeString fileName = MakeNativeString("file.txt");

		Node root;
		root.name = MakeNameRef(rootName);

		Node folder;
		folder.name = MakeNameRef(folderName);

		Node file;
		file.name = MakeNameRef(fileName);

		AppendChild(root, folder);
		AppendChild(folder, file);

		const NativeString path = TreeQuery::BuildNativePath(&file);
		const NativeString expectedPath = MakeNativePath({ "root", "folder", "file.txt" });

		AssertNativeStringEquals(path, expectedPath);
	}

	SF_TEST(file_tree_tree_query, BuildNativePathDoesNotDuplicateExistingSeparator)
	{
		NativeString rootName = MakeNativeString("D:");
		rootName.push_back(std::filesystem::path::preferred_separator);
		NativeString folderName = MakeNativeString("folder");

		Node root;
		root.name = MakeNameRef(rootName);

		Node folder;
		folder.name = MakeNameRef(folderName);

		AppendChild(root, folder);

		NativeString expectedPath = rootName;
		expectedPath.append(folderName);

		AssertNativeStringEquals(TreeQuery::BuildNativePath(&folder), expectedPath);
	}

	SF_TEST(file_tree_tree_query, BuildNativePathDoesNotDuplicateForwardSlashSeparator)
	{
		NativeString rootName = MakeNativeString("D:/");
		NativeString folderName = MakeNativeString("folder");

		Node root;
		root.name = MakeNameRef(rootName);

		Node folder;
		folder.name = MakeNameRef(folderName);

		AppendChild(root, folder);

		NativeString expectedPath = rootName;
		expectedPath.append(folderName);

		AssertNativeStringEquals(TreeQuery::BuildNativePath(&folder), expectedPath);
	}

	SF_TEST(file_tree_tree_query, CollectChildrenPreservesSiblingOrder)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("first");
		NativeString secondName = MakeNativeString("second");
		NativeString thirdName = MakeNativeString("third");

		Node root;
		root.name = MakeNameRef(rootName);

		Node first;
		first.name = MakeNameRef(firstName);

		Node second;
		second.name = MakeNameRef(secondName);

		Node third;
		third.name = MakeNameRef(thirdName);

		AppendChild(root, first);
		AppendChild(root, second);
		AppendChild(root, third);

		const std::vector<const Node*> children = TreeQuery::CollectChildren(&root);

		SF_ASSERT_EQ(children.size(), 3);
		SF_ASSERT_EQ(children[0] == &first, true);
		SF_ASSERT_EQ(children[1] == &second, true);
		SF_ASSERT_EQ(children[2] == &third, true);
		SF_ASSERT_EQ(TreeQuery::CollectChildren(&third).empty(), true);
	}

	SF_TEST(file_tree_tree_query, FindChildByNameComparesNameText)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString childName = MakeNativeString("same-name");
		NativeString searchName = MakeNativeString("same-name");
		NativeString missingName = MakeNativeString("missing");

		Node root;
		root.name = MakeNameRef(rootName);

		Node child;
		child.name = MakeNameRef(childName);

		AppendChild(root, child);

		SF_ASSERT_EQ(TreeQuery::FindChildByName(&root, searchName) == &child, true);
		SF_ASSERT_EQ(TreeQuery::FindChildByName(&root, missingName) == nullptr, true);
	}

	SF_TEST(file_tree_tree_query, FindChildByNameSearchesOnlyDirectChildren)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString folderName = MakeNativeString("folder");
		NativeString targetName = MakeNativeString("target");

		Node root;
		root.name = MakeNameRef(rootName);

		Node folder;
		folder.name = MakeNameRef(folderName);

		Node nestedTarget;
		nestedTarget.name = MakeNameRef(targetName);

		AppendChild(root, folder);
		AppendChild(folder, nestedTarget);

		SF_ASSERT_EQ(TreeQuery::FindChildByName(&root, targetName) == nullptr, true);
		SF_ASSERT_EQ(TreeQuery::FindChildByName(&folder, targetName) == &nestedTarget, true);
	}

	SF_TEST(file_tree_tree_query, FindChildByNameReturnsFirstMatchingChild)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString duplicateName = MakeNativeString("duplicate");

		Node root;
		root.name = MakeNameRef(rootName);

		Node first;
		first.name = MakeNameRef(duplicateName);

		Node second;
		second.name = MakeNameRef(duplicateName);

		AppendChild(root, first);
		AppendChild(root, second);

		SF_ASSERT_EQ(TreeQuery::FindChildByName(&root, duplicateName) == &first, true);
	}

	SF_TEST(file_tree_tree_query, CountAndContainsUseDifferentTraversalContracts)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("first");
		NativeString nestedName = MakeNativeString("nested");
		NativeString secondName = MakeNativeString("second");

		Node root;
		root.name = MakeNameRef(rootName);

		Node first;
		first.name = MakeNameRef(firstName);

		Node nested;
		nested.name = MakeNameRef(nestedName);

		Node second;
		second.name = MakeNameRef(secondName);

		AppendChild(root, first);
		AppendChild(first, nested);
		AppendChild(root, second);

		SF_ASSERT_EQ(TreeQuery::CountSubtreeNodes(&root), 4);
		SF_ASSERT_EQ(TreeQuery::CountSubtreeNodes(&first), 2);
		SF_ASSERT_EQ(TreeQuery::CountSubtreeNodes(&second), 1);

		SF_ASSERT_EQ(TreeQuery::ContainsInSiblingChain(&root, &second), true);
		SF_ASSERT_EQ(TreeQuery::ContainsInSiblingChain(&first, &nested), true);
		SF_ASSERT_EQ(TreeQuery::ContainsInSiblingChain(&first, &second), true);
		SF_ASSERT_EQ(TreeQuery::ContainsInSiblingChain(&root, nullptr), false);

		SF_ASSERT_EQ(TreeQuery::ContainsInSubtree(&root, &second), true);
		SF_ASSERT_EQ(TreeQuery::ContainsInSubtree(&first, &nested), true);
		SF_ASSERT_EQ(TreeQuery::ContainsInSubtree(&first, &second), false);
		SF_ASSERT_EQ(TreeQuery::ContainsInSubtree(&root, nullptr), false);
	}
}
