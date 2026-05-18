#pragma once

#include "common/types.h"
#include "memory/main_ram.h"
#include "memory/shared_wram.h"
#include "memory/tcm.h"

namespace nds::memory {

// Holds physical memory backing stores. The actual address-decode routing
// lives in MemoryMap::read*/write*, which dispatch to whichever store
// corresponds to a given DS physical address per GBATEK "Memory Map".
//
// This first pass implements main RAM, shared WRAM, ARM7 WRAM, and the
// ARM9 TCMs. Everything else (I/O, VRAM, palette, OAM, GBA slot, BIOS)
// reads as 0 and warns on write, to be filled in as later milestones land.
class MemoryMap {
public:
    enum class Cpu { Arm9, Arm7 };

    MainRam     main_ram;
    SharedWram  shared_wram;
    Arm7Wram    arm7_wram;
    Itcm        itcm;
    Dtcm        dtcm;

    u8  read8 (Cpu cpu, u32 addr) const;
    u16 read16(Cpu cpu, u32 addr) const;
    u32 read32(Cpu cpu, u32 addr) const;

    void write8 (Cpu cpu, u32 addr, u8  v);
    void write16(Cpu cpu, u32 addr, u16 v);
    void write32(Cpu cpu, u32 addr, u32 v);

    // Direct access to main RAM as a contiguous buffer (used by cartridge
    // direct-boot to copy ARM9/ARM7 program code in place).
    u8*    main_ram_data()       { return main_ram.data(); }
    usize  main_ram_size() const { return MainRam::size(); }
    u32    main_ram_base() const { return MainRam::kBase; }
};

}  // namespace nds::memory
