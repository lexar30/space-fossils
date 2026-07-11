#pragma once

#include <space_fossils/file_tree/default_constants.hxx>

#include <cstddef>
#include <filesystem>

namespace space_fossils::core::file_tree {
	struct ScanCoordinatorConfig
	{
		std::filesystem::path rootPath;
		std::size_t defaultScanDepth = UnlimitedScanDepth;
	};
}
