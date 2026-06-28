#include "space_fossils/size_formatter.hxx"

#include <array>
#include <limits>
#include <string_view>


namespace space_fossils::core {
	namespace {

		inline constexpr std::size_t UnitsCount = 7;

		struct UnitDefinition
		{
			const std::uintmax_t divisor;
			const std::uintmax_t promotionPoint;
			const std::string_view suffix;
		};

		constexpr UnitDefinition MakeUnit(
			std::uintmax_t divisor,
			std::uintmax_t promotionThreshold,
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
			inline constexpr std::uintmax_t BinaryUnit = std::uintmax_t(1024);

			inline constexpr std::uintmax_t Bytes = std::uintmax_t(1);
			inline constexpr std::uintmax_t KiB = Bytes * BinaryUnit;
			inline constexpr std::uintmax_t MiB = KiB * BinaryUnit;
			inline constexpr std::uintmax_t GiB = MiB * BinaryUnit;
			inline constexpr std::uintmax_t TiB = GiB * BinaryUnit;
			inline constexpr std::uintmax_t PiB = TiB * BinaryUnit;
			inline constexpr std::uintmax_t EiB = PiB * BinaryUnit;

			inline constexpr std::uintmax_t BinaryPromotionThreshold = std::uintmax_t(128);

			inline constexpr std::array<UnitDefinition, UnitsCount> Units{
					MakeUnit(Bytes,	BinaryPromotionThreshold,							"B")
				  , MakeUnit(KiB,			BinaryPromotionThreshold,					"KiB")
				  , MakeUnit(MiB,			BinaryPromotionThreshold,					"MiB")
				  , MakeUnit(GiB,			BinaryPromotionThreshold,					"GiB")
				  , MakeUnit(TiB,			BinaryPromotionThreshold,					"TiB")
				  , MakeUnit(PiB,			BinaryPromotionThreshold,					"PiB")
				  , UnitDefinition{EiB,		std::numeric_limits<std::uintmax_t>::max(),	"EiB"}

			};
		}

		namespace decimal_units
		{
			inline constexpr std::uintmax_t DecimalUnit = std::uintmax_t(1000);

			inline constexpr std::uintmax_t Bytes = std::uintmax_t(1);
			inline constexpr std::uintmax_t KB = Bytes * DecimalUnit;
			inline constexpr std::uintmax_t MB = KB * DecimalUnit;
			inline constexpr std::uintmax_t GB = MB * DecimalUnit;
			inline constexpr std::uintmax_t TB = GB * DecimalUnit;
			inline constexpr std::uintmax_t PB = TB * DecimalUnit;
			inline constexpr std::uintmax_t EB = PB * DecimalUnit;

			inline constexpr std::uintmax_t DecimalPromotionThreshold = std::uintmax_t(100);

			inline constexpr std::array<UnitDefinition, UnitsCount> Units{
					MakeUnit(Bytes,		DecimalPromotionThreshold,					"B")
				  , MakeUnit(KB,		DecimalPromotionThreshold,					"KB")
				  , MakeUnit(MB,		DecimalPromotionThreshold,					"MB")
				  , MakeUnit(GB,		DecimalPromotionThreshold,					"GB")
				  , MakeUnit(TB,		DecimalPromotionThreshold,					"TB")
				  , MakeUnit(PB,		DecimalPromotionThreshold,					"PB")
				  , UnitDefinition{EB,	std::numeric_limits<std::uintmax_t>::max(),	"EB"}

			};
		}
	}

	std::string FormatFileSize(std::uintmax_t fileSize, space_fossils::core::FileSizeUnitSystem unitFormatStyle, std::size_t decimalPlaces)
	{
		decimalPlaces = decimalPlaces > MaxDecimalPlaces ? MaxDecimalPlaces : decimalPlaces;

		std::size_t unitIndex = 0;

		const std::array<UnitDefinition, UnitsCount>& units = unitFormatStyle == FileSizeUnitSystem::Binary ? binary_units::Units : decimal_units::Units;

		while (unitIndex + 1 < UnitsCount) {
			const std::uintmax_t promotionPoint =
				decimalPlaces == 0
				? units.at(unitIndex + 1).divisor
				: units.at(unitIndex).promotionPoint;

			if (fileSize < promotionPoint) {
				break;
			}

			++unitIndex;
		}

		const UnitDefinition& unitDef = units.at(unitIndex);

		if (unitIndex == 0) {
			return std::to_string(fileSize) + " " + std::string(unitDef.suffix);
		}

		const std::uintmax_t wholeNumberPart = fileSize / unitDef.divisor;
		std::uintmax_t remainderNumberPart = fileSize % unitDef.divisor;

		std::string result = std::to_string(wholeNumberPart);

		if (decimalPlaces > 0) {
			result += ".";

			for (std::size_t i = 0; i < decimalPlaces; ++i) {
				remainderNumberPart *= std::uintmax_t(10);

				const std::uintmax_t digit = remainderNumberPart / unitDef.divisor;
				result += std::to_string(digit);

				remainderNumberPart %= unitDef.divisor;
			}
		}

		result += " ";
		result += unitDef.suffix;

		return result;
	}

}
