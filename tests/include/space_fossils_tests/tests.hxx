#pragma once

#include <iosfwd>
#include <string_view>
#include <vector>

namespace space_fossils::tests {

	using TestFunction = void (*)();

	struct TestCase
	{
		std::string_view suite;
		std::string_view name;
		TestFunction function = nullptr;
	};

	void RegisterTest(std::string_view suite, std::string_view name, TestFunction function);
	const std::vector<TestCase>& GetRegisteredTests();

	void RunAllTests();
	bool RunTestSuite(std::string_view suite);
	void PrintAvailableTestSuites(std::ostream& output);


}
