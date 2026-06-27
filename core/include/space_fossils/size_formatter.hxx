#pragma once

#include <cstdint>
#include <string>
#include <cstddef>

namespace space_fossils::core {

	inline constexpr std::size_t DefaultDecimalPlaces = 1;
	inline constexpr std::size_t MaxDecimalPlaces = 6;

	enum class FileSizeUnitSystem
	{
		  Binary
		, Decimal
	};

	std::string FormatFileSize(std::uint64_t fileSize
		, FileSizeUnitSystem unitFormatStyle = FileSizeUnitSystem::Binary
		, std::size_t decimalPlaces = DefaultDecimalPlaces
	);
}
