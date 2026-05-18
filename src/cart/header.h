#pragma once

#include "common/types.h"

#include <array>
#include <cstddef>
#include <string>

namespace nds::cart {

// Nintendo DS cartridge header. Layout per GBATEK "DS Cartridge Header".
// Only the fields relevant to direct-boot are mirrored here; the raw 0x180
// bytes remain available via raw().
struct Header {
    static constexpr usize kSize = 0x180;

    std::array<char, 12> game_title{};   // 0x000  (NUL-padded)
    std::array<char, 4>  game_code{};    // 0x00C
    std::array<char, 2>  maker_code{};   // 0x010
    u8  unit_code = 0;                   // 0x012  (0=DS, 2=DSi+DS, 3=DSi only)
    u8  device_type = 0;                 // 0x013
    u8  device_capacity = 0;             // 0x014  (cart size = 128 KB << this)
    u8  region = 0;                      // 0x01D
    u8  rom_version = 0;                 // 0x01E

    u32 arm9_rom_offset = 0;             // 0x020
    u32 arm9_entry      = 0;             // 0x024
    u32 arm9_ram_addr   = 0;             // 0x028
    u32 arm9_size       = 0;             // 0x02C

    u32 arm7_rom_offset = 0;             // 0x030
    u32 arm7_entry      = 0;             // 0x034
    u32 arm7_ram_addr   = 0;             // 0x038
    u32 arm7_size       = 0;             // 0x03C

    u16 header_checksum = 0;             // 0x15E (CRC-16 over 0x000..0x15D)

    // Parse the first kSize bytes of `data`. Returns true on success.
    bool parse(const u8* data, usize size);

    // Convenience: pretty-print parsed fields via the logger.
    void log_summary() const;

    // Title as a printable, NUL-terminated string.
    std::string title_string() const;
    std::string game_code_string() const;
};

}  // namespace nds::cart
