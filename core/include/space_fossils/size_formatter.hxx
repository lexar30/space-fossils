#pragma once

#include <cstdint>
#include <string>

namespace space_fossils::core {

	inline constexpr std::size_t DefaultDecimalPlaces = 1;

	enum class UnitFormatStyle
	{
		  Binary
		, Decimal
	};

	std::string FormatFileSize(std::uint64_t fileSize
		, UnitFormatStyle unitFormatStyle = UnitFormatStyle::Binary
		, std::size_t decimalPlaces = DefaultDecimalPlaces
	);
}