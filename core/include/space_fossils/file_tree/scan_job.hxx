#pragma once

#include "space_fossils/file_tree/scan_request.hxx"
#include "space_fossils/file_tree/node.hxx"
#include "space_fossils/file_tree/storage_change.hxx"

namespace space_fossils::core::file_tree {
	struct ScanJob
	{
		ScanRequest request;
		IncomingChangeType applyAs = IncomingChangeType::Unknown;
		Node* target = nullptr;
	};
}