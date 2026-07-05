#pragma once

#include "space_fossils/file_tree/change_type.hxx"
#include "space_fossils/file_tree/scan_request.hxx"

namespace space_fossils::core::file_tree {
	struct Node;

	struct ScanJob
	{
		ScanRequest request;
		IncomingChangeType applyAs = IncomingChangeType::Unknown;
		Node* target = nullptr;
	};
}
