cmake_minimum_required(VERSION 3.24)

project(space-fossils LANGUAGES CXX)

add_library (space_fossils_core STATIC)
add_executable (space_fossils_app)
add_executable (space_fossils_tests)

target_sources(space_fossils_core
	PRIVATE
		core/core.cxx
		core/size_formatter.cxx
		
		core/include/space_fossils/core.hxx
		core/include/space_fossils/size_formatter.hxx
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

enable_testing()

add_test(
	NAME space_fossils_tests
	COMMAND space_fossils_tests
)