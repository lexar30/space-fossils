#pragma once

#include <array>
#include <cstdint>

namespace space_fossils::core::file_tree::snapshot {
	inline constexpr std::array<char, 4> MagicBytes{ 'S', 'F', 'V', 'B' };

	inline constexpr std::uint64_t ApplicationVersionLengthLimit = 32;

	// Prevents excessive allocations, covers:
	// - Windows extended paths (~32K UTF-16 code units, 64 KiB).
	// - Linux PATH_MAX (4096 bytes).
	inline constexpr std::uint64_t SourcePathBytesLengthLimit = 64 * 1024;
}
