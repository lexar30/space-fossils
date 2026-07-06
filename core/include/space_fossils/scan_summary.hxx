#pragma once

#include "space_fossils/operation_timer.hxx"
#include "space_fossils/file_tree/types.hxx"

#include <cstddef>

namespace space_fossils::core {
	struct ScanSummary
	{
		MetricsDuration totalElapsedTime = {};
		std::size_t storedNodesCount = 0;
		file_tree::FileSize totalLogicalSize = file_tree::DefaultFileSize;
		file_tree::FileSize poolsAllocatedBytes = file_tree::DefaultFileSize;
		file_tree::FileSize poolsUsedBytes = file_tree::DefaultFileSize;
	};
}
