#include "space_fossils_tests/tests.hxx"

#include <iostream>
#include <string_view>

int main(int argc, char* argv[])
{
	if (argc == 1) {
		space_fossils::tests::RunAllTests();
		return 0;
	}

	const std::string_view testSuite = argv[1];

	if (testSuite == "all") {
		space_fossils::tests::RunAllTests();
	}
	else if (space_fossils::tests::RunTestSuite(testSuite)) { }
	else {
		std::cerr << "Unknown test suite: " << testSuite << "\n";
		space_fossils::tests::PrintAvailableTestSuites(std::cerr);
		return 1;
	}

	return 0;
}
