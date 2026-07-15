#pragma once

#include "space_fossils/core/file_tree/change_type.hxx"
#include "space_fossils/core/file_tree/scan/scan_input.hxx"
#include "space_fossils/core/file_tree/scan/summary.hxx"

#include <cstddef>

namespace space_fossils::core::file_tree {
	class Storage;
	struct Node;
}

namespace space_fossils::core::file_tree::scan {
	class Coordinator;

	class Operations
	{
	public:
		bool TryScheduleRootScan(Coordinator& coordinator, ScanInput input);
		bool TryScheduleAttachScan(Coordinator& coordinator, ScanInput input, Node* target);
		bool TryScheduleReplaceScan(Coordinator& coordinator, ScanInput input, Node* target);
		Summary RunScanning(Coordinator& coordinator);
		bool TryRescan(Storage& storage, Node* target, std::size_t depth);
	};
}
