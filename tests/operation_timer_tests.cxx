#include "space_fossils/core/operation_timer.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

namespace space_fossils::tests {
	namespace {
		auto DurationCount(space_fossils::core::MetricsDuration duration)
		{
			return duration.count();
		}
	}

	SF_TEST(operation_timer, DefaultTimerIsStoppedWithZeroElapsed)
	{
		space_fossils::core::OperationTimer timer;

		SF_ASSERT_EQ(timer.IsRunning(), false);
		SF_ASSERT_EQ(DurationCount(timer.Elapsed()), 0);
	}

	SF_TEST(operation_timer, StopBeforeStartKeepsTimerStoppedWithZeroElapsed)
	{
		space_fossils::core::OperationTimer timer;

		timer.Stop();

		SF_ASSERT_EQ(timer.IsRunning(), false);
		SF_ASSERT_EQ(DurationCount(timer.Elapsed()), 0);
	}

	SF_TEST(operation_timer, StartMarksTimerRunning)
	{
		space_fossils::core::OperationTimer timer;

		timer.Start();

		SF_ASSERT_EQ(timer.IsRunning(), true);
		SF_ASSERT_EQ(DurationCount(timer.Elapsed()) >= 0, true);
	}

	SF_TEST(operation_timer, StopFreezesElapsedValue)
	{
		space_fossils::core::OperationTimer timer;

		timer.Start();
		timer.Stop();

		const auto elapsedAfterStop = timer.Elapsed();
		const auto elapsedAfterSecondRead = timer.Elapsed();

		SF_ASSERT_EQ(timer.IsRunning(), false);
		SF_ASSERT_EQ(DurationCount(elapsedAfterSecondRead), DurationCount(elapsedAfterStop));
		SF_ASSERT_EQ(DurationCount(elapsedAfterStop) >= 0, true);
	}

	SF_TEST(operation_timer, SecondStopKeepsElapsedValue)
	{
		space_fossils::core::OperationTimer timer;

		timer.Start();
		timer.Stop();

		const auto elapsedAfterFirstStop = timer.Elapsed();

		timer.Stop();

		SF_ASSERT_EQ(timer.IsRunning(), false);
		SF_ASSERT_EQ(DurationCount(timer.Elapsed()), DurationCount(elapsedAfterFirstStop));
	}

	SF_TEST(operation_timer, ResetClearsElapsedAndStopsTimer)
	{
		space_fossils::core::OperationTimer timer;

		timer.Start();
		timer.Stop();
		timer.Reset();

		SF_ASSERT_EQ(timer.IsRunning(), false);
		SF_ASSERT_EQ(DurationCount(timer.Elapsed()), 0);
	}

	SF_TEST(operation_timer, ResetWhileRunningClearsElapsedAndStopsTimer)
	{
		space_fossils::core::OperationTimer timer;

		timer.Start();
		timer.Reset();

		SF_ASSERT_EQ(timer.IsRunning(), false);
		SF_ASSERT_EQ(DurationCount(timer.Elapsed()), 0);
	}
}
