#pragma once

#include <space_fossils/file_tree/default_constants.hxx>
#include <space_fossils/file_tree/scan_coordinator_config.hxx>
#include <space_fossils/file_tree/scan_scheduler.hxx>
#include <space_fossils/file_tree/scanner.hxx>
#include <space_fossils/file_tree/storage_change.hxx>
#include <space_fossils/operation_timer.hxx>
#include <space_fossils/scan_summary.hxx>

#include <cstddef>
#include <filesystem>
#include <optional>

namespace space_fossils::core::file_tree {
	class Storage;

	class ScanCoordinator
	{
	public:
		ScanCoordinator(Storage& storage, ScanCoordinatorConfig config = {});

		void ScheduleRootScan(std::size_t maxDepth = DefaultScanDepth);
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
