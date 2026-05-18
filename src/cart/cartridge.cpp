#include "cart/cartridge.h"

#include "common/log.h"

#include <cstdio>
#include <cstring>

namespace nds::cart {

bool Cartridge::load_from_file(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        NDS_LOG_ERROR("cart", "Could not open ROM file: %s", path.c_str());
        return false;
    }

    std::fseek(f, 0, SEEK_END);
    const long len = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    if (len < static_cast<long>(Header::kSize)) {
        NDS_LOG_ERROR("cart", "ROM too small (%ld bytes) to contain header", len);
        std::fclose(f);
        return false;
    }

    rom_.resize(static_cast<usize>(len));
    const usize read = std::fread(rom_.data(), 1, rom_.size(), f);
    std::fclose(f);

    if (read != rom_.size()) {
        NDS_LOG_ERROR("cart", "Short read on ROM (%zu/%zu bytes)", read, rom_.size());
        rom_.clear();
        return false;
    }

    if (!header_.parse(rom_.data(), rom_.size())) {
        NDS_LOG_ERROR("cart", "Failed to parse cartridge header");
        rom_.clear();
        return false;
    }

    NDS_LOG_INFO("cart", "Loaded ROM: %s (%zu bytes)", path.c_str(), rom_.size());
    header_.log_summary();
    return true;
}

usize Cartridge::copy_code(u32 rom_offset, u32 size, u32 ram_addr,
                           u8* dst, usize dst_size, u32 dst_base) const {
    if (size == 0) return 0;
    if (rom_offset >= rom_.size() || rom_offset + size > rom_.size()) {
        NDS_LOG_WARN("cart", "Program region out of ROM bounds (off=0x%X size=0x%X)",
                     rom_offset, size);
        return 0;
    }
    if (ram_addr < dst_base || ram_addr - dst_base + size > dst_size) {
        NDS_LOG_WARN("cart",
                     "Program load address 0x%08X outside provided buffer "
                     "[0x%08X, 0x%08zX)", ram_addr, dst_base, dst_base + dst_size);
        return 0;
    }
    std::memcpy(dst + (ram_addr - dst_base), rom_.data() + rom_offset, size);
    return size;
}

usize Cartridge::copy_arm9_code(u8* dst, usize dst_size, u32 dst_base) const {
    return copy_code(header_.arm9_rom_offset, header_.arm9_size,
                     header_.arm9_ram_addr, dst, dst_size, dst_base);
}

usize Cartridge::copy_arm7_code(u8* dst, usize dst_size, u32 dst_base) const {
    return copy_code(header_.arm7_rom_offset, header_.arm7_size,
                     header_.arm7_ram_addr, dst, dst_size, dst_base);
}

}  // namespace nds::cart
