#pragma once

#include <space_fossils/file_tree/scan_job.hxx>
#include <space_fossils/file_tree/scan_scheduler.hxx>
#include <space_fossils/file_tree/scanner.hxx>
#include <space_fossils/file_tree/storage.hxx>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>

namespace space_fossils::core::file_tree {
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

	private:
		void UpdateScheduledTasks(const AppliedChange& changes);
		void SchedulePending(Node* node, const std::filesystem::path& path);

	private:
		Storage& storage;
		ScanScheduler scheduler;
		Scanner scanner;
		ScanCoordinatorConfig config;
	};
}