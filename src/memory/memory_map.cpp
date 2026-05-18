#include "memory/memory_map.h"

#include "common/log.h"

#include <cstring>

namespace nds::memory {

namespace {

// Helper: locate the host pointer corresponding to a DS physical address.
// Returns nullptr if the address is not yet backed (I/O, VRAM, BIOS, etc.).
//
// GBATEK "DS Memory Map":
//   0200_0000-02FF_FFFF  Main RAM            (4 MB, mirrored)
//   0300_0000-037F_FFFF  Shared WRAM         (per WRAMCNT split)
//   0380_0000-03FF_FFFF  ARM7 WRAM           (ARM7 only, 64 KB mirrored)
//
// ARM9-only TCM regions are CP15-mapped and not yet wired here.
u8* resolve(MemoryMap& m, MemoryMap::Cpu cpu, u32 addr) {
    const u32 region = addr & 0xFF000000u;
    switch (region) {
        case 0x02000000u:
            return m.main_ram.data() + (addr & MainRam::kMask);
        case 0x03000000u: {
            if (cpu == MemoryMap::Cpu::Arm7 && (addr & 0x00800000u)) {
                return m.arm7_wram.data() + (addr & (Arm7Wram::kSize - 1));
            }
            return m.shared_wram.data() + (addr & (SharedWram::kSize - 1));
        }
        default:
            return nullptr;
    }
}

const u8* resolve(const MemoryMap& m, MemoryMap::Cpu cpu, u32 addr) {
    return resolve(const_cast<MemoryMap&>(m), cpu, addr);
}

}  // namespace

u8 MemoryMap::read8(Cpu cpu, u32 addr) const {
    if (const u8* p = resolve(*this, cpu, addr)) return *p;
    return 0;
}

u16 MemoryMap::read16(Cpu cpu, u32 addr) const {
    if (const u8* p = resolve(*this, cpu, addr & ~u32{1})) {
        u16 v;
        std::memcpy(&v, p, sizeof(v));
        return v;
    }
    return 0;
}

u32 MemoryMap::read32(Cpu cpu, u32 addr) const {
    if (const u8* p = resolve(*this, cpu, addr & ~u32{3})) {
        u32 v;
        std::memcpy(&v, p, sizeof(v));
        return v;
    }
    return 0;
}

void MemoryMap::write8(Cpu cpu, u32 addr, u8 v) {
    if (u8* p = resolve(*this, cpu, addr)) { *p = v; return; }
    NDS_LOG_DEBUG("mem", "stub write8  cpu=%d addr=0x%08X val=0x%02X",
                  static_cast<int>(cpu), addr, v);
}

void MemoryMap::write16(Cpu cpu, u32 addr, u16 v) {
    if (u8* p = resolve(*this, cpu, addr & ~u32{1})) {
        std::memcpy(p, &v, sizeof(v));
        return;
    }
    NDS_LOG_DEBUG("mem", "stub write16 cpu=%d addr=0x%08X val=0x%04X",
                  static_cast<int>(cpu), addr, v);
}

void MemoryMap::write32(Cpu cpu, u32 addr, u32 v) {
    if (u8* p = resolve(*this, cpu, addr & ~u32{3})) {
        std::memcpy(p, &v, sizeof(v));
        return;
    }
    NDS_LOG_DEBUG("mem", "stub write32 cpu=%d addr=0x%08X val=0x%08X",
                  static_cast<int>(cpu), addr, v);
}

}  // namespace nds::memory
