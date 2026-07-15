#pragma once

#include <chrono>

namespace space_fossils::core {
	using MetricsClock = std::chrono::steady_clock;
	using MetricsDuration = std::chrono::nanoseconds;
	using MetricsTimePoint = MetricsClock::time_point;

	class OperationTimer
	{
	public:
		void Start();
		void Stop();
		void Reset();
		bool IsRunning() const;

		MetricsDuration Elapsed() const;

	private:
		MetricsTimePoint start = {};
		MetricsDuration elapsed = {};
		bool isRunning = false;
	};
}