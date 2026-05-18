#pragma once

#include "common/types.h"

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
constexpr u32 kPsrModeMask = 0x1Fu;
constexpr u32 kPsrFlagsMask   = 0xF0000000u;       // NZCV
constexpr u32 kPsrControlMask = 0x000000FFu;       // I, F, T, mode

// ARM condition field (bits 31:28). Returns true iff the instruction should
// execute given the current CPSR flags.
bool check_condition(u32 cond, u32 cpsr);

struct ArmState {
    // Live register file (banked into the appropriate storage on mode switch).
    u32 r[16] = {0};
    u32 cpsr  = static_cast<u32>(Mode::Supervisor) | kPsrI | kPsrF;

    // Banked copies. Slot semantics:
    //   banked_usr[i]  ->  r8+i  for User/System
    //   banked_fiq[i]  ->  r8+i  for FIQ
    //   banked_svc[0]/[1] -> r13/r14 for Supervisor
    //   banked_abt[0]/[1] -> r13/r14 for Abort
    //   banked_irq[0]/[1] -> r13/r14 for IRQ
    //   banked_und[0]/[1] -> r13/r14 for Undefined
    u32 banked_usr[7] = {0};
    u32 banked_fiq[7] = {0};
    u32 banked_svc[2] = {0};
    u32 banked_abt[2] = {0};
    u32 banked_irq[2] = {0};
    u32 banked_und[2] = {0};

    // SPSR per privileged mode (User/System have none).
    u32 spsr_fiq = 0;
    u32 spsr_svc = 0;
    u32 spsr_abt = 0;
    u32 spsr_irq = 0;
    u32 spsr_und = 0;

    Mode mode() const { return static_cast<Mode>(cpsr & kPsrModeMask); }
    bool thumb() const { return (cpsr & kPsrT) != 0; }
    bool irq_disabled() const { return (cpsr & kPsrI) != 0; }
    bool fiq_disabled() const { return (cpsr & kPsrF) != 0; }

    // Flag accessors.
    bool flag_n() const { return (cpsr & kPsrN) != 0; }
    bool flag_z() const { return (cpsr & kPsrZ) != 0; }
    bool flag_c() const { return (cpsr & kPsrC) != 0; }
    bool flag_v() const { return (cpsr & kPsrV) != 0; }
    void set_flag_n(bool b) { cpsr = b ? (cpsr | kPsrN) : (cpsr & ~kPsrN); }
    void set_flag_z(bool b) { cpsr = b ? (cpsr | kPsrZ) : (cpsr & ~kPsrZ); }
    void set_flag_c(bool b) { cpsr = b ? (cpsr | kPsrC) : (cpsr & ~kPsrC); }
    void set_flag_v(bool b) { cpsr = b ? (cpsr | kPsrV) : (cpsr & ~kPsrV); }
    void set_nz_from(u32 result) {
        set_flag_n((result & 0x80000000u) != 0);
        set_flag_z(result == 0);
    }

    // Initialize CPU to a clean post-reset state and seed PC.
    void reset(u32 entry_point);

    // Switch operating mode, swapping the affected register banks.
    void switch_mode(Mode new_mode);

    // CPSR write helpers; a write that changes the mode field is treated as a
    // mode switch so banks stay coherent.
    void write_cpsr(u32 value, u32 write_mask);

    // SPSR for the current mode. Reads/writes go nowhere in User/System.
    u32  read_spsr() const;
    void write_spsr(u32 value, u32 write_mask);
};

}  // namespace nds::cpu
