#pragma once

#include "space_fossils/file_tree/scan/planning_config.hxx"
#include "space_fossils/file_tree/scan/job.hxx"

#include <filesystem>
#include <vector>

namespace space_fossils::core::file_tree {
	struct AppliedChange;
	struct Node;
}

namespace space_fossils::core::file_tree::scan {
	class Planner
	{
	public:
		static Job PlanRootScan(ScanInput input);
		static Job PlanReplaceScan(ScanInput input, Node* target);
		static Job PlanAttachScan(ScanInput input, Node* parent);

		static std::vector<Job> PlanFollowUpJobs(
			const Job& completedJob
			, const AppliedChange& changes
			, PlanningConfig config
		);

	private:
		static void PopulateFollowUpJobs(
			std::vector<Job>& jobs
			, Node* node
			, const std::filesystem::path& path
			, PlanningConfig config
		);
	};
}
