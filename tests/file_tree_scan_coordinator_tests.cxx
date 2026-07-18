#include "space_fossils/core/file_tree/scan/coordinator.hxx"

#include "space_fossils/core/file_tree/scan/planner.hxx"
#include "space_fossils/core/file_tree/storage/storage.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;
		using namespace space_fossils::core::file_tree::scan;

		NativeString MakeNativeString(const char* value)
		{
			NativeString result;
			while (*value != '\0') {
				result.push_back(static_cast<NativeChar>(*value));
				++value;
			}

			return result;
		}

		bool NameEquals(const Node& node, const char* expectedValue)
		{
			NativeString expectedName = MakeNativeString(expectedValue);
			NativeStringView actualName = ToStringView(node.name);

			if (actualName.size() != expectedName.size()) {
				return false;
			}

			for (std::size_t index = 0; index < expectedName.size(); ++index) {
				if (actualName[index] != expectedName[index]) {
					return false;
				}
			}

			return true;
		}

		void AssertNameEquals(const Node& node, const char* expectedValue)
		{
			SF_ASSERT_EQ(NameEquals(node, expectedValue), true);
		}

		Node* FindChild(const Node& parent, const char* name)
		{
			Node* child = parent.firstChild;
			while (child != nullptr) {
				if (NameEquals(*child, name)) {
					return child;
				}

				child = child->nextSibling;
			}

			return nullptr;
		}

		Node* RequireChild(const Node& parent, const char* name)
		{
			Node* child = FindChild(parent, name);
			SF_ASSERT_EQ(child != nullptr, true);
			SF_ASSERT_EQ(child->parent == &parent, true);

			return child;
		}

		std::size_t CountChildren(const Node& parent)
		{
			std::size_t count = 0;
			const Node* child = parent.firstChild;
			while (child != nullptr) {
				++count;
				child = child->nextSibling;
			}

			return count;
		}

		std::size_t CountPendingDirectories(const Node* node)
		{
			if (node == nullptr) {
				return 0;
			}

			std::size_t count = 0;
			if (node->entryType == EntryType::Directory && node->scanStatus == EntryScanStatus::Pending) {
				++count;
			}

			for (const Node* child = node->firstChild; child != nullptr; child = child->nextSibling) {
				count += CountPendingDirectories(child);
			}

			return count;
		}

		ScanInput MakeRootScanInput(const char* rootPath, std::size_t maxDepth)
		{
			ScanInput scanInput;
			scanInput.path = std::filesystem::path(rootPath);
			scanInput.maxDepth = maxDepth;

			return scanInput;
		}

		void ScheduleRootJob(Coordinator& coordinator, const char* rootPath, std::size_t maxDepth)
		{
			coordinator.ScheduleJob(Planner::PlanRootScan(MakeRootScanInput(rootPath, maxDepth)));
		}

		Coordinator MakeCoordinator(Storage& storage)
		{
			return Coordinator(storage);
		}

		void AssertApplied(const ApplyResult& result)
		{
			SF_ASSERT_EQ(result.status, ApplyStatus::Applied);
			SF_ASSERT_EQ(result.appliedChange.has_value(), true);
		}

		void AssertRejected(const ApplyResult& result)
		{
			SF_ASSERT_EQ(result.status, ApplyStatus::Rejected);
			SF_ASSERT_EQ(result.appliedChange.has_value(), false);
		}

		void AssertNoJob(const ApplyResult& result)
		{
			SF_ASSERT_EQ(result.status, ApplyStatus::NoJob);
			SF_ASSERT_EQ(result.appliedChange.has_value(), false);
		}

		void AssertSummaryMatchesStorage(const Summary& summary, const Storage& storage)
		{
			SF_ASSERT_EQ(summary.storedNodesCount, storage.GetNodesCount());
			SF_ASSERT_EQ(summary.totalLogicalSize, storage.GetRootSize());
			SF_ASSERT_EQ(summary.namePoolSummary.allocatedBytes, storage.GetNamePoolAllocatedBytes());
			SF_ASSERT_EQ(summary.namePoolSummary.usedBytes, storage.GetNamePoolUsedBytes());
			SF_ASSERT_EQ(summary.namePoolSummary.blocksCount, storage.GetNamePoolBlocksCount());
			SF_ASSERT_EQ(summary.namePoolSummary.blockSize, storage.GetNamePoolBlockSize());
			SF_ASSERT_EQ(summary.nodePoolSummary.allocatedBytes, storage.GetNodePoolAllocatedBytes());
			SF_ASSERT_EQ(summary.nodePoolSummary.usedBytes, storage.GetNodePoolUsedBytes());
			SF_ASSERT_EQ(summary.nodePoolSummary.blocksCount, storage.GetNodePoolBlocksCount());
			SF_ASSERT_EQ(summary.nodePoolSummary.blockSize, storage.GetNodePoolBlockSize());
			SF_ASSERT_EQ(summary.nodePoolSummary.liveNodesCount, storage.GetNodePoolLiveNodesCount());
		}
	}

	SF_TEST(file_tree_scan_coordinator, ProcessNextReturnsEmptyWhenNoJobs)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ApplyResult result = coordinator.ProcessNext();
		Summary summary = coordinator.GetSummary();

		AssertNoJob(result);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		AssertSummaryMatchesStorage(summary, storage);
		SF_ASSERT_EQ(summary.totalLogicalSize, DefaultFileSize);
		SF_ASSERT_EQ(summary.totalScanElapsedTime.count(), 0);
		SF_ASSERT_EQ(summary.pendingJobsPeakCount, 0);
		SF_ASSERT_EQ(summary.scanJobStatistics.emptyProcessCalls, 1);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 0);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 0);
	}

	SF_TEST(file_tree_scan_coordinator, UnknownJobIsRejectedWithoutScanning)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);
		Job job;
		job.input = MakeRootScanInput(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 1);

		coordinator.ScheduleJob(std::move(job));
		ApplyResult result = coordinator.ProcessNext();
		Summary summary = coordinator.GetSummary();

		AssertRejected(result);
		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		SF_ASSERT_EQ(summary.totalScanElapsedTime.count(), 0);
		SF_ASSERT_EQ(summary.pendingJobsPeakCount, 1);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 0);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 1);
	}

	SF_TEST(file_tree_scan_coordinator, NullAttachAndReplaceTargetsAreRejectedWithoutScanning)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);
		ScanInput input = MakeRootScanInput(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 1);

		coordinator.ScheduleJob(Planner::PlanAttachScan(input, nullptr));
		coordinator.ScheduleJob(Planner::PlanReplaceScan(input, nullptr));

		AssertRejected(coordinator.ProcessNext());
		AssertRejected(coordinator.ProcessNext());
		Summary summary = coordinator.GetSummary();

		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		SF_ASSERT_EQ(summary.totalScanElapsedTime.count(), 0);
		SF_ASSERT_EQ(summary.pendingJobsPeakCount, 2);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 0);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 2);
	}

	SF_TEST(file_tree_scan_coordinator, ScheduleJobAdoptsRoot)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 1);
		ApplyResult result = coordinator.ProcessNext();
		AssertApplied(result);
		const AppliedChange& appliedChange = result.appliedChange.value();

		SF_ASSERT_EQ(appliedChange.type, ChangeType::AdoptRoot);
		SF_ASSERT_EQ(appliedChange.target == nullptr, true);
		SF_ASSERT_EQ(appliedChange.addedRoot != nullptr, true);
		SF_ASSERT_EQ(appliedChange.addedNodesCount, 4);
		SF_ASSERT_EQ(appliedChange.removedNodesCount, 0);
		SF_ASSERT_EQ(storage.GetRoot() == appliedChange.addedRoot, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 4);

		const Node* root = storage.GetRoot();
		AssertNameEquals(*root, "root");
		SF_ASSERT_EQ(root->entryType, EntryType::Directory);
		SF_ASSERT_EQ(root->entryStatus, EntryStatus::Accessible);
		SF_ASSERT_EQ(root->scanStatus, EntryScanStatus::Partial);
		SF_ASSERT_EQ(CountChildren(*root), 3);

		Node* firstDirectory = RequireChild(*root, "sub_directory_1");
		Node* secondDirectory = RequireChild(*root, "sub_directory_2");
		SF_ASSERT_EQ(firstDirectory->scanStatus, EntryScanStatus::Pending);
		SF_ASSERT_EQ(secondDirectory->scanStatus, EntryScanStatus::Pending);
	}

	SF_TEST(file_tree_scan_coordinator, PendingRootSchedulesRootReplacement)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 0);
		ApplyResult rootResult = coordinator.ProcessNext();
		AssertApplied(rootResult);
		const AppliedChange& rootChange = rootResult.appliedChange.value();
		SF_ASSERT_EQ(rootChange.type, ChangeType::AdoptRoot);
		SF_ASSERT_EQ(rootChange.addedNodesCount, 1);
		SF_ASSERT_EQ(rootChange.removedNodesCount, 0);

		Node* oldRoot = rootChange.addedRoot;
		SF_ASSERT_EQ(oldRoot != nullptr, true);
		SF_ASSERT_EQ(storage.GetRoot() == oldRoot, true);
		SF_ASSERT_EQ(oldRoot->scanStatus, EntryScanStatus::Pending);
		SF_ASSERT_EQ(storage.GetNodesCount(), 1);

		ApplyResult replacementResult = coordinator.ProcessNext();
		AssertApplied(replacementResult);
		const AppliedChange& replacementChange = replacementResult.appliedChange.value();

		SF_ASSERT_EQ(replacementChange.type, ChangeType::Replace);
		SF_ASSERT_EQ(replacementChange.target == oldRoot, true);
		SF_ASSERT_EQ(replacementChange.addedRoot != nullptr, true);
		SF_ASSERT_EQ(replacementChange.addedRoot == oldRoot, false);
		SF_ASSERT_EQ(replacementChange.addedNodesCount, 8);
		SF_ASSERT_EQ(replacementChange.removedNodesCount, 1);
		SF_ASSERT_EQ(storage.GetRoot() == replacementChange.addedRoot, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 8);
		SF_ASSERT_EQ(CountPendingDirectories(storage.GetRoot()), 0);
		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);
	}

	SF_TEST(file_tree_scan_coordinator, RemoveJobDoesNotRunScannerOrScheduleFollowUpJobs)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 3);
		AssertApplied(coordinator.ProcessNext());
		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);

		Node* root = storage.GetRoot();
		const auto scanElapsedBeforeRemove = coordinator.GetSummary().totalScanElapsedTime;
		Job removeJob;
		removeJob.input = MakeRootScanInput(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH, 1);
		removeJob.applyAs = ChangeType::Remove;
		removeJob.target = root;

		coordinator.ScheduleJob(std::move(removeJob));
		ApplyResult result = coordinator.ProcessNext();
		Summary summary = coordinator.GetSummary();

		AssertApplied(result);
		SF_ASSERT_EQ(result.appliedChange->type, ChangeType::Remove);
		SF_ASSERT_EQ(result.appliedChange->target == root, true);
		SF_ASSERT_EQ(result.appliedChange->addedRoot == nullptr, true);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);
		SF_ASSERT_EQ(summary.totalScanElapsedTime.count(), scanElapsedBeforeRemove.count());
		SF_ASSERT_EQ(summary.pendingJobsPeakCount, 1);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 2);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 0);
	}

	SF_TEST(file_tree_scan_coordinator, NullAndDetachedRemoveTargetsAreRejectedWithoutScanning)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);
		Node detachedTarget;
		Job nullTargetJob;
		nullTargetJob.input = MakeRootScanInput(SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH, 1);
		nullTargetJob.applyAs = ChangeType::Remove;
		Job detachedTargetJob = nullTargetJob;
		detachedTargetJob.target = &detachedTarget;

		coordinator.ScheduleJob(std::move(nullTargetJob));
		coordinator.ScheduleJob(std::move(detachedTargetJob));

		AssertRejected(coordinator.ProcessNext());
		AssertRejected(coordinator.ProcessNext());
		Summary summary = coordinator.GetSummary();

		SF_ASSERT_EQ(coordinator.HasScheduledJobs(), false);
		SF_ASSERT_EQ(storage.GetRoot() == nullptr, true);
		SF_ASSERT_EQ(storage.GetNodesCount(), 0);
		SF_ASSERT_EQ(summary.totalScanElapsedTime.count(), 0);
		SF_ASSERT_EQ(summary.pendingJobsPeakCount, 2);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 0);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 2);
	}

	SF_TEST(file_tree_scan_coordinator, ProcessNextAfterRootScanProcessesScheduledPendingDirectory)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 1);
		AssertApplied(coordinator.ProcessNext());

		ApplyResult result = coordinator.ProcessNext();
		AssertApplied(result);
		const AppliedChange& appliedChange = result.appliedChange.value();

		SF_ASSERT_EQ(appliedChange.type, ChangeType::Replace);
		SF_ASSERT_EQ(appliedChange.target != nullptr, true);
		SF_ASSERT_EQ(appliedChange.addedRoot != nullptr, true);
		SF_ASSERT_EQ(appliedChange.addedNodesCount >= 1, true);
		SF_ASSERT_EQ(appliedChange.removedNodesCount, 1);
		SF_ASSERT_EQ(storage.GetNodesCount() > 4, true);
		SF_ASSERT_EQ(CountPendingDirectories(storage.GetRoot()) >= 1, true);
	}

	SF_TEST(file_tree_scan_coordinator, ProcessNextContinuesAfterRejectedScan)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH, 1);
		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 1);

		AssertRejected(coordinator.ProcessNext());
		AssertApplied(coordinator.ProcessNext());
	}

	SF_TEST(file_tree_scan_coordinator, ProcessesPendingDirectoriesUntilQueueIsEmpty)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 1);
		AssertApplied(coordinator.ProcessNext());

		std::size_t processedFollowUpJobs = 0;
		while (coordinator.HasScheduledJobs()) {
			AssertApplied(coordinator.ProcessNext());
			++processedFollowUpJobs;
		}

		SF_ASSERT_EQ(processedFollowUpJobs, 2);
		SF_ASSERT_EQ(storage.GetNodesCount(), 8);
		SF_ASSERT_EQ(CountPendingDirectories(storage.GetRoot()), 0);

		const Node* root = storage.GetRoot();
		Node* firstDirectory = RequireChild(*root, "sub_directory_1");
		Node* secondDirectory = RequireChild(*root, "sub_directory_2");
		Node* nestedDirectory = RequireChild(*secondDirectory, "sub_directory_3");

		SF_ASSERT_EQ(root->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(firstDirectory->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(secondDirectory->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(nestedDirectory->scanStatus, EntryScanStatus::Complete);
		SF_ASSERT_EQ(CountChildren(*firstDirectory), 2);
		SF_ASSERT_EQ(CountChildren(*secondDirectory), 1);
		SF_ASSERT_EQ(CountChildren(*nestedDirectory), 1);

		const Summary summary = coordinator.GetSummary();
		AssertSummaryMatchesStorage(summary, storage);
		SF_ASSERT_EQ(summary.totalScanElapsedTime.count() >= 0, true);
		SF_ASSERT_EQ(summary.pendingJobsPeakCount, 2);
		SF_ASSERT_EQ(summary.scanJobStatistics.emptyProcessCalls, 0);
		SF_ASSERT_EQ(summary.scanJobStatistics.appliedJobCount, 3);
		SF_ASSERT_EQ(summary.scanJobStatistics.rejectedJobCount, 0);
	}

	SF_TEST(file_tree_scan_coordinator, ScanDepthControlsScheduledFollowUpJobs)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT, 2);
		AssertApplied(coordinator.ProcessNext());

		std::size_t processedFollowUpJobs = 0;
		while (coordinator.HasScheduledJobs()) {
			AssertApplied(coordinator.ProcessNext());
			++processedFollowUpJobs;
		}

		SF_ASSERT_EQ(processedFollowUpJobs, 1);
		SF_ASSERT_EQ(storage.GetNodesCount(), 8);
		SF_ASSERT_EQ(CountPendingDirectories(storage.GetRoot()), 0);
		SF_ASSERT_EQ(storage.GetRoot()->scanStatus, EntryScanStatus::Complete);
	}

	SF_TEST(file_tree_scan_coordinator, MissingRootPathDoesNotScheduleFollowUpJobs)
	{
		Storage storage;
		Coordinator coordinator = MakeCoordinator(storage);

		ScheduleRootJob(coordinator, SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH, 1);
		ApplyResult result = coordinator.ProcessNext();
		AssertRejected(result);
		AssertNoJob(coordinator.ProcessNext());
		AssertNoJob(coordinator.ProcessNext());
	}
}
