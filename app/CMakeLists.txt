add_executable (space_fossils_app)

target_sources(space_fossils_app
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/main.cxx
)

target_link_libraries(space_fossils_app
	PRIVATE
		space_fossils_cli
)
