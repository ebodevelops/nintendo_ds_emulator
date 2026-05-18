#pragma once

#include "common/types.h"

#include <array>

namespace nds::cpu {

// ARM CPU operating mode (CPSR M[4:0]).
enum class Mode : u8 {
    User       = 0x10,
    Fiq        = 0x11,
    Irq        = 0x12,
    Supervisor = 0x13,
    Abort      = 0x17,
    Undefined  = 0x1B,
    System     = 0x1F,
};

// PSR bit positions.
constexpr u32 kPsrN = 1u << 31;
constexpr u32 kPsrZ = 1u << 30;
constexpr u32 kPsrC = 1u << 29;
constexpr u32 kPsrV = 1u << 28;
constexpr u32 kPsrI = 1u << 7;
constexpr u32 kPsrF = 1u << 6;
constexpr u32 kPsrT = 1u << 5;

// Shared register-file state. ARM7TDMI and ARM946E-S both have 16 visible
// registers + CPSR + banked SPSRs; ARMv5 adds CLZ/BLX/etc. as decode-side
// differences. The actual interpreters live in arm7.cpp / arm9.cpp.
struct ArmState {
    std::array<u32, 16> r{};   // r0..r15 (r15 = PC)
    u32 cpsr = static_cast<u32>(Mode::Supervisor);

    // Banked SPSRs, indexed by mode-table slot (Fiq, Svc, Abt, Irq, Und).
    std::array<u32, 5> spsr{};

    // Banked registers for FIQ (r8..r14) and for the other privileged modes
    // (sp/lr per mode). Wiring deferred until M1 when the interpreter lands.
    std::array<u32, 7> fiq_r{};       // r8_fiq..r14_fiq
    std::array<u32, 7> usr_r{};       // r8_usr..r14_usr (saved during FIQ)
    std::array<u32, 2> svc_sp_lr{};
    std::array<u32, 2> abt_sp_lr{};
    std::array<u32, 2> irq_sp_lr{};
    std::array<u32, 2> und_sp_lr{};

    Mode mode() const { return static_cast<Mode>(cpsr & 0x1Fu); }
    bool thumb() const { return (cpsr & kPsrT) != 0; }

    void reset(u32 entry_point) {
        r.fill(0);
        r[15] = entry_point;
        cpsr  = static_cast<u32>(Mode::System) | kPsrI | kPsrF;
    }
};

}  // namespace nds::cpu
