#include "space_fossils/core/file_tree/scan/scheduler.hxx"

#include <utility>

namespace space_fossils::core::file_tree::scan {
	void Scheduler::Schedule(Job scanJob)
	{
		jobs.push_back(std::move(scanJob));

		const std::size_t jobsCount = jobs.size();
		if (jobsCount > pendingJobsPeakCount) {
			pendingJobsPeakCount = jobsCount;
		}
	}

	void Scheduler::Clear()
	{
		pendingJobsPeakCount = 0;
		jobs.clear();
	}

	bool Scheduler::HasJobs() const
	{
		return !jobs.empty();
	}

	std::optional<Job> Scheduler::PopNext()
	{
		if (jobs.empty()) {
			return std::nullopt;
		}

		Job job = std::move(jobs.front());
		jobs.pop_front();

		return job;
	}

	std::size_t Scheduler::GetPendingJobsPeakCount() const
	{
		return pendingJobsPeakCount;
	}
}
