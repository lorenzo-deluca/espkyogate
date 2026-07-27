/*
 * Standalone unit test for partition_status_bits_all_absent() (issue #122).
 *
 * Compile:  g++ -std=c++17 -o test_partition_status_fallback tests/test_partition_status_fallback.cpp && ./test_partition_status_fallback
 */

#include "../components/bentel_kyo/partition_status.h"

#include <cstdio>

using esphome::bentel_kyo::partition_status_bits_all_absent;

static int tests_run = 0;
static int tests_passed = 0;

#define EXPECT(actual, expected)                                                              \
  do {                                                                                        \
    tests_run++;                                                                              \
    if ((actual) == (expected)) {                                                             \
      tests_passed++;                                                                         \
    } else {                                                                                  \
      std::fprintf(stderr, "FAIL %s:%d: expected %d, got %d\n", __FILE__, __LINE__, (expected), \
                   (actual));                                                                  \
    }                                                                                          \
  } while (0)

int main() {
  // --- Real capture from issue #122: working register 0x1502, disarmed panel ---
  // "F0 02 15 12 00 19 00 00 00 FF 00 FF FA ..." -> rx[9]=0xFF (all partitions disarmed).
  // This is also the exact byte pattern the #118 bad register produced (FF padding
  // read as "always disarmed"), so this case doubles as a regression guard: it must
  // NOT be flagged as unmapped, or the #118 fix (defaulting KYO_32 to 0x14EC) would
  // never get a chance to stick.
  {
    const uint8_t rx[] = {0xF0, 0x02, 0x15, 0x12, 0x00, 0x19, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0xFA};
    EXPECT(partition_status_bits_all_absent(rx), false);
  }

  // --- Real capture from issue #122: broken register 0x14EC, same panel state ---
  // "F0 EC 14 12 00 02 00 00 00 00 00 00 ..." -> rx[6..9] all zero, no partition in
  // any arming state at all. This is the pattern that must trigger the fallback.
  {
    const uint8_t rx[] = {0xF0, 0xEC, 0x14, 0x12, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    EXPECT(partition_status_bits_all_absent(rx), true);
  }

  // --- Partition armed totally (bit 0 of rx[6]) ---
  {
    const uint8_t rx[] = {0, 0, 0, 0, 0, 0, 0x01, 0x00, 0x00, 0x00, 0, 0, 0};
    EXPECT(partition_status_bits_all_absent(rx), false);
  }

  // --- Two partitions, one armed total, one disarmed ---
  {
    const uint8_t rx[] = {0, 0, 0, 0, 0, 0, 0x01, 0x00, 0x00, 0x02, 0, 0, 0};
    EXPECT(partition_status_bits_all_absent(rx), false);
  }

  // --- Only rx[6..9] matter: other bytes being non-zero doesn't change the verdict ---
  {
    const uint8_t rx[] = {0, 0, 0, 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0xAB, 0xCD, 0xEF};
    EXPECT(partition_status_bits_all_absent(rx), true);
  }
  {
    const uint8_t rx[] = {0, 0, 0, 0, 0, 0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
    EXPECT(partition_status_bits_all_absent(rx), false);
  }

  std::printf("%d/%d tests passed\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
