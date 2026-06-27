#include "space_fossils_tests/tests.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>
#include <string>
#include <string_view>
#include <limits>
#include <vector>

#include "space_fossils/size_formatter.hxx"

namespace space_fossils::tests {

	void BasicChecks()
	{
		const bool boolFalse = false;
		SF_ASSERT_EQ(boolFalse, false);
		SF_ASSERT_EQ(boolFalse == true, false);

		const int somePositiveIntMagicNum = 123;
		SF_ASSERT_EQ(somePositiveIntMagicNum > 0, true);
		SF_ASSERT_EQ(somePositiveIntMagicNum < 0, false);
		SF_ASSERT_EQ(somePositiveIntMagicNum == 0, false);
	}

	namespace SizeFormatterTests {
		void SizeFormatterDefaultArgumentTests()
		{
			SF_ASSERT_EQ(space_fossils::core::FormatFileSize(0), "0 B");
			SF_ASSERT_EQ(space_fossils::core::FormatFileSize(42), "42 B");
			SF_ASSERT_EQ(space_fossils::core::FormatFileSize(1024), "1.0 KiB");
		}

		void RunFormatterForData(
			const std::vector<std::pair<std::uint64_t, std::string>>& testCases
			, space_fossils::core::UnitFormatStyle unitFormatStyle
			, std::size_t decimalPlaces = space_fossils::core::DefaultDecimalPlaces
		)
		{
			for (const auto& [input, expectedOutput] : testCases) {

				std::string result = space_fossils::core::FormatFileSize(input, unitFormatStyle, decimalPlaces);
				SF_ASSERT_EQ(result, expectedOutput);
			}
		}

		void SizeFormatterBinaryUnitStyleTests()
		{
			std::vector<std::pair<std::uint64_t, std::string>> testCases{
				  {0													, "0 B"}
				, {1													, "1 B"}
				, {127													, "127 B"}
				, {128													, "0.1 KiB"}
				, {511													, "0.4 KiB"}
				, {512													, "0.5 KiB"}
				, {1023													, "0.9 KiB"}
				, {1024													, "1.0 KiB"}
				, {1535													, "1.4 KiB"}
				, {1536													, "1.5 KiB"}
				, {1537													, "1.5 KiB"}
				, {2047													, "1.9 KiB"}
				, {2048													, "2.0 KiB"}
				, {2049													, "2.0 KiB"}
				, {std::uint64_t(1024) * 128 - 1						, "127.9 KiB"}
				, {std::uint64_t(1024) * 128							, "0.1 MiB"}
				, {std::uint64_t(1024) * 1024 - 1						, "0.9 MiB"}
				, {std::uint64_t(1024) * 1024							, "1.0 MiB"}
				, {std::uint64_t(1024) * 1024 * 1024					, "1.0 GiB"}
				, {std::uint64_t(1024) * 1024 * 1024 * 512				, "0.5 TiB"}
				, {std::uint64_t(1024) * 1024 * 1024 * 1024				, "1.0 TiB"}
				, {std::uint64_t(1024) * 1024 * 1024 * 1024 * 1024		, "1.0 PiB"}
				, {std::numeric_limits<std::uint64_t>::max()			, "15.9 EiB"}
			};

			RunFormatterForData(testCases, space_fossils::core::UnitFormatStyle::Binary);

			std::vector<std::pair<std::uint64_t, std::string>> noDecimalPlacesTestCases{
				  {42													, "42 B"}
				, {128													, "0 KiB"}
				, {1024													, "1 KiB"}
				, {1536													, "1 KiB"}
				, {2048													, "2 KiB"}
				, {std::uint64_t(1024) * 128 - 1						, "127 KiB"}
				, {std::uint64_t(1024) * 128							, "0 MiB"}
				, {std::uint64_t(1024) * 1024							, "1 MiB"}
				, {std::numeric_limits<std::uint64_t>::max()			, "15 EiB"}
			};

			RunFormatterForData(noDecimalPlacesTestCases, space_fossils::core::UnitFormatStyle::Binary, 0);

			std::vector<std::pair<std::uint64_t, std::string>> twoDecimalPlacesTestCases{
				  {42													, "42 B"}
				, {128													, "0.12 KiB"}
				, {511													, "0.49 KiB"}
				, {512													, "0.50 KiB"}
				, {1023													, "0.99 KiB"}
				, {1024													, "1.00 KiB"}
				, {1536													, "1.50 KiB"}
				, {std::uint64_t(1024) * 128 - 1						, "127.99 KiB"}
				, {std::uint64_t(1024) * 128							, "0.12 MiB"}
				, {std::numeric_limits<std::uint64_t>::max()			, "15.99 EiB"}
			};

			RunFormatterForData(twoDecimalPlacesTestCases, space_fossils::core::UnitFormatStyle::Binary, 2);

			std::vector<std::pair<std::uint64_t, std::string>> threeDecimalPlacesTestCases{
				  {42													, "42 B"}
				, {128													, "0.125 KiB"}
				, {512													, "0.500 KiB"}
				, {1023													, "0.999 KiB"}
				, {1024													, "1.000 KiB"}
				, {1536													, "1.500 KiB"}
				, {std::uint64_t(1024) * 1024 * 1024 * 1024 * 1024 * 1024, "1.000 EiB"}
			};

			RunFormatterForData(threeDecimalPlacesTestCases, space_fossils::core::UnitFormatStyle::Binary, 3);
		}

