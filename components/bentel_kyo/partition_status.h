/*
 * Partition-status response validity check, kept standalone so it can be
 * unit-tested without pulling in the ESPHome framework (see cp437.h).
 */

#pragma once

#include <cstdint>

namespace esphome {
namespace bentel_kyo {

// A configured partition is always in exactly one of armed_total/armed_partial/
// armed_partial_delay0/disarmed, so a real response always has at least one bit set
// across rx[6..9]. All-zero across all four means the queried register isn't mapped
// on this panel (issue #122: some KYO32 non-G units read all-zero at 0x14EC, where
// 0x1502 is the register that actually carries partition data).
inline bool partition_status_bits_all_absent(const uint8_t *rx) {
  return (rx[6] | rx[7] | rx[8] | rx[9]) == 0;
}

}  // namespace bentel_kyo
}  // namespace esphome
