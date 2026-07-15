cmake_minimum_required(VERSION 3.24)

project(space-fossils LANGUAGES CXX)

add_library (space_fossils_core STATIC)
add_library (space_fossils_cli STATIC)
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
		core/file_tree/memory/name_pool.cxx
		core/file_tree/memory/node_pool.cxx
		core/file_tree/storage/storage.cxx
		core/file_tree/scan/scanner.cxx
		core/file_tree/scan/scheduler.cxx
		core/file_tree/scan/coordinator.cxx
		core/file_tree/scan/planner.cxx
		core/file_tree/scan/operations.cxx
		core/file_tree/snapshot/reader.cxx
		core/file_tree/snapshot/writer.cxx
		core/file_tree/snapshot/operations.cxx
		core/file_tree/report/text_writer.cxx
		core/file_tree/query/tree_query.cxx
		core/file_tree/session/session.cxx
		core/file_tree/session/session_operations.cxx
		core/size_formatter.cxx
		core/operation_timer.cxx

		core/include/space_fossils/core/version.hxx
		core/include/space_fossils/core/memory/memory_arena.hxx
		core/include/space_fossils/core/file_tree/memory/name_pool.hxx
		core/include/space_fossils/core/file_tree/memory/node_pool.hxx
		core/include/space_fossils/core/file_tree/model/default_constants.hxx
		core/include/space_fossils/core/file_tree/change_type.hxx
		core/include/space_fossils/core/file_tree/scan/planning_config.hxx
		core/include/space_fossils/core/file_tree/scan/scan_input.hxx
		core/include/space_fossils/core/file_tree/scan/job.hxx
		core/include/space_fossils/core/file_tree/scan/apply_result.hxx
		core/include/space_fossils/core/file_tree/storage/storage.hxx
		core/include/space_fossils/core/file_tree/storage/storage_change.hxx
		core/include/space_fossils/core/file_tree/model/tree_pool_bundle.hxx
		core/include/space_fossils/core/file_tree/scan/scanner.hxx
		core/include/space_fossils/core/file_tree/scan/scheduler.hxx
		core/include/space_fossils/core/file_tree/scan/coordinator.hxx
		core/include/space_fossils/core/file_tree/scan/planner.hxx
		core/include/space_fossils/core/file_tree/scan/operations.hxx
		core/include/space_fossils/core/file_tree/report/text_writer.hxx
		core/include/space_fossils/core/file_tree/snapshot/reader.hxx
		core/include/space_fossils/core/file_tree/snapshot/writer.hxx
		core/include/space_fossils/core/file_tree/snapshot/operations.hxx
		core/include/space_fossils/core/file_tree/query/tree_query.hxx
		core/include/space_fossils/core/file_tree/session/session.hxx
		core/include/space_fossils/core/file_tree/session/session_operations.hxx
		core/include/space_fossils/core/core.hxx
		core/include/space_fossils/core/size_formatter.hxx
		core/include/space_fossils/core/operation_timer.hxx
		core/include/space_fossils/core/file_tree/scan/summary.hxx
		core/include/space_fossils/core/file_tree/model/types.hxx
		core/include/space_fossils/core/file_tree/model/node.hxx
)

target_include_directories(space_fossils_core
	PUBLIC
		core/include
)

target_compile_features(space_fossils_core
	PUBLIC
		cxx_std_20
)

target_sources(space_fossils_cli
	PRIVATE
		cli/app_state.cxx
		cli/cli_app.cxx
		cli/cli_renderer.cxx
		cli/command_dispatcher.cxx
		cli/command_parser.cxx

		cli/include/space_fossils/cli/app_state.hxx
		cli/include/space_fossils/cli/cli_app.hxx
		cli/include/space_fossils/cli/cli_renderer.hxx
		cli/include/space_fossils/cli/command.hxx
		cli/include/space_fossils/cli/command_dispatcher.hxx
		cli/include/space_fossils/cli/command_parser.hxx
		cli/include/space_fossils/cli/command_result.hxx
		cli/include/space_fossils/cli/command_spec.hxx
		cli/include/space_fossils/cli/command_type.hxx
		cli/include/space_fossils/cli/parse_error.hxx
		cli/include/space_fossils/cli/parsed_command.hxx
		cli/include/space_fossils/cli/scan_command_options.hxx
)

target_include_directories(space_fossils_cli
	PUBLIC
		cli/include
)

target_link_libraries(space_fossils_cli
	PUBLIC
		space_fossils_core
)

target_sources(space_fossils_app
	PRIVATE
		app/main.cxx
)

target_link_libraries(space_fossils_app
	PRIVATE
		space_fossils_cli
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
		space_fossils_cli
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
