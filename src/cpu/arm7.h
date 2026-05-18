#pragma once

#include "cpu/arm_core.h"
#include "common/types.h"

namespace nds::memory { class MemoryMap; }

namespace nds::cpu {

// ARM7TDMI (ARMv4T). Full instruction interpreter lands in milestone M1.
// For now this struct holds the architectural state and a no-op step().
class Arm7 {
public:
    explicit Arm7(memory::MemoryMap& mem) : mem_(mem) {}

    void reset_to_entry(u32 entry);

    // Execute one instruction. Currently a stub; returns the number of
    // cycles the (not-yet-executed) instruction would take, conservatively 1.
    u32 step();

    ArmState& state() { return state_; }
    const ArmState& state() const { return state_; }

private:
    ArmState           state_{};
    memory::MemoryMap& mem_;
};

}  // namespace nds::cpu
