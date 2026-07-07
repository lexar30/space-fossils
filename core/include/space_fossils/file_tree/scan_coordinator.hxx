#pragma once

#include <space_fossils/file_tree/scan_scheduler.hxx>
#include <space_fossils/file_tree/scanner.hxx>
#include <space_fossils/file_tree/storage_change.hxx>
#include <space_fossils/operation_timer.hxx>
#include <space_fossils/scan_summary.hxx>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <optional>

namespace space_fossils::core::file_tree {
	class Storage;

	struct ScanCoordinatorConfig
	{
		std::filesystem::path rootPath;
		std::size_t defaultScanDepth = std::numeric_limits<std::size_t>().max();
	};

	class ScanCoordinator
	{
	public:
		ScanCoordinator(Storage& storage, ScanCoordinatorConfig config = {});

		void ScheduleRootScan(std::size_t maxDepth = 1);
		std::optional<AppliedChange> ProcessNext();

		ScanSummary GetScanSummary() const;

	private:
		void UpdateScheduledTasks(const AppliedChange& changes);
		void SchedulePending(Node* node, const std::filesystem::path& path);
		std::filesystem::path BuildPath(const Node* node) const;

	private:
		Storage& storage;
		ScanScheduler scheduler;
		Scanner scanner;
		ScanCoordinatorConfig config;

		OperationTimer scanTimer = {};
		MetricsDuration scanElapsedTime = {};
	};
}