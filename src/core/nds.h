#pragma once

#include "cart/cartridge.h"
#include "cpu/arm7.h"
#include "cpu/arm9.h"
#include "memory/memory_map.h"

#include <string>

namespace nds::core {

// Top-level wiring: the cartridge, both CPUs, and the memory map. Owns no
// frontend resources (window, audio device, etc.) — those live in src/frontend.
class Nds {
public:
    Nds();

    // Load a .nds ROM, copy ARM9/ARM7 code into main RAM at the addresses
    // specified by the cartridge header, and reset both CPUs to their
    // respective entry points. Returns true on success.
    bool load_rom(const std::string& path);

    // Advance emulation by one frame. Currently a no-op until CPU and video
    // interpreters land.
    void run_frame();

    const cart::Cartridge& cartridge() const { return cart_; }
    memory::MemoryMap&     mem()             { return mem_; }
    cpu::Arm9&             arm9()            { return arm9_; }
    cpu::Arm7&             arm7()            { return arm7_; }

private:
    memory::MemoryMap mem_{};
    cpu::Arm9         arm9_;
    cpu::Arm7         arm7_;
    cart::Cartridge   cart_{};
};

}  // namespace nds::core
