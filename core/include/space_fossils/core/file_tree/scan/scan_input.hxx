#pragma once

#include "space_fossils/core/file_tree/model/default_constants.hxx"

#include <cstddef>
#include <filesystem>

namespace space_fossils::core::file_tree::scan {
	struct ScanInput
	{
		std::filesystem::path path;
		std::size_t maxDepth = DefaultScanDepth;
	};
}
