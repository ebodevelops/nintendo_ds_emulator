// Unit tests for the ARM7TDMI interpreter. Each test assembles a short
// instruction stream into main RAM and runs the CPU one or more steps,
// then checks register/flag state.

#include "test_harness.h"

#include "cpu/arm7.h"
#include "cpu/arm_core.h"
#include "memory/memory_map.h"

#include <cstring>

using nds::cpu::Arm7;
using nds::memory::MemoryMap;

namespace {

constexpr nds::u32 kBase = 0x02000000;

struct Fixture {
    MemoryMap mem;
    Arm7      cpu{mem};

    void load_arm(std::initializer_list<nds::u32> code) {
        nds::u32 addr = kBase;
        for (auto w : code) {
            mem.write32(MemoryMap::Cpu::Arm7, addr, w);
            addr += 4;
        }
        cpu.reset_to_entry(kBase);
        cpu.state().cpsr = static_cast<nds::u32>(nds::cpu::Mode::System);
    }

    void load_thumb(std::initializer_list<nds::u16> code) {
        nds::u32 addr = kBase;
        for (auto h : code) {
            mem.write16(MemoryMap::Cpu::Arm7, addr, h);
            addr += 2;
        }
        cpu.reset_to_entry(kBase);
        cpu.state().cpsr = static_cast<nds::u32>(nds::cpu::Mode::System) | nds::cpu::kPsrT;
    }

    void run(int n) { for (int i = 0; i < n; ++i) cpu.step(); }

    nds::u32 r(int i) const { return cpu.state().r[i]; }
};

}  // namespace

// ---------------------- ARM mode ----------------------

NDS_TEST(arm_mov_imm) {
    Fixture f;
    // MOV r0, #42
    f.load_arm({0xE3A0002A});
    f.run(1);
    EXPECT_EQ(f.r(0), 42u);
}

NDS_TEST(arm_add_reg) {
    Fixture f;
    // MOV r0, #5 ; MOV r1, #7 ; ADD r2, r0, r1
    f.load_arm({0xE3A00005, 0xE3A01007, 0xE0802001});
    f.run(3);
    EXPECT_EQ(f.r(2), 12u);
}

NDS_TEST(arm_subs_flags) {
    Fixture f;
    // MOV r0, #5 ; SUBS r1, r0, #10  -> r1 = -5, N=1, C=0 (borrow), V=0
    f.load_arm({0xE3A00005, 0xE250100A});
    f.run(2);
    EXPECT_EQ(f.r(1), 0xFFFFFFFBu);
    EXPECT_TRUE(f.cpu.state().flag_n());
    EXPECT_FALSE(f.cpu.state().flag_z());
    EXPECT_FALSE(f.cpu.state().flag_c());
    EXPECT_FALSE(f.cpu.state().flag_v());
}

NDS_TEST(arm_branch_link) {
    Fixture f;
    // MOV r0, #1 ; BL +8 (skip MOV r0,#0xFF) ; MOV r0, #0xFF ; MOV r1, #2
    // Layout (4 instrs):
    //   0x00: MOV r0, #1
    //   0x04: BL  -> 0x10
    //   0x08: MOV r0, #0xFF  (should be skipped)
    //   0x0C: MOV r1, #2
    //   0x10: MOV r2, #3
    // BL offset = ((target - (PC+8)) >> 2) where PC = instr addr.
    // Here BL is at 0x04, PC+8 = 0x0C, target 0x10 -> offset = (0x10-0x0C)/4 = 1.
    f.load_arm({
        0xE3A00001,           // 0x00: mov r0, #1
        0xEB000001,           // 0x04: bl 0x10  (offset = 1)
        0xE3A000FF,           // 0x08: mov r0, #0xFF (skipped)
        0xE3A01002,           // 0x0C: mov r1, #2
        0xE3A02003,           // 0x10: mov r2, #3
    });
    f.run(3);  // mov, bl, mov r2
    EXPECT_EQ(f.r(0), 1u);
    EXPECT_EQ(f.r(2), 3u);
    EXPECT_EQ(f.r(14), kBase + 0x08);  // LR = address of next instr after BL
}

