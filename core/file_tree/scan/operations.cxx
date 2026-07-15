#include "space_fossils/core/file_tree/scan/operations.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/query/tree_query.hxx"
#include "space_fossils/core/file_tree/scan/coordinator.hxx"
#include "space_fossils/core/file_tree/scan/planner.hxx"
#include "space_fossils/core/file_tree/storage/storage.hxx"

#include <utility>

namespace space_fossils::core::file_tree::scan {
	bool Operations::TryScheduleRootScan(Coordinator& coordinator, ScanInput input)
	{
		if (input.path.empty()) {
			return false;
		}

		coordinator.ScheduleJob(Planner::PlanRootScan(std::move(input)));
		return true;
	}

	bool Operations::TryScheduleAttachScan(Coordinator& coordinator, ScanInput input, Node* target)
	{
		if (target == nullptr) {
			return false;
		}

		if (input.path.empty()) {
			return false;
		}

		if (!TreeQuery::ContainsInSubtree(coordinator.GetStorage().GetRoot(), target)) {
			return false;
		}

		if (target->entryType != EntryType::Directory) {
			return false;
		}

		coordinator.ScheduleJob(Planner::PlanAttachScan(std::move(input), target));
		return true;
	}

	bool Operations::TryScheduleReplaceScan(Coordinator& coordinator, ScanInput input, Node* target)
	{
		if (target == nullptr) {
			return false;
		}

		if (input.path.empty()) {
			return false;
		}

		if (!TreeQuery::ContainsInSubtree(coordinator.GetStorage().GetRoot(), target)) {
			return false;
		}

		coordinator.ScheduleJob(Planner::PlanReplaceScan(std::move(input), target));
		return true;
	}

	Summary Operations::RunScanning(Coordinator& coordinator)
	{
		// TODO: dangerous func, think about it later
		while (coordinator.HasScheduledJobs()) {
			coordinator.ProcessNext();
		}

		return coordinator.GetSummary();
	}

	bool Operations::TryRescan(Storage& storage, Node* target, std::size_t depth)
	{
		// TODO
		return false;
	}
}
