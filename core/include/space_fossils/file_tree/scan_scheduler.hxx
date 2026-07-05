#pragma once

#include <space_fossils/file_tree/scan_job.hxx>
#include <space_fossils/file_tree/tree_pool_bundle.hxx>

#include <deque>
#include <optional>

namespace space_fossils::core::file_tree {
	class ScanScheduler
	{
	public:
		void Schedule(ScanJob scanJob);
		void Clear();
		bool HasJobs() const;
		std::optional<ScanJob> PopNext();

	private:
		std::deque<ScanJob> jobs;
	};
}