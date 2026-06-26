#pragma once

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

#define SF_RUN_TEST(name) do { \
        std::cout << "[ = START  " << #name << " = ] \n"; \
        std::cout << "[   RUN    " << #name << "   ] \n"; \
        name(); \
        std::cout << "[      OK  " << #name << "   ] \n"; \
        std::cout << "[ = END    " << #name << " = ] \n"; \
        std::cout << "------------------------------------" << "\n\n"; \
    } while(0)
