#include "space_fossils/file_tree/scan_coordinator.hxx"

#include <limits>
#include <vector>

namespace space_fossils::core::file_tree {
	namespace {
		inline std::filesystem::path BuildPath(const Node* node, const Node* rootNode, const std::filesystem::path& rootPath)
		{
			if (node == nullptr) {
				return {};
			}

			if (rootNode == nullptr) {
				return {};
			}

			if (node == rootNode) {
				return rootPath;
			}

			const Node* currentNode = node;
			std::vector<NativeStringView> pathParts;
			while (currentNode != nullptr && currentNode != rootNode) {
				pathParts.push_back(ToStringView(currentNode->name));
				currentNode = currentNode->parent;
			}

			std::filesystem::path path = rootPath;
			for (auto it = pathParts.rbegin(); it != pathParts.rend(); ++it) {
				path /= std::filesystem::path(*it);
			}

			return path;
		}
	}

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
		if (!scheduler.HasJobs()) {
			return std::nullopt;
		}

		auto job = scheduler.PopNext();
		if (!job.has_value())
		{
			return std::nullopt;
		}

		TreePoolBundle bundle = scanner.Scan(job.value().request);

		IncomingChange incomingChanges;
		incomingChanges.bundle = std::move(bundle);
		incomingChanges.target = job.value().target;
		incomingChanges.type = job.value().applyAs;

		const std::optional<AppliedChange> appliedChange = storage.ApplyChange(std::move(incomingChanges));
		if (appliedChange.has_value()) {
			UpdateScheduledTasks(appliedChange.value());
		}

		return appliedChange;
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

		SchedulePending(changes.addedRoot, BuildPath(changes.addedRoot, storage.GetRoot(), config.rootPath));
	}
}