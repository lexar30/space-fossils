#include "space_fossils/core/file_tree/report/text_writer.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>
#include <sstream>
#include <string>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;
		using namespace space_fossils::core::file_tree::report;

		NativeString MakeNativeString(const char* value)
		{
			NativeString result;
			while (*value != '\0') {
				result.push_back(static_cast<NativeChar>(*value));
				++value;
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
	}

	SF_TEST(file_tree_text_report_writer, WritesTreeWithSiblingConnections)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("folder-a");
		NativeString firstChildName = MakeNativeString("inside.txt");
		NativeString secondName = MakeNativeString("folder-b");

		Node root;
		root.name = MakeNameRef(rootName);
		root.logicalSize = 60;

		Node first;
		first.name = MakeNameRef(firstName);
		first.logicalSize = 10;
		first.parent = &root;

		Node firstChild;
		firstChild.name = MakeNameRef(firstChildName);
		firstChild.logicalSize = 10;
		firstChild.parent = &first;

		Node second;
		second.name = MakeNameRef(secondName);
		second.logicalSize = 50;
		second.parent = &root;

		root.firstChild = &first;
		first.firstChild = &firstChild;
		first.nextSibling = &second;

		TextWriter writer;
		std::ostringstream out;

		const bool written = writer.WriteTreeReport(out, root);

		SF_ASSERT_EQ(written, true);
		SF_ASSERT_EQ(out.str(), std::string(
			"root (60)\n"
			"|-- folder-a (10)\n"
			"|   L-- inside.txt (10)\n"
			"L-- folder-b (50)\n"
		));
	}

	SF_TEST(file_tree_text_report_writer, WritesEmptyNameAsBlankName)
	{
		Node root;
		root.logicalSize = 7;

		TextWriter writer;
		std::ostringstream out;

		const bool written = writer.WriteTreeReport(out, root);

		SF_ASSERT_EQ(written, true);
		SF_ASSERT_EQ(out.str(), std::string(" (7)\n"));
	}

	SF_TEST(file_tree_text_report_writer, ReturnsFalseWhenStreamAlreadyFailed)
	{
		NativeString rootName = MakeNativeString("root");

		Node root;
		root.name = MakeNameRef(rootName);

		TextWriter writer;
		std::ostringstream out;
		out.setstate(std::ios::badbit);

		const bool written = writer.WriteTreeReport(out, root);

		SF_ASSERT_EQ(written, false);
	}
}
