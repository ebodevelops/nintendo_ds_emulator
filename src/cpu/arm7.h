#pragma once

#include "common/types.h"
#include "cpu/arm_core.h"

namespace nds::memory { class MemoryMap; }

namespace nds::cpu {

// ARM7TDMI (ARMv4T) interpreter.
//
// Pipeline model: instructions are fetched at PC, and during execute the
// architectural PC reads as PC+8 (ARM) / PC+4 (Thumb). We keep `state_.r[15]`
// pointing at the *instruction being executed*; helpers in the implementation
// account for the +8/+4 read offset whenever the program reads R15.
class Arm7 {
public:
    explicit Arm7(memory::MemoryMap& mem) : mem_(mem) {}

    void reset_to_entry(u32 entry);

    // Execute one instruction. Returns the number of cycles consumed
    // (approximate; cycle-accuracy comes later).
    u32 step();

    ArmState&       state()       { return state_; }
    const ArmState& state() const { return state_; }

    // Signal an exception entry. Exposed so timers/IRQ controllers can poke
    // the CPU once those land.
    void raise_swi();
    void raise_undefined();

private:
    // Memory helpers (handle access widths and addr alignment).
    u32 read32(u32 addr);
    u32 read16(u32 addr);
    u32 read8 (u32 addr);
    void write32(u32 addr, u32 v);
    void write16(u32 addr, u16 v);
    void write8 (u32 addr, u8  v);

    // PC as observed by the program (current PC + 8 ARM / +4 Thumb).
    u32 read_pc() const;

    // Dispatch.
    void step_arm();
    void step_thumb();

    // ----- ARM instruction handlers -----
    void arm_data_processing(u32 instr);
    void arm_psr_transfer   (u32 instr);
    void arm_multiply       (u32 instr);
    void arm_multiply_long  (u32 instr);
    void arm_single_swap    (u32 instr);
    void arm_branch_exchange(u32 instr);
    void arm_halfword_signed(u32 instr);
    void arm_single_transfer(u32 instr);
    void arm_block_transfer (u32 instr);
    void arm_branch         (u32 instr);
    void arm_swi            (u32 instr);
    void arm_undefined      (u32 instr);

    // ----- Thumb instruction handlers (one per format) -----
    void thumb_shifted_register     (u16 instr);   // F1
    void thumb_add_sub              (u16 instr);   // F2
    void thumb_imm_alu              (u16 instr);   // F3
    void thumb_alu_op               (u16 instr);   // F4
    void thumb_hi_reg_bx            (u16 instr);   // F5
    void thumb_pc_relative_load     (u16 instr);   // F6
    void thumb_load_store_reg_off   (u16 instr);   // F7
    void thumb_load_store_signed    (u16 instr);   // F8
    void thumb_load_store_imm_off   (u16 instr);   // F9
    void thumb_load_store_halfword  (u16 instr);   // F10
    void thumb_sp_relative          (u16 instr);   // F11
    void thumb_load_address         (u16 instr);   // F12
    void thumb_sp_add_offset        (u16 instr);   // F13
    void thumb_push_pop             (u16 instr);   // F14
    void thumb_multiple_load_store  (u16 instr);   // F15
    void thumb_conditional_branch   (u16 instr);   // F16
    void thumb_swi                  (u16 instr);   // F17
    void thumb_unconditional_branch (u16 instr);   // F18
    void thumb_long_branch_link     (u16 instr);   // F19

    ArmState           state_{};
    memory::MemoryMap& mem_;
};

}  // namespace nds::cpu
