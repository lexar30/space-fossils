#pragma once

#include "space_fossils/file_tree/default_constants.hxx"

#include <cstddef>
#include <filesystem>

namespace space_fossils::core::file_tree {
	struct ScanRequest
	{
		std::filesystem::path path;
		std::size_t maxDepth = DefaultScanDepth;
	};
}
