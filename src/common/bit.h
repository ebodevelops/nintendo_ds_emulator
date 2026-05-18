#pragma once

#include "common/types.h"

namespace nds::bit {

constexpr u32 rotr32(u32 v, u32 n) {
    n &= 31;
    return (v >> n) | (v << ((32 - n) & 31));
}

constexpr u32 sign_extend(u32 v, u32 bits) {
    const u32 mask = u32{1} << (bits - 1);
    return (v ^ mask) - mask;
}

template <u32 N>
constexpr bool test(u32 v) {
    static_assert(N < 32);
    return ((v >> N) & 1u) != 0;
}

constexpr u32 field(u32 v, u32 lo, u32 width) {
    return (v >> lo) & ((u32{1} << width) - 1);
}

}  // namespace nds::bit
