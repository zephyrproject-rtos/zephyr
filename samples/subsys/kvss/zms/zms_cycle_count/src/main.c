/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ZMS cycle count verification sample.
 * Verifies that zms_get_num_cycles() correctly tracks sector recycle counts
 * across multiple garbage collection cycles, including past the uint8_t
 * wraparound boundary at 256.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kvss/zms.h>

#define ZMS_PARTITION        storage_partition
#define ZMS_PARTITION_DEVICE PARTITION_DEVICE(ZMS_PARTITION)
#define ZMS_PARTITION_OFFSET PARTITION_OFFSET(ZMS_PARTITION)

#define SECTOR_COUNT 3

/* Phase 1: basic test with 30 iterations (expect num_cycles = 11) */
#define PHASE1_ITERATIONS 30

/* Phase 2: push past uint8_t wraparound (256).
 * After phase 1 we have accumulated PHASE1_ITERATIONS / SECTOR_COUNT cycles
 * above the base. We need 256 more cycles to exceed the uint8_t range.
 * PHASE2_REL_TARGET is the total relative cycles needed (above base).
 */
#define PHASE2_REL_TARGET    257
#define PHASE2_ITERATIONS    ((PHASE2_REL_TARGET - (PHASE1_ITERATIONS / SECTOR_COUNT)) \
			      * SECTOR_COUNT)

static struct zms_fs fs;

int main(void)
{
	int rc;
	struct flash_pages_info info;
	uint32_t num_cycles;
	uint32_t expected;
	uint32_t base_cycles;
	int total_advances = 0;

	fs.flash_device = ZMS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device not ready\n");
		return 0;
	}

	fs.offset = ZMS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		printk("Unable to get page info: %d\n", rc);
		return 0;
	}
	fs.sector_size = info.size;
	fs.sector_count = SECTOR_COUNT;

	/* Mount and clear to start from a known state */
	rc = zms_mount(&fs);
	if (rc) {
		printk("FAIL: initial mount failed: %d\n", rc);
		return 0;
	}
	rc = zms_clear(&fs);
	if (rc) {
		printk("FAIL: clear failed: %d\n", rc);
		return 0;
	}

	/* Re-mount after clear */
	rc = zms_mount(&fs);
	if (rc) {
		printk("FAIL: re-mount failed: %d\n", rc);
		return 0;
	}

	rc = zms_get_num_cycles(&fs, &num_cycles);
	if (rc) {
		printk("FAIL: zms_get_num_cycles failed: %d\n", rc);
		return 0;
	}
	base_cycles = num_cycles;
	printk("After mount: num_cycles=%u (base=%u)\n", num_cycles, base_cycles);

	/* === Phase 1: basic cycle count test === */
	printk("--- Phase 1: %d sector advances ---\n", PHASE1_ITERATIONS);
	for (int i = 1; i <= PHASE1_ITERATIONS; i++) {
		rc = zms_sector_use_next(&fs);
		if (rc) {
			printk("FAIL: sector_use_next returned %d at iteration %d\n", rc, i);
			return 0;
		}
		total_advances++;
		rc = zms_get_num_cycles(&fs, &num_cycles);
		if (rc) {
			printk("FAIL: zms_get_num_cycles failed: %d\n", rc);
			return 0;
		}
		printk("After sector_use_next[%d]: num_cycles=%u\n", i, num_cycles);
	}

	expected = base_cycles + (PHASE1_ITERATIONS / SECTOR_COUNT);
	printk("Phase 1 expected=%u, got=%u\n", expected, num_cycles);
	if (num_cycles != expected) {
		printk("FAIL: phase 1 cycle count mismatch\n");
		return 0;
	}

	/* === Phase 2: push past uint8_t boundary (256) === */
	printk("--- Phase 2: %d sector advances to exceed 256 cycles above base ---\n",
	       PHASE2_ITERATIONS);
	for (int i = 1; i <= PHASE2_ITERATIONS; i++) {
		rc = zms_sector_use_next(&fs);
		if (rc) {
			printk("FAIL: sector_use_next returned %d at phase2 iteration %d\n",
			       rc, i);
			return 0;
		}
		total_advances++;
		/* Print progress every SECTOR_COUNT advances (each full cycle) */
		if ((i % SECTOR_COUNT) == 0) {
			rc = zms_get_num_cycles(&fs, &num_cycles);
			if (rc) {
				printk("FAIL: zms_get_num_cycles failed: %d\n", rc);
				return 0;
			}
			printk("Phase 2 progress [%d/%d]: num_cycles=%u\n",
			       i, PHASE2_ITERATIONS, num_cycles);
		}
	}

	rc = zms_get_num_cycles(&fs, &num_cycles);
	if (rc) {
		printk("FAIL: zms_get_num_cycles failed: %d\n", rc);
		return 0;
	}
	expected = base_cycles + (total_advances / SECTOR_COUNT);
	printk("Final expected=%u, got=%u (total_advances=%d)\n",
	       expected, num_cycles, total_advances);

	if (num_cycles != expected) {
		printk("FAIL: full_cycle_cnt mismatch after wraparound\n");
		return 0;
	}

	if (num_cycles - base_cycles < 256) {
		printk("FAIL: num_cycles=%u did not exceed uint8_t range above base=%u\n",
		       num_cycles, base_cycles);
		return 0;
	}

	/* === Phase 3: verify per-sector cycle counts === */
	printk("--- Phase 3: per-sector cycle count verification ---\n");
	for (uint32_t s = 0; s < SECTOR_COUNT; s++) {
		uint32_t sector_cycles;

		rc = zms_get_sector_num_cycles(&fs, s, &sector_cycles);
		if (rc) {
			printk("FAIL: zms_get_sector_num_cycles(sector=%u) returned %d\n", s, rc);
			return 0;
		}
		printk("Sector %u: cycles=%u\n", s, sector_cycles);
		if (sector_cycles == 0) {
			printk("FAIL: sector %u has 0 cycles\n", s);
			return 0;
		}
	}

	/* Verify invalid sector index returns -EINVAL */
	uint32_t dummy;

	rc = zms_get_sector_num_cycles(&fs, SECTOR_COUNT, &dummy);
	if (rc != -EINVAL) {
		printk("FAIL: out-of-range sector returned %d instead of -EINVAL\n", rc);
		return 0;
	}

	printk("PASS: cycle count verification succeeded (num_cycles=%u > 255)\n", num_cycles);

	return 0;
}
