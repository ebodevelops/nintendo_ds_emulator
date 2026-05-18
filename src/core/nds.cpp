#include "core/nds.h"

#include "common/log.h"

namespace nds::core {

Nds::Nds() : arm9_(mem_), arm7_(mem_) {}

bool Nds::load_rom(const std::string& path) {
    if (!cart_.load_from_file(path)) return false;

    // Direct boot: copy ARM9 and ARM7 program code from the ROM into main
    // RAM at the addresses specified by the cartridge header.
    const auto n9 = cart_.copy_arm9_code(mem_.main_ram_data(), mem_.main_ram_size(),
                                         mem_.main_ram_base());
    const auto n7 = cart_.copy_arm7_code(mem_.main_ram_data(), mem_.main_ram_size(),
                                         mem_.main_ram_base());
    NDS_LOG_INFO("core", "Direct-boot copy: %zu bytes ARM9, %zu bytes ARM7", n9, n7);

    arm9_.reset_to_entry(cart_.header().arm9_entry);
    arm7_.reset_to_entry(cart_.header().arm7_entry);
    return true;
}

void Nds::run_frame() {
    // Placeholder timing: step each CPU for a fixed number of instructions
    // per frame. Real cycle accounting + scheduler comes in M3.
    constexpr u32 kArm9StepsPerFrame = 1 * 1024 * 1024;  // ~67 MHz / 60 Hz
    constexpr u32 kArm7StepsPerFrame =     512 * 1024;   // ~33 MHz / 60 Hz
    for (u32 i = 0; i < kArm9StepsPerFrame; ++i) arm9_.step();
    for (u32 i = 0; i < kArm7StepsPerFrame; ++i) arm7_.step();
}

}  // namespace nds::core
