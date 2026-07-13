#pragma once

#include <space_fossils/file_tree/scan/job.hxx>

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

	private:
		std::deque<Job> jobs;
	};
}
