#include "cart/header.h"

#include "common/log.h"

#include <cstring>

namespace nds::cart {

namespace {

u32 read_u32_le(const u8* p) {
    return  u32{p[0]}
         | (u32{p[1]} <<  8)
         | (u32{p[2]} << 16)
         | (u32{p[3]} << 24);
}

u16 read_u16_le(const u8* p) {
    return static_cast<u16>(u16{p[0]} | (u16{p[1]} << 8));
}

}  // namespace

bool Header::parse(const u8* data, usize size) {
    if (data == nullptr || size < kSize) return false;

    std::memcpy(game_title.data(), data + 0x000, game_title.size());
    std::memcpy(game_code.data(),  data + 0x00C, game_code.size());
    std::memcpy(maker_code.data(), data + 0x010, maker_code.size());

    unit_code       = data[0x012];
    device_type     = data[0x013];
    device_capacity = data[0x014];
    region          = data[0x01D];
    rom_version     = data[0x01E];

    arm9_rom_offset = read_u32_le(data + 0x020);
    arm9_entry      = read_u32_le(data + 0x024);
    arm9_ram_addr   = read_u32_le(data + 0x028);
    arm9_size       = read_u32_le(data + 0x02C);

    arm7_rom_offset = read_u32_le(data + 0x030);
    arm7_entry      = read_u32_le(data + 0x034);
    arm7_ram_addr   = read_u32_le(data + 0x038);
    arm7_size       = read_u32_le(data + 0x03C);

    header_checksum = read_u16_le(data + 0x15E);
    return true;
}

std::string Header::title_string() const {
    std::string s(game_title.data(), game_title.size());
    while (!s.empty() && (s.back() == '\0' || s.back() == ' ')) s.pop_back();
    return s;
}

std::string Header::game_code_string() const {
    return std::string(game_code.data(), game_code.size());
}

void Header::log_summary() const {
    NDS_LOG_INFO("cart", "Title       : \"%s\"", title_string().c_str());
    NDS_LOG_INFO("cart", "Game code   : %s   maker: %c%c   unit: %u   rev: %u",
                 game_code_string().c_str(),
                 maker_code[0] ? maker_code[0] : '?',
                 maker_code[1] ? maker_code[1] : '?',
                 unit_code, rom_version);
    NDS_LOG_INFO("cart", "Cart size   : %u KB", 128u << device_capacity);
    NDS_LOG_INFO("cart", "ARM9  rom=0x%08X entry=0x%08X ram=0x%08X size=0x%08X",
                 arm9_rom_offset, arm9_entry, arm9_ram_addr, arm9_size);
    NDS_LOG_INFO("cart", "ARM7  rom=0x%08X entry=0x%08X ram=0x%08X size=0x%08X",
                 arm7_rom_offset, arm7_entry, arm7_ram_addr, arm7_size);
}

}  // namespace nds::cart
