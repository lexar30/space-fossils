#include "space_fossils/file_tree/scan/scheduler.hxx"

#include "space_fossils/file_tree/model/node.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <filesystem>
#include <optional>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;
		using namespace space_fossils::core::file_tree::scan;

		Job MakeJob(const char* path, std::size_t maxDepth, IncomingChangeType applyAs, Node* target = nullptr)
		{
			Job job;
			job.input.path = std::filesystem::path(path);
			job.input.maxDepth = maxDepth;
			job.applyAs = applyAs;
			job.target = target;

			return job;
		}

		void AssertJobEquals(const Job& actual, const Job& expected)
		{
			SF_ASSERT_EQ(actual.input.path == expected.input.path, true);
			SF_ASSERT_EQ(actual.input.maxDepth, expected.input.maxDepth);
			SF_ASSERT_EQ(actual.applyAs, expected.applyAs);
			SF_ASSERT_EQ(actual.target == expected.target, true);
		}
	}

	SF_TEST(file_tree_scan_scheduler, StartsEmpty)
	{
		Scheduler scheduler;

		SF_ASSERT_EQ(scheduler.HasJobs(), false);
		SF_ASSERT_EQ(scheduler.PopNext().has_value(), false);
	}

	SF_TEST(file_tree_scan_scheduler, ScheduleMakesJobAvailable)
	{
		Scheduler scheduler;
		Job expected = MakeJob("root", 1, IncomingChangeType::AdoptRoot);

		scheduler.Schedule(expected);

		SF_ASSERT_EQ(scheduler.HasJobs(), true);
		std::optional<Job> actual = scheduler.PopNext();

		SF_ASSERT_EQ(actual.has_value(), true);
		AssertJobEquals(actual.value(), expected);
		SF_ASSERT_EQ(scheduler.HasJobs(), false);
		SF_ASSERT_EQ(scheduler.PopNext().has_value(), false);
	}

	SF_TEST(file_tree_scan_scheduler, PopNextPreservesFifoOrder)
	{
		Scheduler scheduler;
		Node target;
		Job first = MakeJob("first", 1, IncomingChangeType::AdoptRoot);
		Job second = MakeJob("second", 2, IncomingChangeType::Replace, &target);
		Job third = MakeJob("third", 3, IncomingChangeType::Attach, &target);

		scheduler.Schedule(first);
		scheduler.Schedule(second);
		scheduler.Schedule(third);

		std::optional<Job> firstActual = scheduler.PopNext();
		std::optional<Job> secondActual = scheduler.PopNext();
		std::optional<Job> thirdActual = scheduler.PopNext();

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
		Scheduler scheduler;
		scheduler.Schedule(MakeJob("first", 1, IncomingChangeType::AdoptRoot));
		scheduler.Schedule(MakeJob("second", 1, IncomingChangeType::Replace));

		scheduler.Clear();

		SF_ASSERT_EQ(scheduler.HasJobs(), false);
		SF_ASSERT_EQ(scheduler.PopNext().has_value(), false);
	}
}
