#pragma once

#include "space_fossils/file_tree/types.hxx"

namespace space_fossils::app {
	using CliChar = space_fossils::core::file_tree::NativeChar;

	int RunCli(int argc, CliChar* argv[]);
}
