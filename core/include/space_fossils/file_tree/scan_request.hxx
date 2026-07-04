#pragma once

#include <cstddef>
#include <filesystem>

namespace space_fossils::core::file_tree {
	struct ScanRequest
	{
		std::filesystem::path path;
		std::size_t maxDepth = 1;
	};
}