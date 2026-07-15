#include "space_fossils/core/file_tree/scan/coordinator.hxx"

#include "space_fossils/core/file_tree/scan/planner.hxx"
#include "space_fossils/core/file_tree/storage/storage.hxx"

#include <limits>
#include <utility>

namespace space_fossils::core::file_tree::scan {
	Coordinator::Coordinator(Storage& storage, PlanningConfig planningConfig)
		: storage(storage)
		, planningConfig(planningConfig)
	{
	}

	void Coordinator::ScheduleJob(Job job)
	{
		scheduler.Schedule(std::move(job));
	}

	void Coordinator::ScheduleJobs(std::vector<Job> jobs)
	{
		for (auto& job : jobs) {
			scheduler.Schedule(std::move(job));
		}
	}

	ApplyResult Coordinator::ProcessNext()
	{
		auto job = scheduler.PopNext();
		if (!job.has_value()) {
			++scanJobStatistics.emptyProcessCalls;
			return {};
		}

		if (job.value().applyAs == ChangeType::Unknown) {
			++scanJobStatistics.rejectedJobCount;
			return { ApplyStatus::Rejected };
		}

		Job scanJob = std::move(job.value());
		ApplyResult result;

		if (scanJob.applyAs == ChangeType::Remove) {
			result.appliedChange = storage.TryRemoveSubtree(scanJob.target);
		}
		else {
			if (scanJob.target == nullptr && (scanJob.applyAs == ChangeType::Attach || scanJob.applyAs == ChangeType::Replace)) {
				++scanJobStatistics.rejectedJobCount;
				return { ApplyStatus::Rejected };
			}

			scanTimer.Reset();
			scanTimer.Start();
			TreePoolBundle bundle = scanner.Scan(scanJob.input);
			scanTimer.Stop();
			scanElapsedTime += scanTimer.Elapsed();

			switch (scanJob.applyAs) {
			case ChangeType::AdoptRoot:
				result.appliedChange = storage.TryAdoptRoot(std::move(bundle));
				break;

			case ChangeType::Attach: {
				result.appliedChange = storage.TryAttachChild(scanJob.target, std::move(bundle));
				break;
			}

			case ChangeType::Replace: {
				result.appliedChange = storage.TryReplaceSubtree(scanJob.target, std::move(bundle));
				break;
			}

			default:
				result.appliedChange = std::nullopt;
			}
		}

		if (result.appliedChange.has_value()) {
			result.status = ApplyStatus::Applied;
			++scanJobStatistics.appliedJobCount;
			ScheduleJobs(Planner::PlanFollowUpJobs(scanJob, result.appliedChange.value(), planningConfig));
		}
		else {
			++scanJobStatistics.rejectedJobCount;
			result.status = ApplyStatus::Rejected;
		}

		return result;
	}

	Summary Coordinator::GetSummary() const
	{
		Summary summary;

		summary.totalScanElapsedTime = scanElapsedTime;
		summary.pendingJobsPeakCount = scheduler.GetPendingJobsPeakCount();
		summary.storedNodesCount = storage.GetNodesCount();
		summary.totalLogicalSize = storage.GetRootSize();

		summary.namePoolSummary.allocatedBytes = storage.GetNamePoolAllocatedBytes();
		summary.namePoolSummary.usedBytes = storage.GetNamePoolUsedBytes();
		summary.namePoolSummary.blocksCount = storage.GetNamePoolBlocksCount();
		summary.namePoolSummary.blockSize = storage.GetNamePoolBlockSize();

		summary.nodePoolSummary.allocatedBytes = storage.GetNodePoolAllocatedBytes();
		summary.nodePoolSummary.usedBytes = storage.GetNodePoolUsedBytes();
		summary.nodePoolSummary.blocksCount = storage.GetNodePoolBlocksCount();
		summary.nodePoolSummary.blockSize = storage.GetNodePoolBlockSize();
		summary.nodePoolSummary.liveNodesCount = storage.GetNodePoolLiveNodesCount();

		summary.scanJobStatistics = scanJobStatistics;

		return summary;
	}

	bool Coordinator::HasScheduledJobs() const
	{
		return scheduler.HasJobs();
	}

	const Storage& Coordinator::GetStorage() const
	{
		return storage;
	}
}
