add_library (space_fossils_cli STATIC)

target_sources(space_fossils_cli
	PRIVATE
		app_state.cxx
		cli_app.cxx
		cli_renderer.cxx
		command_dispatcher.cxx
		command_parser.cxx

		include/space_fossils/cli/app_state.hxx
		include/space_fossils/cli/cli_app.hxx
		include/space_fossils/cli/cli_renderer.hxx
		include/space_fossils/cli/command.hxx
		include/space_fossils/cli/command_dispatcher.hxx
		include/space_fossils/cli/command_parser.hxx
		include/space_fossils/cli/command_result.hxx
		include/space_fossils/cli/command_spec.hxx
		include/space_fossils/cli/command_type.hxx
		include/space_fossils/cli/parse_error.hxx
		include/space_fossils/cli/parsed_command.hxx
		include/space_fossils/cli/scan_command_options.hxx
)

target_include_directories(space_fossils_cli
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(space_fossils_cli
	PUBLIC
		space_fossils_core
)
