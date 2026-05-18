#include "cpu/arm7.h"

#include "common/bit.h"
#include "common/log.h"
#include "cpu/arm_core.h"
#include "cpu/arm7_decoder.h"
#include "memory/memory_map.h"

namespace nds::cpu {

namespace {

// Effective operand-2 for a register-form data-processing instruction.
// Returns the operand value and reports whether the shifter produced a carry
// (needed by logical opcodes with the S bit set).
u32 reg_shift_operand(ArmState& s, u32 instr, bool& shifter_carry) {
    const u32 type = (instr >> 5) & 3;
    const u32 rm   = instr & 0xF;
    u32 rm_val = s.r[rm];

    // The "PC + 12" quirk applies when the shift amount comes from a register.
    const bool by_reg = (instr & (1u << 4)) != 0;
    if (rm == 15) rm_val += by_reg ? 4u : 0u;
    // Plus the standard +8 we already have baked into r[15] via fetch.

    if (by_reg) {
        const u32 rs  = (instr >> 8) & 0xF;
        u32 amt = s.r[rs] & 0xFFu;
        return dec::shift_by_type(type, rm_val, amt, shifter_carry, s.flag_c(), /*imm_shift=*/false);
    } else {
        u32 amt = (instr >> 7) & 0x1F;
        return dec::shift_by_type(type, rm_val, amt, shifter_carry, s.flag_c(), /*imm_shift=*/true);
    }
}

// Effective operand-2 for an immediate data-processing instruction.
u32 imm_shift_operand(ArmState& s, u32 instr, bool& shifter_carry) {
    const u32 imm = instr & 0xFF;
    const u32 rot = ((instr >> 8) & 0xF) * 2;
    if (rot == 0) {
        shifter_carry = s.flag_c();
        return imm;
    }
    const u32 v = (imm >> rot) | (imm << (32 - rot));
    shifter_carry = (v & 0x80000000u) != 0;
    return v;
}

}  // namespace

void Arm7::arm_data_processing(u32 instr) {
    const u32 opcode = (instr >> 21) & 0xF;
    const bool set_flags = (instr & (1u << 20)) != 0;
    const u32 rn = (instr >> 16) & 0xF;
    const u32 rd = (instr >> 12) & 0xF;
    const bool imm_form = (instr & (1u << 25)) != 0;

    bool shifter_c = state_.flag_c();
    u32  op2;
    if (imm_form) {
        op2 = imm_shift_operand(state_, instr, shifter_c);
    } else {
        op2 = reg_shift_operand(state_, instr, shifter_c);
    }

    // Read Rn after operand-2 (in case Rs read above changed nothing — just
    // be consistent about the +8 PC offset). When the shift is by register,
    // Rn-as-PC reads as PC+12; otherwise PC+8.
    u32 rn_val = state_.r[rn];
    if (rn == 15) {
        const bool reg_shift = !imm_form && (instr & (1u << 4));
        rn_val += reg_shift ? 4u : 0u;
    }

    bool c = state_.flag_c();
    bool v = state_.flag_v();
    u32 result = 0;
    bool logical = false;
    bool writes_rd = true;

    switch (opcode) {
        case 0x0: result = rn_val & op2;                       logical = true;  break; // AND
        case 0x1: result = rn_val ^ op2;                       logical = true;  break; // EOR
        case 0x2: result = dec::sub_cv(rn_val, op2, c, v);                       break; // SUB
        case 0x3: result = dec::sub_cv(op2, rn_val, c, v);                       break; // RSB
        case 0x4: result = dec::add_cv(rn_val, op2, c, v);                       break; // ADD
        case 0x5: result = dec::adc_cv(rn_val, op2, state_.flag_c(), c, v);      break; // ADC
        case 0x6: result = dec::sbc_cv(rn_val, op2, state_.flag_c(), c, v);      break; // SBC
        case 0x7: result = dec::sbc_cv(op2, rn_val, state_.flag_c(), c, v);      break; // RSC
        case 0x8: result = rn_val & op2; writes_rd = false;    logical = true;  break; // TST
        case 0x9: result = rn_val ^ op2; writes_rd = false;    logical = true;  break; // TEQ
        case 0xA: result = dec::sub_cv(rn_val, op2, c, v); writes_rd = false;    break; // CMP
        case 0xB: result = dec::add_cv(rn_val, op2, c, v); writes_rd = false;    break; // CMN
        case 0xC: result = rn_val | op2;                       logical = true;  break; // ORR
        case 0xD: result = op2;                                 logical = true;  break; // MOV
        case 0xE: result = rn_val & ~op2;                       logical = true;  break; // BIC
        case 0xF: result = ~op2;                                logical = true;  break; // MVN
    }

    if (set_flags) {
        if (rd == 15 && writes_rd) {
            // S=1 with Rd=PC: copy SPSR to CPSR (mode-restoring return).
            state_.write_cpsr(state_.read_spsr(), 0xFFFFFFFFu);
        } else {
            state_.set_nz_from(result);
            if (logical) {
                state_.set_flag_c(shifter_c);
                // V unchanged for logical ops.
            } else {
                state_.set_flag_c(c);
                state_.set_flag_v(v);
            }
        }
    }

    if (writes_rd) state_.r[rd] = result;
}

void Arm7::arm_psr_transfer(u32 instr) {
    const bool to_psr = (instr & (1u << 21)) != 0;
    const bool use_spsr = (instr & (1u << 22)) != 0;
    if (!to_psr) {
        // MRS Rd, CPSR/SPSR
        const u32 rd = (instr >> 12) & 0xF;
        state_.r[rd] = use_spsr ? state_.read_spsr() : state_.cpsr;
        return;
    }
    // MSR CPSR/SPSR_<flags>, op2
    const bool imm_form = (instr & (1u << 25)) != 0;
    u32 value;
    if (imm_form) {
        const u32 imm = instr & 0xFF;
        const u32 rot = ((instr >> 8) & 0xF) * 2;
        value = rot ? ((imm >> rot) | (imm << (32 - rot))) : imm;
    } else {
        value = state_.r[instr & 0xF];
    }
    u32 mask = 0;
    if (instr & (1u << 19)) mask |= 0xFF000000u;  // flags
    if (instr & (1u << 18)) mask |= 0x00FF0000u;  // status (unused on ARMv4)
    if (instr & (1u << 17)) mask |= 0x0000FF00u;  // extension (unused)
    if (instr & (1u << 16)) mask |= 0x000000FFu;  // control
    // In User mode, only the flags byte is writable; mask out the rest.
    if (state_.mode() == Mode::User) mask &= 0xF0000000u;

    if (use_spsr) state_.write_spsr(value, mask);
    else          state_.write_cpsr(value, mask);
}

void Arm7::arm_multiply(u32 instr) {
    const u32 rd = (instr >> 16) & 0xF;
    const u32 rn = (instr >> 12) & 0xF;
    const u32 rs = (instr >> 8)  & 0xF;
    const u32 rm = instr & 0xF;
    const bool accumulate = (instr & (1u << 21)) != 0;
    const bool set_flags  = (instr & (1u << 20)) != 0;

    u32 result = state_.r[rm] * state_.r[rs];
    if (accumulate) result += state_.r[rn];
    state_.r[rd] = result;
    if (set_flags) state_.set_nz_from(result);
}

void Arm7::arm_multiply_long(u32 instr) {
    const u32 rdhi = (instr >> 16) & 0xF;
    const u32 rdlo = (instr >> 12) & 0xF;
    const u32 rs   = (instr >> 8)  & 0xF;
    const u32 rm   = instr & 0xF;
    const bool sign       = (instr & (1u << 22)) != 0;
    const bool accumulate = (instr & (1u << 21)) != 0;
    const bool set_flags  = (instr & (1u << 20)) != 0;

    u64 result;
    if (sign) {
        const s64 m = static_cast<s32>(state_.r[rm]);
        const s64 s = static_cast<s32>(state_.r[rs]);
        s64 r = m * s;
        if (accumulate) {
            const s64 acc = (static_cast<s64>(state_.r[rdhi]) << 32) |
                            static_cast<u64>(state_.r[rdlo]);
            r += acc;
        }
        result = static_cast<u64>(r);
    } else {
        const u64 m = state_.r[rm];
        const u64 s = state_.r[rs];
        result = m * s;
        if (accumulate) {
            const u64 acc = (static_cast<u64>(state_.r[rdhi]) << 32) |
                            static_cast<u64>(state_.r[rdlo]);
            result += acc;
        }
    }
    state_.r[rdlo] = static_cast<u32>(result);
    state_.r[rdhi] = static_cast<u32>(result >> 32);
    if (set_flags) {
        state_.set_flag_n((state_.r[rdhi] & 0x80000000u) != 0);
        state_.set_flag_z(result == 0);
    }
}

void Arm7::arm_single_swap(u32 instr) {
    const u32 rn = (instr >> 16) & 0xF;
    const u32 rd = (instr >> 12) & 0xF;
    const u32 rm = instr & 0xF;
    const bool byte = (instr & (1u << 22)) != 0;
    const u32 addr = state_.r[rn];
    if (byte) {
        const u8 tmp = static_cast<u8>(read8(addr));
        write8(addr, static_cast<u8>(state_.r[rm]));
        state_.r[rd] = tmp;
    } else {
        // Rotated read for unaligned addresses, like LDR.
        const u32 raw = read32(addr & ~3u);
        const u32 rot = (addr & 3u) * 8u;
        const u32 tmp = rot ? ((raw >> rot) | (raw << (32 - rot))) : raw;
        write32(addr & ~3u, state_.r[rm]);
        state_.r[rd] = tmp;
    }
}

void Arm7::arm_branch_exchange(u32 instr) {
    const u32 rm = instr & 0xF;
    const u32 target = state_.r[rm];
    if (target & 1u) {
        state_.cpsr |= kPsrT;
        state_.r[15] = target & ~1u;
    } else {
        state_.cpsr &= ~kPsrT;
        state_.r[15] = target & ~3u;
    }
}

void Arm7::arm_halfword_signed(u32 instr) {
    const bool pre   = (instr & (1u << 24)) != 0;
    const bool up    = (instr & (1u << 23)) != 0;
    const bool imm   = (instr & (1u << 22)) != 0;
    const bool wb    = (instr & (1u << 21)) != 0;
    const bool load  = (instr & (1u << 20)) != 0;
    const u32 rn = (instr >> 16) & 0xF;
    const u32 rd = (instr >> 12) & 0xF;
    const u32 sh = (instr >> 5) & 0x3;

    u32 offset;
    if (imm) {
        offset = ((instr >> 4) & 0xF0) | (instr & 0xF);
    } else {
        offset = state_.r[instr & 0xF];
    }

    u32 base = state_.r[rn];
    u32 addr = pre ? (up ? base + offset : base - offset) : base;
    u32 wb_addr = pre ? addr : (up ? base + offset : base - offset);

    if (load) {
        switch (sh) {
            case 1: { // LDRH
                const u32 raw = read16(addr & ~1u);
                const u32 rot = (addr & 1u) * 8u;
                state_.r[rd] = rot ? ((raw >> rot) | (raw << (32 - rot))) : raw;
                break;
            }
            case 2: { // LDRSB
                state_.r[rd] = bit::sign_extend(read8(addr), 8);
                break;
            }
            case 3: { // LDRSH
                if (addr & 1u) {
                    // Misaligned: behaves like LDRSB on the low byte.
                    state_.r[rd] = bit::sign_extend(read8(addr), 8);
                } else {
                    state_.r[rd] = bit::sign_extend(read16(addr), 16);
                }
                break;
            }
            default: break;
        }
    } else {
        if (sh == 1) write16(addr & ~1u, static_cast<u16>(state_.r[rd]));
    }

    if (!pre || wb) {
        if (!load || rd != rn) state_.r[rn] = wb_addr;
    }
}

void Arm7::arm_single_transfer(u32 instr) {
    const bool imm_off = (instr & (1u << 25)) == 0;  // I bit inverted: 0=imm, 1=reg
    const bool pre   = (instr & (1u << 24)) != 0;
    const bool up    = (instr & (1u << 23)) != 0;
    const bool byte  = (instr & (1u << 22)) != 0;
    const bool wb    = (instr & (1u << 21)) != 0;
    const bool load  = (instr & (1u << 20)) != 0;
    const u32 rn = (instr >> 16) & 0xF;
    const u32 rd = (instr >> 12) & 0xF;

    u32 offset;
    if (imm_off) {
        offset = instr & 0xFFF;
    } else {
        const u32 type = (instr >> 5) & 3;
        const u32 amt  = (instr >> 7) & 0x1F;
        bool dummy_c;
        offset = dec::shift_by_type(type, state_.r[instr & 0xF], amt,
                                    dummy_c, state_.flag_c(), /*imm_shift=*/true);
    }

    u32 base = state_.r[rn];
    u32 addr = pre ? (up ? base + offset : base - offset) : base;
    u32 wb_addr = pre ? addr : (up ? base + offset : base - offset);

    if (load) {
        if (byte) {
            state_.r[rd] = read8(addr);
        } else {
            const u32 raw = read32(addr & ~3u);
            const u32 rot = (addr & 3u) * 8u;
            state_.r[rd] = rot ? ((raw >> rot) | (raw << (32 - rot))) : raw;
        }
    } else {
        // Store of PC stores PC+12.
        u32 val = state_.r[rd];
        if (rd == 15) val += 4;
        if (byte) write8(addr, static_cast<u8>(val));
        else      write32(addr & ~3u, val);
    }

    if (!pre || wb) {
        if (!load || rd != rn) state_.r[rn] = wb_addr;
    }
}

void Arm7::arm_block_transfer(u32 instr) {
    const bool pre   = (instr & (1u << 24)) != 0;
    const bool up    = (instr & (1u << 23)) != 0;
    const bool psr_u = (instr & (1u << 22)) != 0;  // S bit
    const bool wb    = (instr & (1u << 21)) != 0;
    const bool load  = (instr & (1u << 20)) != 0;
    const u32 rn     = (instr >> 16) & 0xF;
    const u16 rlist  = static_cast<u16>(instr & 0xFFFF);

    // Empty list edge case: ARM7TDMI transfers nothing, but still writes back
    // an offset of ±0x40.
    u32 count = 0;
    for (u32 i = 0; i < 16; ++i) if (rlist & (1u << i)) ++count;
    const u32 total_bytes = count * 4;

    u32 base = state_.r[rn];
    u32 start, end, wb_addr;
    if (up) {
        start   = pre ? base + 4 : base;
        end     = pre ? base + total_bytes : base + total_bytes - 4;
        wb_addr = base + (total_bytes ? total_bytes : 0x40);
    } else {
        start   = pre ? base - total_bytes : base - total_bytes + 4;
        end     = pre ? base - 4 : base;
        wb_addr = base - (total_bytes ? total_bytes : 0x40);
    }
    (void)end;

    // If we're transferring user-mode regs from a privileged mode (S bit and
    // either store or load-without-PC), temporarily access the user bank.
    const bool user_bank = psr_u && (!load || (rlist & (1u << 15)) == 0);
    const Mode saved_mode = state_.mode();
    if (user_bank && saved_mode != Mode::User && saved_mode != Mode::System) {
        state_.switch_mode(Mode::User);
    }

    u32 addr = start;
    if (count == 0) {
        // Empty list still transfers R15 with the special offset (treat as
        // single R15 access at base ± 0).
    }

    if (load) {
        for (u32 i = 0; i < 16; ++i) {
            if (!(rlist & (1u << i))) continue;
            state_.r[i] = read32(addr & ~3u);
            addr += 4;
        }
        if (psr_u && (rlist & (1u << 15))) {
            // LDM with PSR transfer: CPSR <- SPSR after loading PC.
            state_.write_cpsr(state_.read_spsr(), 0xFFFFFFFFu);
        }
    } else {
        bool first = true;
        for (u32 i = 0; i < 16; ++i) {
            if (!(rlist & (1u << i))) continue;
            u32 val = state_.r[i];
            // Storing PC stores PC+12.
            if (i == 15) val += 4;
            // Writeback-before-store edge case: when Rn is the first stored
            // register, the original Rn is stored; otherwise the post-WB Rn.
            if (i == rn && !first && wb) val = wb_addr;
            write32(addr & ~3u, val);
            addr += 4;
            first = false;
        }
    }

    if (user_bank && saved_mode != Mode::User && saved_mode != Mode::System) {
        state_.switch_mode(saved_mode);
    }

    if (wb) {
        // If LDM and Rn is in the list, writeback is suppressed.
        const bool rn_in_list = (rlist & (1u << rn)) != 0;
        if (!load || !rn_in_list) state_.r[rn] = wb_addr;
    }
}

void Arm7::arm_branch(u32 instr) {
    const bool link = (instr & (1u << 24)) != 0;
    s32 offset = static_cast<s32>(bit::sign_extend(instr & 0x00FFFFFF, 24)) << 2;
    if (link) state_.r[14] = state_.r[15];  // r[15] already advanced to next instr
    state_.r[15] = state_.r[15] + 4 + static_cast<u32>(offset);
}

void Arm7::arm_swi(u32 /*instr*/) {
    raise_swi();
}

void Arm7::arm_undefined(u32 instr) {
    NDS_LOG_WARN("arm7", "Undefined ARM instruction 0x%08X at 0x%08X",
                 instr, state_.r[15] - 4);
    raise_undefined();
}

}  // namespace nds::cpu
