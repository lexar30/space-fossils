#include "space_fossils/file_tree/scan/scheduler.hxx"

#include <utility>

namespace space_fossils::core::file_tree::scan {
	void Scheduler::Schedule(Job scanJob)
	{
		jobs.push_back(std::move(scanJob));
	}

	void Scheduler::Clear()
	{
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
}
