#include "space_fossils/file_tree/report/text_writer.hxx"

#include "space_fossils/file_tree/model/node.hxx"

#include <filesystem>
#include <string>

namespace space_fossils::core::file_tree::report {
	void TextWriter::WriteName(std::ostream& out, NameRef name)
	{
		NativeStringView view = ToStringView(name);
		const std::filesystem::path path{ NativeString(view) };
		const auto utf8 = path.u8string();

		out.write(
			reinterpret_cast<const char*>(utf8.data()),
			static_cast<std::streamsize>(utf8.size())
		);
	}

	bool TextWriter::WriteTreeReport(std::ostream& out, const Node& root)
	{
		WriteNode(out, root, {}, true, true);
		return out.good();
	}

	bool TextWriter::WriteNode(std::ostream& out, const Node& node, const std::string& prefix, bool isLast, bool isRoot)
	{
		if (!isRoot) {
			out << prefix << (isLast ? "L-- " : "|-- ");
		}

		WriteName(out, node.name);
		out << " (" << node.logicalSize << ")\n";

		std::string childPrefix = prefix;
		if (!isRoot) {
			childPrefix += isLast ? "    " : "|   ";
		}

		for (const Node* child = node.firstChild; child != nullptr; child = child->nextSibling) {
			const bool childIsLast = child->nextSibling == nullptr;
			if (!WriteNode(out, *child, childPrefix, childIsLast, false)) {
				return false;
			}
		}

		return out.good();
	}
}