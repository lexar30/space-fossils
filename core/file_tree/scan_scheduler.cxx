#include "space_fossils/file_tree/scan_scheduler.hxx"

#include <utility>

namespace space_fossils::core::file_tree {
	void ScanScheduler::Schedule(ScanJob scanJob)
	{
		jobs.push_back(std::move(scanJob));
	}

	void ScanScheduler::Clear()
	{
		jobs.clear();
	}

	bool ScanScheduler::HasJobs() const
	{
		return !jobs.empty();
	}

	std::optional<ScanJob> ScanScheduler::PopNext()
	{
		if (jobs.empty()) {
			return std::nullopt;
		}

		ScanJob job = std::move(jobs.front());
		jobs.pop_front();

		return job;
	}
}
