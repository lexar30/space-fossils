#pragma once

#include <space_fossils/core/file_tree/model/default_constants.hxx>
#include <space_fossils/core/file_tree/scan/planning_config.hxx>
#include <space_fossils/core/file_tree/scan/apply_result.hxx>
#include <space_fossils/core/file_tree/scan/scan_input.hxx>
#include <space_fossils/core/file_tree/scan/job.hxx>
#include <space_fossils/core/file_tree/scan/scheduler.hxx>
#include <space_fossils/core/file_tree/scan/scanner.hxx>
#include <space_fossils/core/file_tree/storage/storage_change.hxx>
#include <space_fossils/core/file_tree/scan/summary.hxx>
#include <space_fossils/core/operation_timer.hxx>

#include <cstddef>
#include <filesystem>
#include <vector>

namespace space_fossils::core::file_tree {
	struct Node;
	class Storage;
}

namespace space_fossils::core::file_tree::scan {
	class Coordinator
	{
	public:
		explicit Coordinator(Storage& storage, PlanningConfig planningConfig = {});

		void ScheduleJob(Job job);
		void ScheduleJobs(std::vector<Job> jobs);

		ApplyResult ProcessNext();

		Summary GetSummary() const;
		bool HasScheduledJobs() const;

		const Storage& GetStorage() const;

	private:
		Storage& storage;
		PlanningConfig planningConfig;
		Scheduler scheduler;
		Scanner scanner;

		OperationTimer scanTimer = {};
		MetricsDuration scanElapsedTime = {};

		JobStatistics scanJobStatistics;
	};
}
