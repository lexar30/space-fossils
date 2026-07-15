cmake_minimum_required(VERSION 3.24)

project(space-fossils LANGUAGES CXX)

enable_testing()

add_subdirectory(core)
add_subdirectory(cli)
add_subdirectory(app)
add_subdirectory(tests)
