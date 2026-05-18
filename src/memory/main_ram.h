#pragma once

#include "common/types.h"

#include <array>

namespace nds::memory {

// 4 MB main RAM, mirrored across 0x02000000–0x02FFFFFF on both CPUs.
class MainRam {
public:
    static constexpr usize kSize = 4 * 1024 * 1024;
    static constexpr u32   kBase = 0x02000000u;
    static constexpr u32   kMask = kSize - 1u;

    u8*       data()       { return data_.data(); }
    const u8* data() const { return data_.data(); }
    static constexpr usize size() { return kSize; }

private:
    std::array<u8, kSize> data_{};
};

}  // namespace nds::memory
