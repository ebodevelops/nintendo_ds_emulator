#include "cpu/arm7.h"

#include "common/log.h"
#include "memory/memory_map.h"

namespace nds::cpu {

void Arm7::reset_to_entry(u32 entry) {
    state_.reset(entry);
    NDS_LOG_INFO("arm7", "Reset; PC = 0x%08X", entry);
}

u32 Arm7::step() {
    // TODO(M1): fetch/decode/execute ARMv4T (ARM + Thumb).
    (void)mem_;
    return 1;
}

}  // namespace nds::cpu
