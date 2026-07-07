#pragma once

#include "space_fossils/operation_timer.hxx"
#include "space_fossils/file_tree/types.hxx"

#include <cstddef>

namespace space_fossils::core {
	struct NamePoolSummary
	{
		std::size_t allocatedBytes = file_tree::DefaultFileSize;
		std::size_t usedBytes = file_tree::DefaultFileSize;
		std::size_t blocksCount = file_tree::DefaultFileSize;
		std::size_t blockSize = file_tree::DefaultFileSize;
	};

	struct NodePoolSummary
	{
		std::size_t allocatedBytes = file_tree::DefaultFileSize;
		std::size_t usedBytes = file_tree::DefaultFileSize;
		std::size_t blocksCount = file_tree::DefaultFileSize;
		std::size_t blockSize = file_tree::DefaultFileSize;
		std::size_t liveNodesCount = file_tree::DefaultFileSize;
	};

	struct ScanSummary
	{
		MetricsDuration totalScanElapsedTime = {};
		std::size_t storedNodesCount = 0;
		file_tree::FileSize totalLogicalSize = file_tree::DefaultFileSize;

		NamePoolSummary namePoolSummary;
		NodePoolSummary nodePoolSummary;
	};
}