NDS_TEST(arm_str_ldr) {
    Fixture f;
    // MOV r0, #0xAB ; MOV r1, #0 ; ORR r1, r1, #0x02000000 ; ADD r1, r1, #0x100
    // STR r0, [r1] ; LDR r2, [r1]
    f.load_arm({
        0xE3A000AB,
        0xE3A01000,
        0xE3811402,           // ORR r1, r1, #0x02000000 (imm rotated)
        0xE2811C01,           // ADD r1, r1, #0x100
        0xE5810000,           // STR r0, [r1]
        0xE5912000,           // LDR r2, [r1]
    });
    f.run(6);
    EXPECT_EQ(f.r(2), 0xABu);
}

NDS_TEST(arm_block_transfer) {
    Fixture f;
    // MOV r0, #1 ; MOV r1, #2 ; MOV r2, #3
    // MOV sp, #0x02100000
    // STMDB sp!, {r0,r1,r2}  -> writes 3 words
    // LDMIA sp!, {r4,r5,r6}  -> reads them back
    f.load_arm({
        0xE3A00001,
        0xE3A01002,
        0xE3A02003,
        0xE3A0D801,           // MOV sp, #0x10000 << 8 = 0x00010000... actually let's use a real address
        0xE38DD402,           // ORR sp, sp, #0x02000000
        0xE28DDC01,           // ADD sp, sp, #0x100  ; sp = 0x02010100
        0xE92D0007,           // STMDB sp!, {r0,r1,r2}
        0xE8BD0070,           // LDMIA sp!, {r4,r5,r6}
    });
    f.run(8);
    EXPECT_EQ(f.r(4), 1u);
    EXPECT_EQ(f.r(5), 2u);
    EXPECT_EQ(f.r(6), 3u);
}

NDS_TEST(arm_multiply) {
    Fixture f;
    // MOV r0,#7 ; MOV r1,#6 ; MUL r2, r0, r1
    f.load_arm({0xE3A00007, 0xE3A01006, 0xE0020091});
    f.run(3);
    EXPECT_EQ(f.r(2), 42u);
}

NDS_TEST(arm_cmp_branch_eq) {
    Fixture f;
    // MOV r0,#5 ; CMP r0,#5 ; BEQ skip ; MOV r1,#0xFF ; skip: MOV r2,#1
    f.load_arm({
        0xE3A00005,           // mov r0,#5
        0xE3500005,           // cmp r0,#5
        0x0A000000,           // beq +0   (skip the next instruction)
        0xE3A010FF,           // mov r1,#0xFF (skipped)
        0xE3A02001,           // mov r2,#1
    });
    f.run(4);
    EXPECT_EQ(f.r(0), 5u);
    EXPECT_EQ(f.r(1), 0u);
    EXPECT_EQ(f.r(2), 1u);
}

// ---------------------- Thumb mode ----------------------

NDS_TEST(thumb_mov_add) {
    Fixture f;
    // MOV r0, #5 (F3 imm form: 00100 dst[2:0] imm8)
    // ADD r1, r0, #2 (F2 imm: 0001110 imm3 src dst)
    f.load_thumb({
        0x2005,               // mov r0, #5
        0x1C41,               // add r1, r0, #0... actually mov-reg form. Use 0x1C82 for add r2,r0,#2
    });
    // Use a cleaner sequence:
    f.load_thumb({
        0x2005,               // mov r0, #5
        0x1C82,               // adds r2, r0, #2 (F2 imm: 000111 imm src(=0) dst(=2))
    });
    f.run(2);
    EXPECT_EQ(f.r(0), 5u);
    EXPECT_EQ(f.r(2), 7u);
}

NDS_TEST(thumb_branch_cond) {
    Fixture f;
    // mov r0,#3 ; cmp r0,#3 ; beq +2 ; mov r1,#0xFF ; mov r2,#9
    f.load_thumb({
        0x2003,               // mov r0,#3
        0x2803,               // cmp r0,#3
        0xD000,               // beq +0 (skips one halfword: the mov r1)
        0x21FF,               // mov r1,#0xFF (should be skipped)
        0x2209,               // mov r2,#9
    });
    f.run(4);
    EXPECT_EQ(f.r(0), 3u);
    EXPECT_EQ(f.r(1), 0u);
    EXPECT_EQ(f.r(2), 9u);
}

