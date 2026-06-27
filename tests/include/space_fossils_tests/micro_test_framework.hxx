#pragma once

#include "space_fossils_tests/tests.hxx"

#include <type_traits>
#include <iostream>
#include <cstdlib>

namespace space_fossils::tests {

    template<typename T>
    auto ToPrintableValue(const T& value)
    {
        if constexpr (std::is_enum_v<T>) {
            return static_cast<std::underlying_type_t<T>>(value);
        }
        else {
            return value;
        }
    }
}

#define SF_ASSERT_EQ(a,b) do { \
        auto assert_eq_lhs = (a); \
        auto assert_eq_rhs = (b); \
        if (assert_eq_lhs != assert_eq_rhs) { \
            std::cerr << "ASSERT_EQ failed at " << __FILE__ << ":" << __LINE__ << "\n" \
                      << " expected: " << #a << " == " << #b << "\n" \
                      << " actual:   " << ::space_fossils::tests::ToPrintableValue(assert_eq_lhs) << " != " \
                      << ::space_fossils::tests::ToPrintableValue(assert_eq_rhs) << "\n"; \
            std::exit(1); \
        } \
    } while(0)

#define SF_CONCAT_IMPL(a,b) a##b
#define SF_CONCAT(a,b) SF_CONCAT_IMPL(a,b)
#define SF_TEST_FUNCTION_NAME(suite, name) SF_CONCAT(SF_CONCAT(suite, _), name)
#define SF_TEST_REGISTRAR_NAME(suite, name) SF_CONCAT(SF_TEST_FUNCTION_NAME(suite, name), _registrar)
#define SF_TEST_REGISTRAR_INSTANCE_NAME(suite, name) SF_CONCAT(SF_TEST_FUNCTION_NAME(suite, name), _registrar_instance)

#define SF_TEST(suite, name) \
    static void SF_TEST_FUNCTION_NAME(suite, name)(); \
    namespace { \
        struct SF_TEST_REGISTRAR_NAME(suite, name) { \
            SF_TEST_REGISTRAR_NAME(suite, name)() \
            { \
                ::space_fossils::tests::RegisterTest(#suite, #name, &SF_TEST_FUNCTION_NAME(suite, name)); \
            } \
        }; \
        [[maybe_unused]] const SF_TEST_REGISTRAR_NAME(suite, name) SF_TEST_REGISTRAR_INSTANCE_NAME(suite, name); \
    } \
    static void SF_TEST_FUNCTION_NAME(suite, name)()
