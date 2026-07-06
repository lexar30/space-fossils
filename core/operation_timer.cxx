#include "space_fossils/operation_timer.hxx"

namespace space_fossils::core {
	void OperationTimer::Start()
	{
		isRunning = true;
		start = MetricsClock::now();
	}

	void OperationTimer::Stop()
	{
		if (!isRunning) {
			return;
		}

		elapsed = MetricsClock::now() - start;
		isRunning = false;
	}

	bool OperationTimer::IsRunning() const
	{
		return isRunning;
	}

	void OperationTimer::Reset()
	{
		start = {};
		elapsed = {};
		isRunning = false;
	}

	MetricsDuration OperationTimer::Elapsed() const
	{
		if (isRunning) {
			return MetricsClock::now() - start;
		}

		return elapsed;
	}
}