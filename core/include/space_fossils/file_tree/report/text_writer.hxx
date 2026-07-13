#pragma once

#include "space_fossils/file_tree/model/types.hxx"

#include <cstddef>
#include <ostream>
#include <string>

namespace space_fossils::core::file_tree {
	struct Node;
}

namespace space_fossils::core::file_tree::report {
	class TextWriter
	{
	public:
		bool WriteTreeReport(std::ostream& out, const Node& root);

	private:
		bool WriteNode(std::ostream& out, const Node& node, const std::string& prefix, bool isLast, bool isRoot);
		void WriteName(std::ostream& out, NameRef name);
	};
}
