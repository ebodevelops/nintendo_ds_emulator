#include "cpu/arm7.h"

#include "common/bit.h"
#include "common/log.h"
#include "cpu/arm_core.h"
#include "cpu/arm7_decoder.h"
#include "memory/memory_map.h"

namespace nds::cpu {

// Thumb instruction set per ARM7TDMI Reference Manual §5.

void Arm7::thumb_shifted_register(u16 instr) {
    const u32 op  = (instr >> 11) & 0x3;
    const u32 amt = (instr >> 6)  & 0x1F;
    const u32 rs  = (instr >> 3)  & 0x7;
    const u32 rd  =  instr        & 0x7;
    bool c = state_.flag_c();
    u32 result;
    switch (op) {
        case 0: result = dec::lsl(state_.r[rs], amt, c, state_.flag_c()); break;
        case 1: {
            u32 a = amt == 0 ? 32u : amt;
            result = dec::lsr(state_.r[rs], a, c, state_.flag_c()); break;
        }
        case 2: {
            u32 a = amt == 0 ? 32u : amt;
            result = dec::asr(state_.r[rs], a, c, state_.flag_c()); break;
        }
        default: result = state_.r[rs]; break;
    }
    state_.r[rd] = result;
    state_.set_nz_from(result);
    state_.set_flag_c(c);
}

void Arm7::thumb_add_sub(u16 instr) {
    const bool imm = (instr & (1u << 10)) != 0;
    const bool sub = (instr & (1u << 9))  != 0;
    const u32 rn_imm = (instr >> 6) & 0x7;
    const u32 rs = (instr >> 3) & 0x7;
    const u32 rd =  instr & 0x7;
    const u32 a = state_.r[rs];
    const u32 b = imm ? rn_imm : state_.r[rn_imm];
    bool c, v;
    const u32 r = sub ? dec::sub_cv(a, b, c, v) : dec::add_cv(a, b, c, v);
    state_.r[rd] = r;
    state_.set_nz_from(r);
    state_.set_flag_c(c);
    state_.set_flag_v(v);
}

void Arm7::thumb_imm_alu(u16 instr) {
    const u32 op = (instr >> 11) & 0x3;
    const u32 rd = (instr >> 8) & 0x7;
    const u32 imm = instr & 0xFF;
    const u32 a = state_.r[rd];
    bool c, v;
    u32 r;
    switch (op) {
        case 0: r = imm;                           state_.r[rd] = r; state_.set_nz_from(r); return;          // MOV
        case 1: r = dec::sub_cv(a, imm, c, v);     state_.set_nz_from(r); state_.set_flag_c(c); state_.set_flag_v(v); return; // CMP
        case 2: r = dec::add_cv(a, imm, c, v);     state_.r[rd] = r; state_.set_nz_from(r); state_.set_flag_c(c); state_.set_flag_v(v); return; // ADD
        case 3: r = dec::sub_cv(a, imm, c, v);     state_.r[rd] = r; state_.set_nz_from(r); state_.set_flag_c(c); state_.set_flag_v(v); return; // SUB
    }
}

void Arm7::thumb_alu_op(u16 instr) {
    const u32 op = (instr >> 6) & 0xF;
    const u32 rs = (instr >> 3) & 0x7;
    const u32 rd = instr & 0x7;
    const u32 a = state_.r[rd];
    const u32 b = state_.r[rs];
    bool c = state_.flag_c();
    bool v = state_.flag_v();
    u32 r;
    bool writes = true;
    bool logical = false;

    switch (op) {
        case 0x0: r = a & b;                                   logical = true;  break; // AND
        case 0x1: r = a ^ b;                                   logical = true;  break; // EOR
        case 0x2: r = dec::lsl(a, b & 0xFFu, c, state_.flag_c()); logical = true; break; // LSL
        case 0x3: r = dec::lsr(a, b & 0xFFu, c, state_.flag_c()); logical = true; break; // LSR
        case 0x4: r = dec::asr(a, b & 0xFFu, c, state_.flag_c()); logical = true; break; // ASR
        case 0x5: r = dec::adc_cv(a, b, state_.flag_c(), c, v);                  break; // ADC
        case 0x6: r = dec::sbc_cv(a, b, state_.flag_c(), c, v);                  break; // SBC
        case 0x7: r = dec::ror(a, b & 0xFFu, c, state_.flag_c()); logical = true; break; // ROR
        case 0x8: r = a & b; writes = false;                   logical = true;  break; // TST
        case 0x9: r = dec::sub_cv(0, b, c, v);                                    break; // NEG
        case 0xA: r = dec::sub_cv(a, b, c, v); writes = false;                    break; // CMP
        case 0xB: r = dec::add_cv(a, b, c, v); writes = false;                    break; // CMN
        case 0xC: r = a | b;                                   logical = true;  break; // ORR
        case 0xD: r = a * b;                                   logical = true;  break; // MUL  (C UNPREDICTABLE -> leave)
        case 0xE: r = a & ~b;                                  logical = true;  break; // BIC
        default:  r = ~b;                                      logical = true;  break; // MVN
    }
    if (writes) state_.r[rd] = r;
    state_.set_nz_from(r);
    if (logical) {
        state_.set_flag_c(c);
    } else {
        state_.set_flag_c(c);
        state_.set_flag_v(v);
    }
}

void Arm7::thumb_hi_reg_bx(u16 instr) {
    const u32 op = (instr >> 8) & 0x3;
    const u32 h1 = (instr >> 7) & 0x1;
    const u32 h2 = (instr >> 6) & 0x1;
    const u32 rs = ((instr >> 3) & 0x7) | (h2 << 3);
    const u32 rd = ( instr       & 0x7) | (h1 << 3);
    // PC reads as current PC + 4 in Thumb (already baked into r[15]+2 vs +4
    // discipline) — we keep r[15] = current instr addr, so reads of r[15]
    // need +4.
    u32 a = rd == 15 ? state_.r[15] + 4 : state_.r[rd];
    u32 b = rs == 15 ? state_.r[15] + 4 : state_.r[rs];
    switch (op) {
        case 0: { // ADD (no flags)
            const u32 r = a + b;
            if (rd == 15) {
                state_.r[15] = (r & ~1u);
            } else {
                state_.r[rd] = r;
            }
            break;
        }
        case 1: { // CMP (flags)
            bool c, v;
            const u32 r = dec::sub_cv(a, b, c, v);
            state_.set_nz_from(r); state_.set_flag_c(c); state_.set_flag_v(v);
            break;
        }
        case 2: { // MOV (no flags)
            if (rd == 15) state_.r[15] = b & ~1u;
            else          state_.r[rd] = b;
            break;
        }
        case 3: { // BX
            if (b & 1u) { state_.cpsr |= kPsrT;  state_.r[15] = b & ~1u; }
            else        { state_.cpsr &= ~kPsrT; state_.r[15] = b & ~3u; }
            break;
        }
    }
}

void Arm7::thumb_pc_relative_load(u16 instr) {
    const u32 rd = (instr >> 8) & 0x7;
    const u32 imm = (instr & 0xFF) * 4;
    const u32 base = (state_.r[15] + 4) & ~3u;
    state_.r[rd] = read32(base + imm);
}

void Arm7::thumb_load_store_reg_off(u16 instr) {
    const u32 op = (instr >> 10) & 0x3;
    const u32 ro = (instr >> 6) & 0x7;
    const u32 rb = (instr >> 3) & 0x7;
    const u32 rd =  instr & 0x7;
    const u32 addr = state_.r[rb] + state_.r[ro];
    switch (op) {
        case 0: write32(addr & ~3u, state_.r[rd]); break;                            // STR
        case 1: write8 (addr, static_cast<u8>(state_.r[rd])); break;                  // STRB
        case 2: {                                                                    // LDR
            const u32 raw = read32(addr & ~3u);
            const u32 rot = (addr & 3u) * 8u;
            state_.r[rd] = rot ? ((raw >> rot) | (raw << (32 - rot))) : raw;
            break;
        }
        case 3: state_.r[rd] = read8(addr); break;                                   // LDRB
    }
}

void Arm7::thumb_load_store_signed(u16 instr) {
    const u32 op = (instr >> 10) & 0x3;
    const u32 ro = (instr >> 6) & 0x7;
    const u32 rb = (instr >> 3) & 0x7;
    const u32 rd =  instr & 0x7;
    const u32 addr = state_.r[rb] + state_.r[ro];
    switch (op) {
        case 0: write16(addr & ~1u, static_cast<u16>(state_.r[rd])); break;          // STRH
        case 1: state_.r[rd] = bit::sign_extend(read8(addr), 8); break;              // LDSB
        case 2: {                                                                    // LDRH
            const u32 raw = read16(addr & ~1u);
            const u32 rot = (addr & 1u) * 8u;
            state_.r[rd] = rot ? ((raw >> rot) | (raw << (32 - rot))) : raw;
            break;
        }
        case 3:                                                                      // LDSH
            if (addr & 1u) state_.r[rd] = bit::sign_extend(read8(addr), 8);
            else           state_.r[rd] = bit::sign_extend(read16(addr), 16);
            break;
    }
}

void Arm7::thumb_load_store_imm_off(u16 instr) {
    const bool byte = (instr & (1u << 12)) != 0;
    const bool load = (instr & (1u << 11)) != 0;
    const u32 imm = (instr >> 6) & 0x1F;
    const u32 rb  = (instr >> 3) & 0x7;
    const u32 rd  =  instr & 0x7;
    const u32 addr = state_.r[rb] + (byte ? imm : imm * 4);
    if (load) {
        if (byte) state_.r[rd] = read8(addr);
        else {
            const u32 raw = read32(addr & ~3u);
            const u32 rot = (addr & 3u) * 8u;
            state_.r[rd] = rot ? ((raw >> rot) | (raw << (32 - rot))) : raw;
        }
    } else {
        if (byte) write8(addr, static_cast<u8>(state_.r[rd]));
        else      write32(addr & ~3u, state_.r[rd]);
    }
}

void Arm7::thumb_load_store_halfword(u16 instr) {
    const bool load = (instr & (1u << 11)) != 0;
    const u32 imm = ((instr >> 6) & 0x1F) * 2;
    const u32 rb  = (instr >> 3) & 0x7;
    const u32 rd  =  instr & 0x7;
    const u32 addr = state_.r[rb] + imm;
    if (load) {
        const u32 raw = read16(addr & ~1u);
        const u32 rot = (addr & 1u) * 8u;
        state_.r[rd] = rot ? ((raw >> rot) | (raw << (32 - rot))) : raw;
    } else {
        write16(addr & ~1u, static_cast<u16>(state_.r[rd]));
    }
}

void Arm7::thumb_sp_relative(u16 instr) {
    const bool load = (instr & (1u << 11)) != 0;
    const u32 rd = (instr >> 8) & 0x7;
    const u32 imm = (instr & 0xFF) * 4;
    const u32 addr = state_.r[13] + imm;
    if (load) {
        const u32 raw = read32(addr & ~3u);
        const u32 rot = (addr & 3u) * 8u;
        state_.r[rd] = rot ? ((raw >> rot) | (raw << (32 - rot))) : raw;
    } else {
        write32(addr & ~3u, state_.r[rd]);
    }
}

void Arm7::thumb_load_address(u16 instr) {
    const bool use_sp = (instr & (1u << 11)) != 0;
    const u32 rd = (instr >> 8) & 0x7;
    const u32 imm = (instr & 0xFF) * 4;
    if (use_sp) state_.r[rd] = state_.r[13] + imm;
    else        state_.r[rd] = ((state_.r[15] + 4) & ~3u) + imm;
}

void Arm7::thumb_sp_add_offset(u16 instr) {
    const u32 imm = (instr & 0x7F) * 4;
    if (instr & (1u << 7)) state_.r[13] -= imm;
    else                   state_.r[13] += imm;
}

void Arm7::thumb_push_pop(u16 instr) {
    const bool pop = (instr & (1u << 11)) != 0;
    const bool r_bit = (instr & (1u << 8)) != 0;  // PC for pop, LR for push
    const u32 rlist = instr & 0xFF;

    u32 count = 0;
    for (u32 i = 0; i < 8; ++i) if (rlist & (1u << i)) ++count;
    if (r_bit) ++count;
    const u32 total = count * 4;

    if (pop) {
        u32 addr = state_.r[13];
        for (u32 i = 0; i < 8; ++i) {
            if (rlist & (1u << i)) {
                state_.r[i] = read32(addr & ~3u);
                addr += 4;
            }
        }
        if (r_bit) {
            const u32 pc = read32(addr & ~3u);
            addr += 4;
            // POP {PC} on ARMv4 just copies the value; bottom bit is ignored.
            state_.r[15] = pc & ~1u;
        }
        state_.r[13] += total;
    } else {
        u32 addr = state_.r[13] - total;
        state_.r[13] = addr;
        for (u32 i = 0; i < 8; ++i) {
            if (rlist & (1u << i)) {
                write32(addr & ~3u, state_.r[i]);
                addr += 4;
            }
        }
        if (r_bit) {
            write32(addr & ~3u, state_.r[14]);
        }
    }
}

void Arm7::thumb_multiple_load_store(u16 instr) {
    const bool load = (instr & (1u << 11)) != 0;
    const u32 rb = (instr >> 8) & 0x7;
    const u32 rlist = instr & 0xFF;

    u32 addr = state_.r[rb];
    u32 count = 0;
    for (u32 i = 0; i < 8; ++i) if (rlist & (1u << i)) ++count;

    if (rlist == 0) {
        // Edge case: empty list transfers R15 and advances by 0x40.
        if (load) state_.r[15] = read32(addr & ~3u) & ~1u;
        else      write32(addr & ~3u, state_.r[15] + 2);
        state_.r[rb] += 0x40;
        return;
    }

    if (load) {
        for (u32 i = 0; i < 8; ++i) {
            if (rlist & (1u << i)) {
                state_.r[i] = read32(addr & ~3u);
                addr += 4;
            }
        }
        const bool rb_in_list = (rlist & (1u << rb)) != 0;
        if (!rb_in_list) state_.r[rb] = addr;
    } else {
        bool first = true;
        for (u32 i = 0; i < 8; ++i) {
            if (rlist & (1u << i)) {
                u32 val = state_.r[i];
                if (i == rb && !first) val = state_.r[rb] + count * 4;
                write32(addr & ~3u, val);
                addr += 4;
                first = false;
            }
        }
        state_.r[rb] = addr;
    }
}

void Arm7::thumb_conditional_branch(u16 instr) {
    const u32 cond = (instr >> 8) & 0xF;
    if (!check_condition(cond, state_.cpsr)) return;
    const s32 offset = static_cast<s32>(bit::sign_extend(instr & 0xFFu, 8)) << 1;
    state_.r[15] = state_.r[15] + 2 + static_cast<u32>(offset);
}

void Arm7::thumb_swi(u16 /*instr*/) {
    raise_swi();
}

void Arm7::thumb_unconditional_branch(u16 instr) {
    const s32 offset = static_cast<s32>(bit::sign_extend(instr & 0x7FFu, 11)) << 1;
    state_.r[15] = state_.r[15] + 2 + static_cast<u32>(offset);
}

void Arm7::thumb_long_branch_link(u16 instr) {
    const bool high = (instr & (1u << 11)) != 0;
    const u32 offset = instr & 0x7FF;
    if (!high) {
        // First half: LR = PC + (signed offset << 12).
        const s32 ext = static_cast<s32>(bit::sign_extend(offset, 11)) << 12;
        state_.r[14] = state_.r[15] + 2 + static_cast<u32>(ext);
    } else {
        // Second half: PC = LR + (offset << 1); LR = old_PC | 1.
        const u32 next_link = (state_.r[15]) | 1u;  // r[15] already advanced past 2nd half
        const u32 target = state_.r[14] + (offset << 1);
        state_.r[14] = next_link;
        state_.r[15] = target & ~1u;
    }
}

}  // namespace nds::cpu
