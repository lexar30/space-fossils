cmake_minimum_required(VERSION 3.24)

project(space-fossils LANGUAGES CXX)

add_library (space_fossils_core STATIC)
add_executable (space_fossils_app)
add_executable (space_fossils_tests)

file(GLOB SPACE_FOSSILS_TEST_SOURCE_FILES
	CONFIGURE_DEPENDS
		tests/*_tests.cxx
)

list(REMOVE_ITEM SPACE_FOSSILS_TEST_SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/main_tests.cxx"
)

target_sources(space_fossils_core
	PRIVATE
		core/memory/memory_arena.cxx
		core/file_tree/name_pool.cxx
		core/file_tree/node_pool.cxx
		core/file_tree/storage.cxx
		core/file_tree/scanner.cxx
		core/file_tree/scan_scheduler.cxx
		core/file_tree/scan_coordinator.cxx
		core/file_tree/snapshot_reader.cxx
		core/file_tree/snapshot_writer.cxx
		core/file_tree/text_report_writer.cxx
		core/size_formatter.cxx
		core/operation_timer.cxx

		core/include/space_fossils/version.hxx
		core/include/space_fossils/memory/memory_arena.hxx
		core/include/space_fossils/file_tree/name_pool.hxx
		core/include/space_fossils/file_tree/node_pool.hxx
		core/include/space_fossils/file_tree/change_type.hxx
		core/include/space_fossils/file_tree/scan_request.hxx
		core/include/space_fossils/file_tree/scan_job.hxx
		core/include/space_fossils/file_tree/storage.hxx
		core/include/space_fossils/file_tree/storage_change.hxx
		core/include/space_fossils/file_tree/tree_pool_bundle.hxx
		core/include/space_fossils/file_tree/scanner.hxx
		core/include/space_fossils/file_tree/scan_scheduler.hxx
		core/include/space_fossils/file_tree/scan_coordinator.hxx
		core/include/space_fossils/file_tree/text_report_writer.hxx
		core/include/space_fossils/file_tree/snapshot_reader.hxx
		core/include/space_fossils/file_tree/snapshot_writer.hxx
		core/include/space_fossils/core.hxx
		core/include/space_fossils/size_formatter.hxx
		core/include/space_fossils/operation_timer.hxx
		core/include/space_fossils/scan_summary.hxx
		core/include/space_fossils/file_tree/types.hxx
		core/include/space_fossils/file_tree/node.hxx
)

target_include_directories(space_fossils_core
	PUBLIC
		core/include
)

target_compile_features(space_fossils_core
	PUBLIC
		cxx_std_20
)

target_sources(space_fossils_app
	PRIVATE
		app/cli.cxx
		app/cli.hxx
		app/main.cxx
)

target_link_libraries(space_fossils_app
	PRIVATE
		space_fossils_core
)

target_sources(space_fossils_tests
	PRIVATE
		tests/main_tests.cxx
		tests/tests.cxx
		${SPACE_FOSSILS_TEST_SOURCE_FILES}

		tests/include/space_fossils_tests/micro_test_framework.hxx
		tests/include/space_fossils_tests/tests.hxx
)

target_include_directories(space_fossils_tests
	PRIVATE
		tests/include
)

target_link_libraries(space_fossils_tests
	PRIVATE
		space_fossils_core
)

target_compile_definitions(space_fossils_tests
	PRIVATE
		SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/file_scanner/non-empty/root"
		SPACE_FOSSILS_FILE_SCANNER_FIXTURE_TEST_FILE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/file_scanner/single-file/test_file.txt"
		SPACE_FOSSILS_FILE_SCANNER_FIXTURE_ROOT_EMPTY="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/file_scanner/empty/root"
		SPACE_FOSSILS_FILE_SCANNER_FIXTURE_INVALID_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/file_scanner/invalid-path/root"
)

enable_testing()

foreach(test_source_file IN LISTS SPACE_FOSSILS_TEST_SOURCE_FILES)
	get_filename_component(test_source_name "${test_source_file}" NAME_WE)
	string(REGEX REPLACE "_tests$" "" test_suite_name "${test_source_name}")

	add_test(
		NAME "space_fossils_${test_suite_name}_tests"
		COMMAND space_fossils_tests "${test_suite_name}"
	)
endforeach()
