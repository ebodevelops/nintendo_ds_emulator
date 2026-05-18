#pragma once

#include "common/types.h"

#include <array>

namespace nds::memory {

// ARM9 Tightly-Coupled Memory.
// ITCM: 32 KB physical, mapped via CP15 (size + base configurable; default
//       0x00000000–0x00007FFF).
// DTCM: 16 KB physical, mapped via CP15 (typical base 0x027C0000).
class Itcm {
public:
    static constexpr usize kSize = 32 * 1024;
    u8*       data()       { return data_.data(); }
    const u8* data() const { return data_.data(); }
private:
    std::array<u8, kSize> data_{};
};

class Dtcm {
public:
    static constexpr usize kSize = 16 * 1024;
    u8*       data()       { return data_.data(); }
    const u8* data() const { return data_.data(); }
private:
    std::array<u8, kSize> data_{};
};

}  // namespace nds::memory
