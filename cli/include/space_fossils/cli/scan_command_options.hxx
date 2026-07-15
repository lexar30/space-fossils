#pragma once

#include "space_fossils/file_tree/model/default_constants.hxx"

#include <cstddef>
#include <filesystem>

namespace space_fossils::cli {
	struct ScanCommandOptions
	{
		std::filesystem::path inputPath;
		std::filesystem::path treeOutputPath;
		std::size_t depth = space_fossils::core::file_tree::DefaultScanDepth;
	};
}
