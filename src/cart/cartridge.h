#pragma once

#include "cart/header.h"
#include "common/types.h"

#include <string>
#include <vector>

namespace nds::cart {

class Cartridge {
public:
    // Load an entire .nds file from disk. Returns true on success and parses
    // the header. On failure, logs an error and leaves the cartridge empty.
    bool load_from_file(const std::string& path);

    const Header& header() const { return header_; }
    const std::vector<u8>& rom() const { return rom_; }
    bool loaded() const { return !rom_.empty(); }

    // Copies the ARM9 / ARM7 program code from the ROM into a destination
    // buffer that represents the CPU's physical address space. `dst_size`
    // is the length of `dst`. Returns the number of bytes copied (0 on
    // bounds failure).
    usize copy_arm9_code(u8* dst, usize dst_size, u32 dst_base) const;
    usize copy_arm7_code(u8* dst, usize dst_size, u32 dst_base) const;

private:
    Header           header_{};
    std::vector<u8>  rom_{};

    usize copy_code(u32 rom_offset, u32 size, u32 ram_addr,
                    u8* dst, usize dst_size, u32 dst_base) const;
};

}  // namespace nds::cart
