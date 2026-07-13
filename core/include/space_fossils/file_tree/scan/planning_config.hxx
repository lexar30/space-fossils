#pragma once

#include "space_fossils/file_tree/model/default_constants.hxx"

#include <cstddef>

namespace space_fossils::core::file_tree::scan {
	struct PlanningConfig
	{
		std::size_t jobDepth = DefaultScanDepth;
	};
}
