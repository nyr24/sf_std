#pragma once

#include "defines.hpp"
#include <concepts>
#include <string_view>
#include <type_traits>

namespace sf {

[[noreturn]] void panic(const char* message);
[[noreturn]] void unreachable();
constexpr u32 get_mem_page_size() { return 4096; }

template<typename T>
constexpr bool is_power_of_two(T x)  noexcept {
    return (x & (x-1)) == 0;
}

template<typename T>
constexpr T next_power_of_2(T x)
{
	T y = 1;
	while (y < x) y += y;
	return y;
}

template<typename T>
consteval bool smaller_than_two_words() noexcept {
    return sizeof(T) <= sizeof(void*) * 2;
}

template<typename T>
consteval bool should_pass_by_value() noexcept {
    return smaller_than_two_words<T>() && !std::same_as<T, std::string_view>;
}

template<typename T>
struct RRefOrVal {
    using Type = std::conditional_t<should_pass_by_value<T>(), T, T&&>;
};

template<typename T>
struct ConstLRefOrVal {
    using Type = std::conditional_t<should_pass_by_value<T>(), T, const T&>;
};

template<typename T>
using ConstLRefOrValType = typename ConstLRefOrVal<T>::Type;

template<typename T>
using RRefOrValType = typename RRefOrVal<T>::Type;

template<typename First, typename Second>
concept SameTypes = std::same_as<std::remove_cv_t<std::remove_reference_t<First>>, std::remove_cv_t<std::remove_reference_t<Second>>>;

template<typename T>
constexpr T sf_clamp(ConstLRefOrValType<T> val, ConstLRefOrValType<T> min, ConstLRefOrValType<T> max) noexcept {
    if (val < min) {
        return min;
    }
    if (val > max) {
        return max;
    }
    return val;
}

} // sf
