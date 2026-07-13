#include "space_fossils/file_tree/scan/planner.hxx"

#include "space_fossils/file_tree/model/node.hxx"
#include "space_fossils/file_tree/storage/storage_change.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>
#include <filesystem>
#include <vector>

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

		NameRef MakeNameRef(const NativeString& value)
		{
			return NameRef{
				value.data(),
				static_cast<std::uint32_t>(value.size())
			};
		}

		ScanInput MakeInput(const char* path, std::size_t maxDepth)
		{
			ScanInput input;
			input.path = std::filesystem::path(path);
			input.maxDepth = maxDepth;

			return input;
		}

		void SetDirectory(Node& node, const NativeString& name, EntryScanStatus scanStatus)
		{
			node.name = MakeNameRef(name);
			node.entryType = EntryType::Directory;
			node.scanStatus = scanStatus;
		}

		void SetFile(Node& node, const NativeString& name)
		{
			node.name = MakeNameRef(name);
			node.entryType = EntryType::File;
			node.scanStatus = EntryScanStatus::Complete;
		}

		void AppendChild(Node& parent, Node& child)
		{
			child.parent = &parent;
			child.nextSibling = nullptr;

			if (parent.firstChild == nullptr) {
				parent.firstChild = &child;
				return;
			}

			Node* lastChild = parent.firstChild;
			while (lastChild->nextSibling != nullptr) {
				lastChild = lastChild->nextSibling;
			}

			lastChild->nextSibling = &child;
		}
	}

	SF_TEST(file_tree_scan_planner, PlanRootScanCreatesAdoptRootJob)
	{
		ScanInput input = MakeInput("root", 3);

		Job job = Planner::PlanRootScan(input);

		SF_ASSERT_EQ(job.input.path == input.path, true);
		SF_ASSERT_EQ(job.input.maxDepth, input.maxDepth);
		SF_ASSERT_EQ(job.applyAs, ChangeType::AdoptRoot);
		SF_ASSERT_EQ(job.target == nullptr, true);
	}

	SF_TEST(file_tree_scan_planner, PlanReplaceScanKeepsTarget)
	{
		Node target;
		ScanInput input = MakeInput("root/folder", 2);

		Job job = Planner::PlanReplaceScan(input, &target);

		SF_ASSERT_EQ(job.input.path == input.path, true);
		SF_ASSERT_EQ(job.input.maxDepth, input.maxDepth);
		SF_ASSERT_EQ(job.applyAs, ChangeType::Replace);
		SF_ASSERT_EQ(job.target == &target, true);
	}

	SF_TEST(file_tree_scan_planner, PlanAttachScanKeepsParentTarget)
	{
		Node parent;
		ScanInput input = MakeInput("root/new-child", 1);

		Job job = Planner::PlanAttachScan(input, &parent);

		SF_ASSERT_EQ(job.input.path == input.path, true);
		SF_ASSERT_EQ(job.input.maxDepth, input.maxDepth);
		SF_ASSERT_EQ(job.applyAs, ChangeType::Attach);
		SF_ASSERT_EQ(job.target == &parent, true);
	}

	SF_TEST(file_tree_scan_planner, PlanFollowUpJobsReturnsEmptyForInvalidInputs)
	{
		Job completedJob = Planner::PlanRootScan(MakeInput("root", 1));
		PlanningConfig config;
		config.jobDepth = 1;

		AppliedChange emptyChange;
		SF_ASSERT_EQ(Planner::PlanFollowUpJobs(completedJob, emptyChange, config).empty(), true);

		Node root;
		NativeString rootName = MakeNativeString("root");
		SetDirectory(root, rootName, EntryScanStatus::Pending);

		AppliedChange unknownChange;
		unknownChange.addedRoot = &root;
		SF_ASSERT_EQ(Planner::PlanFollowUpJobs(completedJob, unknownChange, config).empty(), true);

		AppliedChange validChange;
		validChange.type = ChangeType::AdoptRoot;
		validChange.addedRoot = &root;

		PlanningConfig zeroDepthConfig;
		zeroDepthConfig.jobDepth = 0;
		SF_ASSERT_EQ(Planner::PlanFollowUpJobs(completedJob, validChange, zeroDepthConfig).empty(), true);

		Job emptyPathJob = completedJob;
		emptyPathJob.input.path.clear();
		SF_ASSERT_EQ(Planner::PlanFollowUpJobs(emptyPathJob, validChange, config).empty(), true);
	}

	SF_TEST(file_tree_scan_planner, PlanFollowUpJobsSchedulesPendingRootOnly)
	{
		NativeString rootName = MakeNativeString("root");
		Node root;
		SetDirectory(root, rootName, EntryScanStatus::Pending);

		Job completedJob = Planner::PlanRootScan(MakeInput("root", 0));
		AppliedChange change;
		change.type = ChangeType::AdoptRoot;
		change.addedRoot = &root;

		PlanningConfig config;
		config.jobDepth = 2;

		std::vector<Job> jobs = Planner::PlanFollowUpJobs(completedJob, change, config);

		SF_ASSERT_EQ(jobs.size(), 1);
		SF_ASSERT_EQ(jobs[0].input.path == completedJob.input.path, true);
		SF_ASSERT_EQ(jobs[0].input.maxDepth, config.jobDepth);
		SF_ASSERT_EQ(jobs[0].applyAs, ChangeType::Replace);
		SF_ASSERT_EQ(jobs[0].target == &root, true);
	}

	SF_TEST(file_tree_scan_planner, PlanFollowUpJobsSchedulesPendingDirectoriesWithResolvedPaths)
	{
		NativeString rootName = MakeNativeString("root");
		NativeString firstName = MakeNativeString("first");
		NativeString secondName = MakeNativeString("second");
		NativeString fileName = MakeNativeString("file.txt");
		NativeString nestedName = MakeNativeString("nested");

		Node root;
		SetDirectory(root, rootName, EntryScanStatus::Complete);

		Node first;
		SetDirectory(first, firstName, EntryScanStatus::Pending);
		AppendChild(root, first);

		Node second;
		SetDirectory(second, secondName, EntryScanStatus::Partial);
		AppendChild(root, second);

		Node file;
		SetFile(file, fileName);
		AppendChild(root, file);

		Node nested;
		SetDirectory(nested, nestedName, EntryScanStatus::Pending);
		AppendChild(second, nested);

		Job completedJob = Planner::PlanRootScan(MakeInput("root", 1));
		AppliedChange change;
		change.type = ChangeType::AdoptRoot;
		change.addedRoot = &root;

		PlanningConfig config;
		config.jobDepth = 3;

		std::vector<Job> jobs = Planner::PlanFollowUpJobs(completedJob, change, config);

		SF_ASSERT_EQ(jobs.size(), 2);
		SF_ASSERT_EQ(jobs[0].input.path == std::filesystem::path("root") / firstName, true);
		SF_ASSERT_EQ(jobs[0].input.maxDepth, config.jobDepth);
		SF_ASSERT_EQ(jobs[0].applyAs, ChangeType::Replace);
		SF_ASSERT_EQ(jobs[0].target == &first, true);

		SF_ASSERT_EQ(jobs[1].input.path == std::filesystem::path("root") / secondName / nestedName, true);
		SF_ASSERT_EQ(jobs[1].input.maxDepth, config.jobDepth);
		SF_ASSERT_EQ(jobs[1].applyAs, ChangeType::Replace);
		SF_ASSERT_EQ(jobs[1].target == &nested, true);
	}
}
