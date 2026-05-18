#pragma once

#include "common/types.h"

#include <array>

namespace nds::memory {

// 32 KB shared WRAM at 0x03000000. WRAMCNT (ARM9 I/O 0x04000247) splits it
// between ARM9 and ARM7 in halves; the routing logic lives in memory_map.
class SharedWram {
public:
    static constexpr usize kSize = 32 * 1024;
    static constexpr u32   kBase = 0x03000000u;

    u8*       data()       { return data_.data(); }
    const u8* data() const { return data_.data(); }

    // Latest WRAMCNT (lower two bits used).
    u8 wramcnt() const { return wramcnt_; }
    void set_wramcnt(u8 v) { wramcnt_ = v & 0x03u; }

private:
    std::array<u8, kSize> data_{};
    u8 wramcnt_ = 0;
};

// ARM7-only 64 KB WRAM at 0x03800000–0x0380FFFF (mirrored to 0x037FFFFF block).
class Arm7Wram {
public:
    static constexpr usize kSize = 64 * 1024;
    static constexpr u32   kBase = 0x03800000u;

    u8*       data()       { return data_.data(); }
    const u8* data() const { return data_.data(); }

private:
    std::array<u8, kSize> data_{};
};

}  // namespace nds::memory
