#pragma once

#include "space_fossils/core/file_tree/model/node.hxx"

#include <cstddef>
#include <limits>

namespace space_fossils::core::file_tree {
	inline constexpr std::size_t DefaultNameBlockSize = sizeof(NativeChar) * 4096;
	inline constexpr std::size_t DefaultNodeBlockSize = sizeof(Node) * 1024;
	inline constexpr std::size_t DefaultScanDepth = 1;
	inline constexpr std::size_t UnlimitedScanDepth = std::numeric_limits<std::size_t>::max();
}
