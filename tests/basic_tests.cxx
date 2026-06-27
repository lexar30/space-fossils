#include "space_fossils_tests/micro_test_framework.hxx"

namespace space_fossils::tests {

	SF_TEST(basic, BasicChecks)
	{
		const bool boolFalse = false;
		SF_ASSERT_EQ(boolFalse, false);
		SF_ASSERT_EQ(boolFalse == true, false);

		const int somePositiveIntMagicNum = 123;
		SF_ASSERT_EQ(somePositiveIntMagicNum > 0, true);
		SF_ASSERT_EQ(somePositiveIntMagicNum < 0, false);
		SF_ASSERT_EQ(somePositiveIntMagicNum == 0, false);
	}
}
