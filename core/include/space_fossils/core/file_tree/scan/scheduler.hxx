#pragma once

#include <space_fossils/core/file_tree/scan/job.hxx>

#include <cstddef>
#include <deque>
#include <optional>

namespace space_fossils::core::file_tree::scan {
	class Scheduler
	{
	public:
		void Schedule(Job scanJob);
		void Clear();
		bool HasJobs() const;
		std::optional<Job> PopNext();
		std::size_t GetPendingJobsPeakCount() const;

	private:
		std::deque<Job> jobs;
		std::size_t pendingJobsPeakCount = 0;
	};
}
