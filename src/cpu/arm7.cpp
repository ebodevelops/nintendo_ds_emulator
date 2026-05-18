#include "cpu/arm7.h"

#include "common/log.h"
#include "cpu/arm_core.h"
#include "memory/memory_map.h"

namespace nds::cpu {

namespace {
constexpr u32 kExcVecReset      = 0x00000000u;
constexpr u32 kExcVecUndefined  = 0x00000004u;
constexpr u32 kExcVecSWI        = 0x00000008u;
constexpr u32 kExcVecPrefetch   = 0x0000000Cu;
constexpr u32 kExcVecData       = 0x00000010u;
constexpr u32 kExcVecIRQ        = 0x00000018u;
constexpr u32 kExcVecFIQ        = 0x0000001Cu;
}  // namespace

void Arm7::reset_to_entry(u32 entry) {
    state_.reset(entry);
    NDS_LOG_INFO("arm7", "Reset; PC = 0x%08X", entry);
}

u32 Arm7::read_pc() const {
    return state_.r[15] + (state_.thumb() ? 4u : 8u);
}

u32  Arm7::read32 (u32 addr) { return mem_.read32(memory::MemoryMap::Cpu::Arm7, addr); }
u32  Arm7::read16 (u32 addr) { return mem_.read16(memory::MemoryMap::Cpu::Arm7, addr); }
u32  Arm7::read8  (u32 addr) { return mem_.read8 (memory::MemoryMap::Cpu::Arm7, addr); }
void Arm7::write32(u32 addr, u32 v) { mem_.write32(memory::MemoryMap::Cpu::Arm7, addr, v); }
void Arm7::write16(u32 addr, u16 v) { mem_.write16(memory::MemoryMap::Cpu::Arm7, addr, v); }
void Arm7::write8 (u32 addr, u8  v) { mem_.write8 (memory::MemoryMap::Cpu::Arm7, addr, v); }

u32 Arm7::step() {
    if (state_.thumb()) {
        step_thumb();
    } else {
        step_arm();
    }
    return 1;
}

void Arm7::step_arm() {
    const u32 pc    = state_.r[15];
    const u32 instr = read32(pc);
    state_.r[15] = pc + 4;

    const u32 cond = (instr >> 28) & 0xF;
    if (!check_condition(cond, state_.cpsr)) return;

    // Top-level decode on bits [27:25] and a handful of distinguishing bits.
    const u32 op = (instr >> 25) & 0x7;
    switch (op) {
        case 0b000: {
            // Multiply, multiply-long, swap, BX, halfword/signed transfer,
            // MRS/MSR (register), or data-processing-register.
            if ((instr & 0x0FFFFFF0u) == 0x012FFF10u) { arm_branch_exchange(instr); break; }
            if ((instr & 0x0FC000F0u) == 0x00000090u) { arm_multiply(instr);       break; }
            if ((instr & 0x0F8000F0u) == 0x00800090u) { arm_multiply_long(instr);  break; }
            if ((instr & 0x0FB00FF0u) == 0x01000090u) { arm_single_swap(instr);    break; }
            // Halfword/signed transfers have bit 7 set, bit 4 set, bits 6:5 != 00.
            if ((instr & 0x0E400F90u) == 0x00000090u || (instr & 0x0E400090u) == 0x00400090u) {
                arm_halfword_signed(instr);
                break;
            }
            // PSR transfers.
            //   MRS Rd, CPSR/SPSR:  cond 0001 0R00 1111 Rd 0000 0000 0000
            //   MSR CPSR/SPSR, Rm:  cond 0001 0R10 mask 1111 0000 0000 Rm
            if ((instr & 0x0FBF0FFFu) == 0x010F0000u) { arm_psr_transfer(instr); break; }
            if ((instr & 0x0FB0FFF0u) == 0x0120F000u) { arm_psr_transfer(instr); break; }
            arm_data_processing(instr);
            break;
        }
        case 0b001: {
            // Data-processing-immediate or MSR-immediate.
            //   MSR CPSR/SPSR, #imm:  cond 0011 0R10 mask 1111 imm12
            if ((instr & 0x0FB0F000u) == 0x0320F000u) { arm_psr_transfer(instr); break; }
            arm_data_processing(instr);
            break;
        }
        case 0b010: arm_single_transfer(instr); break;
        case 0b011:
            if (instr & (1u << 4)) { arm_undefined(instr); break; }
            arm_single_transfer(instr);
            break;
        case 0b100: arm_block_transfer(instr); break;
        case 0b101: arm_branch(instr); break;
        case 0b110: arm_undefined(instr); break;  // coprocessor LDC/STC — none on ARM7
        case 0b111:
            if (instr & (1u << 24)) { arm_swi(instr); break; }
            arm_undefined(instr);
            break;
    }
}

void Arm7::step_thumb() {
    const u32 pc    = state_.r[15];
    const u16 instr = static_cast<u16>(read16(pc));
    state_.r[15] = pc + 2;

    // Decode based on top 5+ bits.
    if ((instr & 0xF800) == 0x1800)      thumb_add_sub(instr);            // 00011..
    else if ((instr & 0xE000) == 0x0000) thumb_shifted_register(instr);   // 000xx
    else if ((instr & 0xE000) == 0x2000) thumb_imm_alu(instr);            // 001xx
    else if ((instr & 0xFC00) == 0x4000) thumb_alu_op(instr);             // 010000
    else if ((instr & 0xFC00) == 0x4400) thumb_hi_reg_bx(instr);          // 010001
    else if ((instr & 0xF800) == 0x4800) thumb_pc_relative_load(instr);   // 01001
    else if ((instr & 0xF200) == 0x5000) thumb_load_store_reg_off(instr); // 0101xx0
    else if ((instr & 0xF200) == 0x5200) thumb_load_store_signed(instr);  // 0101xx1
    else if ((instr & 0xE000) == 0x6000) thumb_load_store_imm_off(instr); // 011xx
    else if ((instr & 0xF000) == 0x8000) thumb_load_store_halfword(instr);// 1000x
    else if ((instr & 0xF000) == 0x9000) thumb_sp_relative(instr);        // 1001x
    else if ((instr & 0xF000) == 0xA000) thumb_load_address(instr);       // 1010x
    else if ((instr & 0xFF00) == 0xB000) thumb_sp_add_offset(instr);      // 10110000
    else if ((instr & 0xF600) == 0xB400) thumb_push_pop(instr);           // 1011x10x
    else if ((instr & 0xF000) == 0xC000) thumb_multiple_load_store(instr);// 1100x
    else if ((instr & 0xFF00) == 0xDF00) thumb_swi(instr);                // 11011111
    else if ((instr & 0xF000) == 0xD000) thumb_conditional_branch(instr); // 1101
    else if ((instr & 0xF800) == 0xE000) thumb_unconditional_branch(instr);// 11100
    else if ((instr & 0xF000) == 0xF000) thumb_long_branch_link(instr);   // 1111
    else {
        NDS_LOG_WARN("arm7", "Undefined Thumb instruction 0x%04X at 0x%08X",
                     instr, pc);
    }
}

void Arm7::raise_swi() {
    const u32 next_pc = state_.thumb() ? state_.r[15] : state_.r[15];
    const u32 saved_cpsr = state_.cpsr;
    state_.switch_mode(Mode::Supervisor);
    state_.spsr_svc = saved_cpsr;
    state_.r[14] = next_pc;  // address of next instruction
    state_.cpsr &= ~kPsrT;
    state_.cpsr |= kPsrI;
    state_.r[15] = kExcVecSWI;
}

void Arm7::raise_undefined() {
    const u32 next_pc = state_.r[15];
    const u32 saved_cpsr = state_.cpsr;
    state_.switch_mode(Mode::Undefined);
    state_.spsr_und = saved_cpsr;
    state_.r[14] = next_pc;
    state_.cpsr &= ~kPsrT;
    state_.cpsr |= kPsrI;
    state_.r[15] = kExcVecUndefined;
}

}  // namespace nds::cpu
