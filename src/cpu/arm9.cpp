#include "cpu/arm9.h"

#include "common/log.h"
#include "memory/memory_map.h"

namespace nds::cpu {

void Arm9::reset_to_entry(u32 entry) {
    state_.reset(entry);
    NDS_LOG_INFO("arm9", "Reset; PC = 0x%08X", entry);
}

u32 Arm9::step() {
    // TODO(M2): fetch/decode/execute ARMv5TE (ARM + Thumb) plus CP15.
    (void)mem_;
    return 1;
}

}  // namespace nds::cpu
