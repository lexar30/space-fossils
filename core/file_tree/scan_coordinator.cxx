#include "space_fossils/file_tree/scan_coordinator.hxx"

#include "space_fossils/file_tree/storage.hxx"

#include <limits>
#include <utility>
#include <vector>

namespace space_fossils::core::file_tree {
	ScanCoordinator::ScanCoordinator(Storage& storage, ScanCoordinatorConfig config)
		: storage(storage)
		, config(config)
	{
	}

	void ScanCoordinator::ScheduleRootScan(std::size_t maxDepth)
	{
		ScanJob job;
		job.request.path = config.rootPath;
		job.request.maxDepth = maxDepth;
		job.applyAs = IncomingChangeType::AdoptRoot;
		job.target = nullptr;

		scheduler.Schedule(std::move(job));
	}

	std::optional<AppliedChange> ScanCoordinator::ProcessNext()
	{
		while (scheduler.HasJobs()) {
			auto job = scheduler.PopNext();
			if (!job.has_value()) {
				return std::nullopt;
			}

			scanTimer.Reset();
			scanTimer.Start();
			TreePoolBundle bundle = scanner.Scan(job.value().request);
			scanTimer.Stop();
			scanElapsedTime += scanTimer.Elapsed();

			IncomingChange incomingChanges;
			incomingChanges.bundle = std::move(bundle);
			incomingChanges.target = job.value().target;
			incomingChanges.type = job.value().applyAs;

			const std::optional<AppliedChange> appliedChange = storage.ApplyChange(std::move(incomingChanges));
			if (appliedChange.has_value()) {
				UpdateScheduledTasks(appliedChange.value());
				return appliedChange;
			}
		}

		return std::nullopt;
	}

	ScanSummary ScanCoordinator::GetScanSummary() const
	{
		ScanSummary summary;

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

		return summary;
	}

	void ScanCoordinator::SchedulePending(Node* node, const std::filesystem::path& path)
	{
		if (node == nullptr) {
			return;
		}

		if (path.empty()) {
			return;
		}

		if (node->entryType == EntryType::Directory
			&& node->scanStatus == EntryScanStatus::Pending) {
			ScanJob job;
			job.request.path = path;
			job.request.maxDepth = config.defaultScanDepth;
			job.applyAs = IncomingChangeType::Replace;
			job.target = node;

			scheduler.Schedule(std::move(job));
			return;
		}

		for (Node* child = node->firstChild; child != nullptr; child = child->nextSibling) {
			SchedulePending(child, path / ToStringView(child->name));
		}
	}

	void ScanCoordinator::UpdateScheduledTasks(const AppliedChange& changes)
	{
		if (changes.addedRoot == nullptr) {
			return;
		}

		if (changes.type == IncomingChangeType::Unknown) {
			return;
		}

		SchedulePending(changes.addedRoot, BuildPath(changes.addedRoot));
	}

	std::filesystem::path ScanCoordinator::BuildPath(const Node* node) const
	{
		if (node == nullptr) {
			return {};
		}

		const Node* rootNode = storage.GetRoot();
		if (rootNode == nullptr) {
			return {};
		}

		if (node == rootNode) {
			return config.rootPath;
		}

		const Node* currentNode = node;
		std::vector<NativeStringView> pathParts;
		while (currentNode != nullptr && currentNode != rootNode) {
			pathParts.push_back(ToStringView(currentNode->name));
			currentNode = currentNode->parent;
		}

		if (currentNode != rootNode) {
			return {};
		}

		std::filesystem::path path = config.rootPath;
		for (auto it = pathParts.rbegin(); it != pathParts.rend(); ++it) {
			path /= std::filesystem::path(*it);
		}

		return path;
	}
}