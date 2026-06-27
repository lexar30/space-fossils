#include "space_fossils_tests/tests.hxx"

#include <algorithm>
#include <iostream>
#include <string_view>
#include <vector>

namespace space_fossils::tests {
	namespace {
		std::vector<TestCase>& GetMutableRegistry()
		{
			static std::vector<TestCase> registry;
			return registry;
		}

		void RunTestCase(const TestCase& testCase)
		{
			std::cout << "[ = START  " << testCase.suite << "." << testCase.name << " = ] \n";
			std::cout << "[   RUN    " << testCase.suite << "." << testCase.name << "   ] \n";
			testCase.function();
			std::cout << "[      OK  " << testCase.suite << "." << testCase.name << "   ] \n";
			std::cout << "[ = END    " << testCase.suite << "." << testCase.name << " = ] \n";
			std::cout << "------------------------------------" << "\n\n";
		}
	}

	void RegisterTest(std::string_view suite, std::string_view name, TestFunction function)
	{
		GetMutableRegistry().push_back(TestCase{ suite, name, function });
	}

	const std::vector<TestCase>& GetRegisteredTests()
	{
		return GetMutableRegistry();
	}

	void RunAllTests()
	{
		for (const TestCase& testCase : GetRegisteredTests()) {
			RunTestCase(testCase);
		}
	}

	bool RunTestSuite(std::string_view suite)
	{
		bool foundSuite = false;

		for (const TestCase& testCase : GetRegisteredTests()) {
			if (testCase.suite == suite) {
				foundSuite = true;
				RunTestCase(testCase);
			}
		}

		return foundSuite;
	}

	void PrintAvailableTestSuites(std::ostream& output)
	{
		std::vector<std::string_view> suites;

		for (const TestCase& testCase : GetRegisteredTests()) {
			if (std::find(suites.begin(), suites.end(), testCase.suite) == suites.end()) {
				suites.push_back(testCase.suite);
			}
		}

		output << "Available test suites: all";
		for (std::string_view suite : suites) {
			output << ", " << suite;
		}
		output << "\n";
	}
}
