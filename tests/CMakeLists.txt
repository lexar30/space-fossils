add_executable (space_fossils_tests)

file(GLOB SPACE_FOSSILS_TEST_SOURCE_FILES
	CONFIGURE_DEPENDS
		*_tests.cxx
)

target_sources(space_fossils_tests
	PRIVATE
		main.cxx
		tests.cxx
		${SPACE_FOSSILS_TEST_SOURCE_FILES}

		include/space_fossils_tests/micro_test_framework.hxx
		include/space_fossils_tests/tests.hxx
)

target_include_directories(space_fossils_tests
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(space_fossils_tests
	PRIVATE
		space_fossils_core
		space_fossils_cli
)

target_compile_definitions(space_fossils_tests
	PRIVATE
		SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/file_scanner/non-empty/root"
		SPACE_FOSSILS_FILE_SCANNER_FIXTURE_TEST_FILE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/file_scanner/single-file/test_file.txt"
		SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT_EMPTY="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/file_scanner/empty/root"
		SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/file_scanner/invalid-path/root"
)

foreach(test_source_file IN LISTS SPACE_FOSSILS_TEST_SOURCE_FILES)
	get_filename_component(test_source_name "${test_source_file}" NAME_WE)
	string(REGEX REPLACE "_tests$" "" test_suite_name "${test_source_name}")

	add_test(
		NAME "space_fossils_${test_suite_name}_tests"
		COMMAND space_fossils_tests "${test_suite_name}"
	)
endforeach()
