#pragma once

#include "space_fossils/core/file_tree/model/default_constants.hxx"

#include <cstddef>

namespace space_fossils::core::file_tree::scan {
	struct PlanningConfig
	{
		// TODO: temp, initial - DefaultScanDepth
		std::size_t jobDepth = UnlimitedScanDepth;
	};
}
