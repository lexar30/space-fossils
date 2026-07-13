#pragma once

#include "space_fossils/operation_timer.hxx"
#include "space_fossils/file_tree/model/types.hxx"

#include <cstddef>

namespace space_fossils::core::file_tree::scan {
	struct NamePoolSummary
	{
		std::size_t allocatedBytes = DefaultFileSize;
		std::size_t usedBytes = DefaultFileSize;
		std::size_t blocksCount = DefaultFileSize;
		std::size_t blockSize = DefaultFileSize;
	};

	struct NodePoolSummary
	{
		std::size_t allocatedBytes = DefaultFileSize;
		std::size_t usedBytes = DefaultFileSize;
		std::size_t blocksCount = DefaultFileSize;
		std::size_t blockSize = DefaultFileSize;
		std::size_t liveNodesCount = DefaultFileSize;
	};

	struct JobStatistics
	{
		std::size_t emptyProcessCalls = 0;
		std::size_t appliedJobCount = 0;
		std::size_t rejectedJobCount = 0;
	};

	struct Summary
	{
		MetricsDuration totalScanElapsedTime = {};
		std::size_t storedNodesCount = 0;
		FileSize totalLogicalSize = DefaultFileSize;
		JobStatistics scanJobStatistics;

		NamePoolSummary namePoolSummary;
		NodePoolSummary nodePoolSummary;
	};
}
