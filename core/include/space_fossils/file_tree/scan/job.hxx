#pragma once

#include "space_fossils/file_tree/change_type.hxx"
#include "space_fossils/file_tree/scan/scan_input.hxx"

namespace space_fossils::core::file_tree {
	struct Node;
}

namespace space_fossils::core::file_tree::scan {
	struct Job
	{
		ScanInput input;
		IncomingChangeType applyAs = IncomingChangeType::Unknown;
		Node* target = nullptr;
	};
}
