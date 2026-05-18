#pragma once

#include "common/types.h"
#include "cpu/arm_core.h"

// Shared inline decode helpers used by the ARM7 ARM/Thumb interpreters.
// All routines operate on a passed-in ArmState reference; they have no side
// effects on memory.

namespace nds::cpu::dec {

// Logical shift left, returning value + carry-out. `amount` may be 0..255.
inline u32 lsl(u32 value, u32 amount, bool& carry_out, bool carry_in) {
    if (amount == 0)      { carry_out = carry_in; return value; }
    if (amount < 32)      { carry_out = ((value >> (32 - amount)) & 1u) != 0; return value << amount; }
    if (amount == 32)     { carry_out = (value & 1u) != 0; return 0; }
    carry_out = false; return 0;
}

inline u32 lsr(u32 value, u32 amount, bool& carry_out, bool carry_in) {
    if (amount == 0)      { carry_out = carry_in; return value; }
    if (amount < 32)      { carry_out = ((value >> (amount - 1)) & 1u) != 0; return value >> amount; }
    if (amount == 32)     { carry_out = (value & 0x80000000u) != 0; return 0; }
    carry_out = false; return 0;
}

inline u32 asr(u32 value, u32 amount, bool& carry_out, bool carry_in) {
    if (amount == 0)      { carry_out = carry_in; return value; }
    if (amount < 32) {
        carry_out = ((value >> (amount - 1)) & 1u) != 0;
        return static_cast<u32>(static_cast<s32>(value) >> amount);
    }
    carry_out = (value & 0x80000000u) != 0;
    return carry_out ? 0xFFFFFFFFu : 0u;
}

inline u32 ror(u32 value, u32 amount, bool& carry_out, bool carry_in) {
    if (amount == 0)      { carry_out = carry_in; return value; }
    amount &= 31u;
    if (amount == 0)      { carry_out = (value & 0x80000000u) != 0; return value; }
    const u32 result = (value >> amount) | (value << (32 - amount));
    carry_out = (result & 0x80000000u) != 0;
    return result;
}

// RRX (rotate right with extend) — used when ROR by 0 with an explicit
// immediate of 0 in the shifter operand.
inline u32 rrx(u32 value, bool& carry_out, bool carry_in) {
    carry_out = (value & 1u) != 0;
    return (value >> 1) | (carry_in ? 0x80000000u : 0u);
}

// Decode an ARM shifter operand by shift-type bits 6:5 with a given amount.
// `amount` should be the resolved shift amount (immediate or low byte of a
// register), and `imm_shift` indicates whether this is the immediate-shift
// form (which has special-cased zero-amount semantics for LSR/ASR/ROR).
inline u32 shift_by_type(u32 type, u32 value, u32 amount,
                         bool& carry_out, bool carry_in, bool imm_shift) {
    switch (type & 3) {
        case 0: return lsl(value, amount, carry_out, carry_in);
        case 1:
            if (imm_shift && amount == 0) amount = 32;
            return lsr(value, amount, carry_out, carry_in);
        case 2:
            if (imm_shift && amount == 0) amount = 32;
            return asr(value, amount, carry_out, carry_in);
        case 3:
            if (imm_shift && amount == 0) return rrx(value, carry_out, carry_in);
            return ror(value, amount, carry_out, carry_in);
    }
    carry_out = carry_in;
    return value;
}

// ALU add/sub primitives that report carry/overflow.
inline u32 add_cv(u32 a, u32 b, bool& c, bool& v) {
    const u64 r64 = static_cast<u64>(a) + static_cast<u64>(b);
    const u32 r = static_cast<u32>(r64);
    c = (r64 >> 32) != 0;
    v = (~(a ^ b) & (a ^ r) & 0x80000000u) != 0;
    return r;
}

inline u32 adc_cv(u32 a, u32 b, bool cin, bool& c, bool& v) {
    const u64 r64 = static_cast<u64>(a) + static_cast<u64>(b) + (cin ? 1u : 0u);
    const u32 r = static_cast<u32>(r64);
    c = (r64 >> 32) != 0;
    v = (~(a ^ b) & (a ^ r) & 0x80000000u) != 0;
    return r;
}

inline u32 sub_cv(u32 a, u32 b, bool& c, bool& v) {
    const u32 r = a - b;
    c = a >= b;  // NOT-borrow
    v = ((a ^ b) & (a ^ r) & 0x80000000u) != 0;
    return r;
}

inline u32 sbc_cv(u32 a, u32 b, bool cin, bool& c, bool& v) {
    // a - b - (1 - C) == a + ~b + C
    const u64 r64 = static_cast<u64>(a) + static_cast<u64>(~b) + (cin ? 1u : 0u);
    const u32 r = static_cast<u32>(r64);
    c = (r64 >> 32) != 0;
    v = ((a ^ b) & (a ^ r) & 0x80000000u) != 0;
    return r;
}

}  // namespace nds::cpu::dec