		void SizeFormatterDecimalUnitStyleTests()
		{
			std::vector<std::pair<std::uint64_t, std::string>> testCases{
				  {0													, "0 B"}
				, {1													, "1 B"}
				, {99													, "99 B"}
				, {100													, "0.1 KB"}
				, {499													, "0.4 KB"}
				, {500													, "0.5 KB"}
				, {999													, "0.9 KB"}
				, {1000													, "1.0 KB"}
				, {1499													, "1.4 KB"}
				, {1500													, "1.5 KB"}
				, {1501													, "1.5 KB"}
				, {1999													, "1.9 KB"}
				, {2000													, "2.0 KB"}
				, {2001													, "2.0 KB"}
				, {std::uint64_t(1000) * 100 - 1						, "99.9 KB"}
				, {std::uint64_t(1000) * 100							, "0.1 MB"}
				, {std::uint64_t(1000) * 1000 - 1						, "0.9 MB"}
				, {std::uint64_t(1000) * 1000							, "1.0 MB"}
				, {std::uint64_t(1000) * 1000 * 1000					, "1.0 GB"}
				, {std::uint64_t(1000) * 1000 * 1000 * 500				, "0.5 TB"}
				, {std::uint64_t(1000) * 1000 * 1000 * 1000				, "1.0 TB"}
				, {std::uint64_t(1000) * 1000 * 1000 * 1000 * 1000		, "1.0 PB"}
				, {std::numeric_limits<std::uint64_t>::max()			, "18.4 EB"}
			};

			RunFormatterForData(testCases, space_fossils::core::UnitFormatStyle::Decimal);

			std::vector<std::pair<std::uint64_t, std::string>> noDecimalPlacesTestCases{
				  {42													, "42 B"}
				, {100													, "0 KB"}
				, {1000													, "1 KB"}
				, {1500													, "1 KB"}
				, {2000													, "2 KB"}
				, {std::uint64_t(1000) * 100 - 1						, "99 KB"}
				, {std::uint64_t(1000) * 100							, "0 MB"}
				, {std::uint64_t(1000) * 1000							, "1 MB"}
				, {std::numeric_limits<std::uint64_t>::max()			, "18 EB"}
			};

			RunFormatterForData(noDecimalPlacesTestCases, space_fossils::core::UnitFormatStyle::Decimal, 0);

			std::vector<std::pair<std::uint64_t, std::string>> twoDecimalPlacesTestCases{
				  {42													, "42 B"}
				, {100													, "0.10 KB"}
				, {499													, "0.49 KB"}
				, {500													, "0.50 KB"}
				, {999													, "0.99 KB"}
				, {1000													, "1.00 KB"}
				, {1500													, "1.50 KB"}
				, {std::uint64_t(1000) * 100 - 1						, "99.99 KB"}
				, {std::uint64_t(1000) * 100							, "0.10 MB"}
				, {std::numeric_limits<std::uint64_t>::max()			, "18.44 EB"}
			};

			RunFormatterForData(twoDecimalPlacesTestCases, space_fossils::core::UnitFormatStyle::Decimal, 2);

			std::vector<std::pair<std::uint64_t, std::string>> threeDecimalPlacesTestCases{
				  {42													, "42 B"}
				, {100													, "0.100 KB"}
				, {500													, "0.500 KB"}
				, {999													, "0.999 KB"}
				, {1000													, "1.000 KB"}
				, {1500													, "1.500 KB"}
				, {std::uint64_t(1000) * 1000 * 1000 * 1000 * 1000 * 1000, "1.000 EB"}
			};

			RunFormatterForData(threeDecimalPlacesTestCases, space_fossils::core::UnitFormatStyle::Decimal, 3);
		}
	}

	void RunAllTests()
	{
		SF_RUN_TEST(BasicChecks);

		SF_RUN_TEST(SizeFormatterTests::SizeFormatterDefaultArgumentTests);
		SF_RUN_TEST(SizeFormatterTests::SizeFormatterBinaryUnitStyleTests);
		SF_RUN_TEST(SizeFormatterTests::SizeFormatterDecimalUnitStyleTests);


	}
}
