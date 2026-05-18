#pragma once

#include "cpu/arm_core.h"
#include "common/types.h"

namespace nds::memory { class MemoryMap; }

namespace nds::cpu {

// ARM946E-S (ARMv5TE) + CP15. Full interpreter lands in milestone M2.
class Arm9 {
public:
    explicit Arm9(memory::MemoryMap& mem) : mem_(mem) {}

    void reset_to_entry(u32 entry);

    u32 step();

    ArmState& state() { return state_; }
    const ArmState& state() const { return state_; }

private:
    ArmState           state_{};
    memory::MemoryMap& mem_;
};

}  // namespace nds::cpu