NDS_TEST(thumb_push_pop) {
    Fixture f;
    f.load_thumb({
        0x2011,               // mov r0, #0x11
        0x2122,               // mov r1, #0x22
        0xB403,               // push {r0, r1}
        0xBC0C,               // pop  {r2, r3}
    });
    f.cpu.state().r[13] = kBase + 0x100;  // SP points inside main RAM
    f.run(4);
    EXPECT_EQ(f.r(2), 0x11u);
    EXPECT_EQ(f.r(3), 0x22u);
}

NDS_TEST(thumb_ldr_pc_relative) {
    Fixture f;
    // Encoding: 01001 Rd[2:0] imm8. Effective addr = (PC&~3) + imm8*4,
    // where PC = instr_addr + 4 in Thumb. With instr at addr 0:
    //   PC = 4, aligned = 4, +imm8*4. imm8=1 -> addr 8.
    // Layout (halfwords):
    //   0: ldr r0, [pc, #4]
    //   2: nop
    //   4: nop
    //   6: nop  (padding to align the 32-bit constant on a 4-byte boundary)
    //   8..B: constant 0xBEEFDEAD (little-endian halfwords: 0xDEAD,0xBEEF)
    f.load_thumb({
        0x4801,
        0x46C0,
        0x46C0,
        0x46C0,
        0xDEAD, 0xBEEF,
    });
    f.run(1);
    EXPECT_EQ(f.r(0), 0xBEEFDEADu);
}

NDS_TEST(thumb_long_branch_link) {
    Fixture f;
    // BL is a pair of halfwords: hi (H=0) sets LR = (PC + 4) + (sign-ext(off)<<12);
    // lo (H=1) sets PC = LR + (off<<1), LR = next_pc | 1.
    // Want to branch from addr 0 to addr 6 (skip the MOV r0 only).
    //   hi-half at 0: LR = 4 + 0 = 4.
    //   lo-half at 2: target = 4 + (off<<1) = 6  ->  off = 1.
    f.load_thumb({
        0xF000,               // BL hi (offset = 0)
        0xF801,               // BL lo (offset = 1 -> target = LR + 2 = 6)
        0x20FF,               // mov r0, #0xFF (skipped)
        0x2155,               // mov r1, #0x55
        0x2277,               // mov r2, #0x77
    });
    f.run(4);  // BL hi, BL lo, mov r1, mov r2
    EXPECT_EQ(f.r(0), 0u);
    EXPECT_EQ(f.r(1), 0x55u);
    EXPECT_EQ(f.r(2), 0x77u);
}

NDS_TEST(arm_msr_mrs) {
    Fixture f;
    // MOV r0, #0xF0000000 ; MSR CPSR_f, r0 ; MRS r1, CPSR
    // Use immediate rotated: #0xF0000000 = imm 0xF rotated right by 4*2=8? -> rotate field = (32-28)/2 = 2
    // Actually 0xF0000000 == ror(0xF, 4). rotate-field value is 4/2 = 2? GBATEK: rot = field*2.
    // ror(0xF, 4) but we need ror by 4 -> field = (32-4)/2 = 14? Let me think.
    // The shifter rotates imm right by (rot*2). To get 0xF0000000 from imm=0xF we need ror by 4
    // which means rot field = 2 (since rot*2 = 4 -> wait no, rot field*2 = 4 means rot=2).
    // Hmm rot field 2 -> ror by 4. ror(0xF,4) = 0xF0000000. Good.
    // Encoding: cond=E, 001 1101 0 0000 0000 rot=2 imm=0xF
    // = 0xE3A0_020F
    f.load_arm({
        0xE3A0020F,           // mov r0, #0xF0000000
        0xE128F000,           // msr CPSR_f, r0
        0xE10F1000,           // mrs r1, CPSR
    });
    f.run(3);
    EXPECT_EQ(f.r(0), 0xF0000000u);
    EXPECT_EQ(f.r(1) & 0xF0000000u, 0xF0000000u);
}

NDS_TEST(arm_swi_switches_to_supervisor) {
    Fixture f;
    f.load_arm({0xEF000001});  // SWI #1
    f.run(1);
    EXPECT_EQ(static_cast<nds::u32>(f.cpu.state().mode()),
              static_cast<nds::u32>(nds::cpu::Mode::Supervisor));
    EXPECT_EQ(f.cpu.state().r[15], 0x00000008u);
    EXPECT_TRUE(f.cpu.state().irq_disabled());
}

int main() {
    return nds::testing::run_all();
}
