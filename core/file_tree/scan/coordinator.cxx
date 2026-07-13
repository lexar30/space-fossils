#include "space_fossils/file_tree/scan/coordinator.hxx"

#include "space_fossils/file_tree/scan/planner.hxx"
#include "space_fossils/file_tree/storage/storage.hxx"

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

		Job scanJob = std::move(job.value());

		scanTimer.Reset();
		scanTimer.Start();
		TreePoolBundle bundle = scanner.Scan(scanJob.input);
		scanTimer.Stop();
		scanElapsedTime += scanTimer.Elapsed();

		IncomingChange incomingChanges;
		incomingChanges.bundle = std::move(bundle);
		incomingChanges.target = scanJob.target;
		incomingChanges.type = scanJob.applyAs;

		ApplyResult result;
		result.appliedChange = storage.ApplyChange(std::move(incomingChanges));
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
