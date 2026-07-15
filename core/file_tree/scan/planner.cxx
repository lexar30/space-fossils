#include "space_fossils/core/file_tree/scan/planner.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/scan/scan_input.hxx"
#include "space_fossils/core/file_tree/storage/storage_change.hxx"
#include "space_fossils/core/file_tree/model/types.hxx"

#include <utility>

namespace space_fossils::core::file_tree::scan {
	Job Planner::PlanRootScan(ScanInput input)
	{
		Job scanJob;
		scanJob.input = std::move(input);
		scanJob.applyAs = ChangeType::AdoptRoot;
		scanJob.target = nullptr;

		return scanJob;
	}

	Job Planner::PlanReplaceScan(ScanInput input, Node* target)
	{
		Job scanJob;
		scanJob.input = std::move(input);
		scanJob.applyAs = ChangeType::Replace;
		scanJob.target = target;

		return scanJob;
	}

	Job Planner::PlanAttachScan(ScanInput input, Node* parent)
	{
		Job scanJob;
		scanJob.input = std::move(input);
		scanJob.applyAs = ChangeType::Attach;
		scanJob.target = parent;

		return scanJob;
	}

	void Planner::PopulateFollowUpJobs(std::vector<Job>& jobs, Node* node, const std::filesystem::path& path, PlanningConfig config)
	{
		if (node == nullptr) {
			return;
		}

		if (path.empty()) {
			return;
		}

		if (node->entryType == EntryType::Directory
			&& node->scanStatus == EntryScanStatus::Pending) {
			Job job;
			job.input.path = path;
			job.input.maxDepth = config.jobDepth;
			job.applyAs = ChangeType::Replace;
			job.target = node;

			jobs.push_back(std::move(job));
			return;
		}

		for (Node* child = node->firstChild; child != nullptr; child = child->nextSibling) {
			const NativeString childName{ ToStringView(child->name) };
			PopulateFollowUpJobs(jobs, child, path / childName, config);
		}
	}

	std::vector<Job> Planner::PlanFollowUpJobs(const Job& completedJob, const AppliedChange& changes, PlanningConfig config)
	{
		if (changes.addedRoot == nullptr) {
			return {};
		}

		if (changes.type == ChangeType::Unknown) {
			return {};
		}

		if (config.jobDepth == 0) {
			return {};
		}

		if (completedJob.input.path.empty()) {
			return {};
		}

		std::vector<Job> jobs;
		jobs.reserve(8);
		PopulateFollowUpJobs(jobs, changes.addedRoot, completedJob.input.path, config);
		return jobs;
	}
}
