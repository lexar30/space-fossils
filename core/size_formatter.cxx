#include "space_fossils/size_formatter.hxx"
#include <array>
#include <limits>


namespace space_fossils::core {
	namespace {

		inline constexpr std::size_t UnitsCount = 7;

		struct UnitDefinition
		{
			const std::uint64_t divisor;
			const std::uint64_t promotionPoint;
			const std::string_view suffix;
		};

		constexpr UnitDefinition MakeUnit(
			std::uint64_t divisor,
			std::uint64_t promotionThreshold,
			std::string_view suffix
		)
		{
			return UnitDefinition{
				divisor,
				promotionThreshold * divisor,
				suffix
			};
		}

		namespace binary_units
		{
			inline constexpr std::uint64_t BinaryUnit = std::uint64_t(1024);

			inline constexpr std::uint64_t Bytes = std::uint64_t(1);
			inline constexpr std::uint64_t KiB = Bytes * BinaryUnit;
			inline constexpr std::uint64_t MiB = KiB * BinaryUnit;
			inline constexpr std::uint64_t GiB = MiB * BinaryUnit;
			inline constexpr std::uint64_t TiB = GiB * BinaryUnit;
			inline constexpr std::uint64_t PiB = TiB * BinaryUnit;
			inline constexpr std::uint64_t EiB = PiB * BinaryUnit;

			inline constexpr std::uint64_t BinaryPromotionThreshold = std::uint64_t(128);

			inline constexpr std::array<UnitDefinition, UnitsCount> Units{
					MakeUnit(Bytes,	BinaryPromotionThreshold,							"B")
				  , MakeUnit(KiB,			BinaryPromotionThreshold,					"KiB")
				  , MakeUnit(MiB,			BinaryPromotionThreshold,					"MiB")
				  , MakeUnit(GiB,			BinaryPromotionThreshold,					"GiB")
				  , MakeUnit(TiB,			BinaryPromotionThreshold,					"TiB")
				  , MakeUnit(PiB,			BinaryPromotionThreshold,					"PiB")
				  , UnitDefinition{EiB,		std::numeric_limits<std::uint64_t>::max(),	"EiB"}

			};
		}

		namespace decimal_units
		{
			inline constexpr std::uint64_t DecimalUnit = std::uint64_t(1000);

			inline constexpr std::uint64_t Bytes = std::uint64_t(1);
			inline constexpr std::uint64_t KB = Bytes * DecimalUnit;
			inline constexpr std::uint64_t MB = KB * DecimalUnit;
			inline constexpr std::uint64_t GB = MB * DecimalUnit;
			inline constexpr std::uint64_t TB = GB * DecimalUnit;
			inline constexpr std::uint64_t PB = TB * DecimalUnit;
			inline constexpr std::uint64_t EB = PB * DecimalUnit;

			inline constexpr std::uint64_t DecimalPromotionThreshold = std::uint64_t(100);

			inline constexpr std::array<UnitDefinition, UnitsCount> Units{
					MakeUnit(Bytes,		DecimalPromotionThreshold,					"B")
				  , MakeUnit(KB,		DecimalPromotionThreshold,					"KB")
				  , MakeUnit(MB,		DecimalPromotionThreshold,					"MB")
				  , MakeUnit(GB,		DecimalPromotionThreshold,					"GB")
				  , MakeUnit(TB,		DecimalPromotionThreshold,					"TB")
				  , MakeUnit(PB,		DecimalPromotionThreshold,					"PB")
				  , UnitDefinition{EB,	std::numeric_limits<std::uint64_t>::max(),	"EB"}

			};
		}
	}

	std::string FormatFileSize(std::uint64_t fileSize, space_fossils::core::UnitFormatStyle unitFormatStyle, std::size_t decimalPlaces)
	{
		std::size_t unitIndex = 0;

		const std::array<UnitDefinition, UnitsCount>& units = unitFormatStyle == UnitFormatStyle::Binary ? binary_units::Units : decimal_units::Units;

		while (unitIndex + 1 < UnitsCount) {
			if (fileSize < units.at(unitIndex).promotionPoint) {
				break;
			}

			++unitIndex;
		}

		const UnitDefinition& unitDef = units.at(unitIndex);

		if (unitIndex == 0) {
			return std::to_string(fileSize) + " " + std::string(unitDef.suffix);
		}

		const std::uint64_t wholeNumberPart = fileSize / unitDef.divisor;
		std::uint64_t remainderNumberPart = fileSize % unitDef.divisor;

		std::string result = std::to_string(wholeNumberPart);

		if (decimalPlaces > 0) {
			result += ".";

			for (std::size_t i = 0; i < decimalPlaces; ++i) {
				remainderNumberPart *= std::uint64_t(10);

				const std::uint64_t digit = remainderNumberPart / unitDef.divisor;
				result += std::to_string(digit);

				remainderNumberPart %= unitDef.divisor;
			}
		}

		result += " ";
		result += unitDef.suffix;

		return result;
	}

}