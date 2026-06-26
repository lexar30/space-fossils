cmake_minimum_required(VERSION 3.24)

project(space-fossils LANGUAGES CXX)

add_library (space_fossils_core STATIC)
add_executable (space_fossils_app)
add_executable (space_fossils_tests)

target_sources(space_fossils_core
	PRIVATE
		src/core.cxx
)

target_include_directories(space_fossils_core
	PUBLIC
		src/include
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
)

target_link_libraries(space_fossils_tests
	PRIVATE
		space_fossils_core
)