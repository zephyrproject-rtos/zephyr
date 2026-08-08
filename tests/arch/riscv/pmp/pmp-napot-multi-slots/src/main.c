/*
 * Copyright (c) 2026 Picoheart Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/pmp.h>

/*
 * Non-naturally-aligned region: start=0x81010800, size=0x2000.
 *
 * The start address (0x81010800) is not aligned to the region size (0x2000),
 * so it cannot be covered by a single NAPOT entry. With
 * CONFIG_PMP_NAPOT_USE_MULTI_SLOTS enabled, set_pmp_entry() splits it
 * into multiple naturally-aligned NAPOT blocks:
 *
 *   [0x81010800, 0x81011000)  size=0x800   (max aligned block at 0x...800)
 *   [0x81011000, 0x81012000)  size=0x1000  (max aligned block at 0x...1000)
 *   [0x81012000, 0x81012800)  size=0x800   (limited by remaining size)
 */
#define TEST_REGION_ADDR 0x81010800UL
#define TEST_REGION_SIZE 0x2000UL
#define TEST_REGION_END  (TEST_REGION_ADDR + TEST_REGION_SIZE)
#define TEST_REGION_PERM (PMP_R | PMP_W)

#define EXPECTED_ENTRY_COUNT 3

struct expected_napot {
	unsigned long start;
	unsigned long end; /* inclusive */
	bool found;
};

static struct expected_napot expected_entries[EXPECTED_ENTRY_COUNT] = {
	{0x81010800UL, 0x81010FFFUL, false},
	{0x81011000UL, 0x81011FFFUL, false},
	{0x81012000UL, 0x810127FFUL, false},
};

PMP_SOC_REGION_DEFINE(test_region_napot_multi, TEST_REGION_ADDR, TEST_REGION_END, TEST_REGION_PERM);

/*
 * Collect all NAPOT entries that fall within the test region.
 * Returns the number of entries found and fills the out array.
 */
static unsigned int collect_napot_entries(unsigned long *starts, unsigned long *ends,
					  uint8_t *perms, unsigned int max_entries)
{
	const size_t num_pmpcfg_regs = CONFIG_PMP_SLOTS / sizeof(unsigned long);
	const size_t num_pmpaddr_regs = CONFIG_PMP_SLOTS;

	unsigned long pmpcfg[num_pmpcfg_regs];
	unsigned long pmpaddr[num_pmpaddr_regs];
	const uint8_t *const cfg = (const uint8_t *)pmpcfg;
	unsigned int count = 0;

	z_riscv_pmp_read_config(pmpcfg, ARRAY_SIZE(pmpcfg));
	z_riscv_pmp_read_addr(pmpaddr, ARRAY_SIZE(pmpaddr));

	for (unsigned int i = 0; i < CONFIG_PMP_SLOTS; i++) {
		uint8_t cfg_byte = cfg[i];

		if ((cfg_byte & PMP_A) != PMP_NAPOT) {
			continue;
		}

		unsigned long start, end;

		pmp_decode_region(cfg_byte, pmpaddr, i, &start, &end);

		/* Skip entries not entirely within our test region */
		if (start < TEST_REGION_ADDR || end >= TEST_REGION_END) {
			continue;
		}

		if (count < max_entries) {
			starts[count] = start;
			ends[count] = end;
			perms[count] = cfg_byte;
			count++;
		}
	}

	return count;
}

ZTEST(riscv_pmp_napot_multi, test_region_split_into_multiple_napot_entries)
{
	unsigned long starts[CONFIG_PMP_SLOTS];
	unsigned long ends[CONFIG_PMP_SLOTS];
	uint8_t perms[CONFIG_PMP_SLOTS];
	unsigned int count;

	count = collect_napot_entries(starts, ends, perms, CONFIG_PMP_SLOTS);

	zassert_equal(count, EXPECTED_ENTRY_COUNT, "Expected %u NAPOT entries, found %u",
		      EXPECTED_ENTRY_COUNT, count);

	/* Reset expected entries */
	for (unsigned int i = 0; i < EXPECTED_ENTRY_COUNT; i++) {
		expected_entries[i].found = false;
	}

	/* Match each found entry against expected entries */
	for (unsigned int i = 0; i < count; i++) {
		bool matched = false;

		for (unsigned int j = 0; j < EXPECTED_ENTRY_COUNT; j++) {
			if (expected_entries[j].found) {
				continue;
			}

			if (starts[i] == expected_entries[j].start &&
			    ends[i] == expected_entries[j].end) {
				expected_entries[j].found = true;
				matched = true;

				/* Verify permissions (R/W/X bits only) */
				zassert_equal((perms[i] & 0x07), TEST_REGION_PERM,
					      "Permission mismatch: got 0x%02x, expected 0x%02x",
					      perms[i] & 0x07, TEST_REGION_PERM);
				break;
			}
		}

		zassert_true(matched, "Unexpected NAPOT entry: start=0x%lx end=0x%lx", starts[i],
			     ends[i]);
	}

	/* Verify all expected entries were found */
	for (unsigned int i = 0; i < EXPECTED_ENTRY_COUNT; i++) {
		zassert_true(expected_entries[i].found,
			     "Expected NAPOT entry not found: start=0x%lx end=0x%lx",
			     expected_entries[i].start, expected_entries[i].end);
	}
}

ZTEST(riscv_pmp_napot_multi, test_full_coverage_no_gaps)
{
	unsigned long starts[CONFIG_PMP_SLOTS];
	unsigned long ends[CONFIG_PMP_SLOTS];
	uint8_t perms[CONFIG_PMP_SLOTS];
	unsigned int count;

	count = collect_napot_entries(starts, ends, perms, CONFIG_PMP_SLOTS);

	zassert_true(count > 1, "Expected multiple NAPOT entries, found %u", count);

	/* Verify first entry starts at region start */
	unsigned long min_start = starts[0];
	unsigned long max_end = ends[0];

	for (unsigned int i = 1; i < count; i++) {
		if (starts[i] < min_start) {
			min_start = starts[i];
		}
		if (ends[i] > max_end) {
			max_end = ends[i];
		}
	}

	zassert_equal(min_start, TEST_REGION_ADDR,
		      "Coverage start mismatch: got 0x%lx, expected 0x%lx", min_start,
		      TEST_REGION_ADDR);

	zassert_equal(max_end, TEST_REGION_END - 1,
		      "Coverage end mismatch: got 0x%lx, expected 0x%lx", max_end,
		      TEST_REGION_END - 1);
}

ZTEST(riscv_pmp_napot_multi, test_config_options)
{
	zassert_true(IS_ENABLED(CONFIG_PMP_NAPOT_USE_MULTI_SLOTS),
		     "CONFIG_PMP_NAPOT_USE_MULTI_SLOTS should be enabled");
	zassert_false(IS_ENABLED(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT),
		      "CONFIG_PMP_POWER_OF_TWO_ALIGNMENT should be disabled");
	zassert_true(IS_ENABLED(CONFIG_PMP_NO_TOR), "CONFIG_PMP_NO_TOR should be enabled");
}

ZTEST_SUITE(riscv_pmp_napot_multi, NULL, NULL, NULL, NULL, NULL);
