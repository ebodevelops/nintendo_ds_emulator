#include "cpu/arm_core.h"

#include "common/log.h"

namespace nds::cpu {

bool check_condition(u32 cond, u32 cpsr) {
    const bool n = (cpsr & kPsrN) != 0;
    const bool z = (cpsr & kPsrZ) != 0;
    const bool c = (cpsr & kPsrC) != 0;
    const bool v = (cpsr & kPsrV) != 0;
    switch (cond & 0xF) {
        case 0x0: return z;                 // EQ
        case 0x1: return !z;                // NE
        case 0x2: return c;                 // CS / HS
        case 0x3: return !c;                // CC / LO
        case 0x4: return n;                 // MI
        case 0x5: return !n;                // PL
        case 0x6: return v;                 // VS
        case 0x7: return !v;                // VC
        case 0x8: return c && !z;           // HI
        case 0x9: return !c || z;           // LS
        case 0xA: return n == v;            // GE
        case 0xB: return n != v;            // LT
        case 0xC: return !z && (n == v);    // GT
        case 0xD: return z || (n != v);     // LE
        case 0xE: return true;              // AL
        case 0xF: return true;              // NV on ARMv4 is unpredictable; AL here
    }
    return false;
}

namespace {

// Save the registers belonging to `mode` from the live register file into
// the corresponding bank storage.
void save_bank(ArmState& s, Mode mode) {
    switch (mode) {
        case Mode::User:
        case Mode::System:
            for (int i = 0; i < 7; ++i) s.banked_usr[i] = s.r[8 + i];
            break;
        case Mode::Fiq:
            for (int i = 0; i < 7; ++i) s.banked_fiq[i] = s.r[8 + i];
            break;
        case Mode::Supervisor:
            for (int i = 0; i < 5; ++i) s.banked_usr[i] = s.r[8 + i];
            s.banked_svc[0] = s.r[13];
            s.banked_svc[1] = s.r[14];
            break;
        case Mode::Abort:
            for (int i = 0; i < 5; ++i) s.banked_usr[i] = s.r[8 + i];
            s.banked_abt[0] = s.r[13];
            s.banked_abt[1] = s.r[14];
            break;
        case Mode::Irq:
            for (int i = 0; i < 5; ++i) s.banked_usr[i] = s.r[8 + i];
            s.banked_irq[0] = s.r[13];
            s.banked_irq[1] = s.r[14];
            break;
        case Mode::Undefined:
            for (int i = 0; i < 5; ++i) s.banked_usr[i] = s.r[8 + i];
            s.banked_und[0] = s.r[13];
            s.banked_und[1] = s.r[14];
            break;
    }
}

// Load `mode`'s bank into the live register file.
void load_bank(ArmState& s, Mode mode) {
    switch (mode) {
        case Mode::User:
        case Mode::System:
            for (int i = 0; i < 7; ++i) s.r[8 + i] = s.banked_usr[i];
            break;
        case Mode::Fiq:
            for (int i = 0; i < 7; ++i) s.r[8 + i] = s.banked_fiq[i];
            break;
        case Mode::Supervisor:
            for (int i = 0; i < 5; ++i) s.r[8 + i] = s.banked_usr[i];
            s.r[13] = s.banked_svc[0];
            s.r[14] = s.banked_svc[1];
            break;
        case Mode::Abort:
            for (int i = 0; i < 5; ++i) s.r[8 + i] = s.banked_usr[i];
            s.r[13] = s.banked_abt[0];
            s.r[14] = s.banked_abt[1];
            break;
        case Mode::Irq:
            for (int i = 0; i < 5; ++i) s.r[8 + i] = s.banked_usr[i];
            s.r[13] = s.banked_irq[0];
            s.r[14] = s.banked_irq[1];
            break;
        case Mode::Undefined:
            for (int i = 0; i < 5; ++i) s.r[8 + i] = s.banked_usr[i];
            s.r[13] = s.banked_und[0];
            s.r[14] = s.banked_und[1];
            break;
    }
}

}  // namespace

void ArmState::switch_mode(Mode new_mode) {
    const Mode old = mode();
    if (old == new_mode) {
        cpsr = (cpsr & ~kPsrModeMask) | static_cast<u32>(new_mode);
        return;
    }
    save_bank(*this, old);
    load_bank(*this, new_mode);
    cpsr = (cpsr & ~kPsrModeMask) | static_cast<u32>(new_mode);
}

void ArmState::write_cpsr(u32 value, u32 write_mask) {
    const u32 new_value = (cpsr & ~write_mask) | (value & write_mask);
    const Mode new_mode = static_cast<Mode>(new_value & kPsrModeMask);
    if ((write_mask & kPsrModeMask) && new_mode != mode()) {
        switch_mode(new_mode);
        cpsr = (cpsr & kPsrModeMask) | (new_value & ~kPsrModeMask);
    } else {
        cpsr = new_value;
    }
}

u32 ArmState::read_spsr() const {
    switch (mode()) {
        case Mode::Fiq:        return spsr_fiq;
        case Mode::Supervisor: return spsr_svc;
        case Mode::Abort:      return spsr_abt;
        case Mode::Irq:        return spsr_irq;
        case Mode::Undefined:  return spsr_und;
        default:               return cpsr;  // no SPSR; behavior is unpredictable
    }
}

void ArmState::write_spsr(u32 value, u32 write_mask) {
    u32* target = nullptr;
    switch (mode()) {
        case Mode::Fiq:        target = &spsr_fiq; break;
        case Mode::Supervisor: target = &spsr_svc; break;
        case Mode::Abort:      target = &spsr_abt; break;
        case Mode::Irq:        target = &spsr_irq; break;
        case Mode::Undefined:  target = &spsr_und; break;
        default: return;
    }
    *target = (*target & ~write_mask) | (value & write_mask);
}

void ArmState::reset(u32 entry_point) {
    for (auto& v : r) v = 0;
    for (auto& v : banked_usr) v = 0;
    for (auto& v : banked_fiq) v = 0;
    for (auto& v : banked_svc) v = 0;
    for (auto& v : banked_abt) v = 0;
    for (auto& v : banked_irq) v = 0;
    for (auto& v : banked_und) v = 0;
    spsr_fiq = spsr_svc = spsr_abt = spsr_irq = spsr_und = 0;
    cpsr = static_cast<u32>(Mode::System) | kPsrI | kPsrF;
    r[15] = entry_point;
}

}  // namespace nds::cpu
