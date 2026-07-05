#include "space_fossils/file_tree/scan_scheduler.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <filesystem>
#include <optional>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;

		ScanJob MakeJob(const char* path, std::size_t maxDepth, IncomingChangeType applyAs, Node* target = nullptr)
		{
			ScanJob job;
			job.request.path = std::filesystem::path(path);
			job.request.maxDepth = maxDepth;
			job.applyAs = applyAs;
			job.target = target;

			return job;
		}

		void AssertJobEquals(const ScanJob& actual, const ScanJob& expected)
		{
			SF_ASSERT_EQ(actual.request.path == expected.request.path, true);
			SF_ASSERT_EQ(actual.request.maxDepth, expected.request.maxDepth);
			SF_ASSERT_EQ(actual.applyAs, expected.applyAs);
			SF_ASSERT_EQ(actual.target == expected.target, true);
		}
	}

	SF_TEST(file_tree_scan_scheduler, StartsEmpty)
	{
		ScanScheduler scheduler;

		SF_ASSERT_EQ(scheduler.HasJobs(), false);
		SF_ASSERT_EQ(scheduler.PopNext().has_value(), false);
	}

	SF_TEST(file_tree_scan_scheduler, ScheduleMakesJobAvailable)
	{
		ScanScheduler scheduler;
		ScanJob expected = MakeJob("root", 1, IncomingChangeType::AdoptRoot);

		scheduler.Schedule(expected);

		SF_ASSERT_EQ(scheduler.HasJobs(), true);
		std::optional<ScanJob> actual = scheduler.PopNext();

		SF_ASSERT_EQ(actual.has_value(), true);
		AssertJobEquals(actual.value(), expected);
		SF_ASSERT_EQ(scheduler.HasJobs(), false);
		SF_ASSERT_EQ(scheduler.PopNext().has_value(), false);
	}

	SF_TEST(file_tree_scan_scheduler, PopNextPreservesFifoOrder)
	{
		ScanScheduler scheduler;
		Node target;
		ScanJob first = MakeJob("first", 1, IncomingChangeType::AdoptRoot);
		ScanJob second = MakeJob("second", 2, IncomingChangeType::Replace, &target);
		ScanJob third = MakeJob("third", 3, IncomingChangeType::Attach, &target);

		scheduler.Schedule(first);
		scheduler.Schedule(second);
		scheduler.Schedule(third);

		std::optional<ScanJob> firstActual = scheduler.PopNext();
		std::optional<ScanJob> secondActual = scheduler.PopNext();
		std::optional<ScanJob> thirdActual = scheduler.PopNext();

		SF_ASSERT_EQ(firstActual.has_value(), true);
		SF_ASSERT_EQ(secondActual.has_value(), true);
		SF_ASSERT_EQ(thirdActual.has_value(), true);
		AssertJobEquals(firstActual.value(), first);
		AssertJobEquals(secondActual.value(), second);
		AssertJobEquals(thirdActual.value(), third);
		SF_ASSERT_EQ(scheduler.HasJobs(), false);
	}

	SF_TEST(file_tree_scan_scheduler, ClearRemovesScheduledJobs)
	{
		ScanScheduler scheduler;
		scheduler.Schedule(MakeJob("first", 1, IncomingChangeType::AdoptRoot));
		scheduler.Schedule(MakeJob("second", 1, IncomingChangeType::Replace));

		scheduler.Clear();

		SF_ASSERT_EQ(scheduler.HasJobs(), false);
		SF_ASSERT_EQ(scheduler.PopNext().has_value(), false);
	}
}
