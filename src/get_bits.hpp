#pragma once

#include <cstddef>

// Get the n bits of value after the first offset bits
template <typename Type>
[[nodiscard]] static constexpr inline Type get_bits(Type value, size_t offset, size_t n)
{
    return (value >> offset) & ((1 << n) - 1);
}