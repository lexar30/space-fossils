#pragma once

#include <ostream>

namespace space_fossils::core::file_tree {
	struct Node;

	class SnapshotWriter
	{
		bool WriteSnapshot(std::ostream& out, const Node& root);
	};
}
