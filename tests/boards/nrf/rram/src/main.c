/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#endif

#define TEST_AREA storage_partition

/* TEST_AREA is only defined for configurations that rely on
 * fixed-partition nodes.
 */
#if defined(TEST_AREA)
#define TEST_AREA_OFFSET PARTITION_OFFSET(TEST_AREA)
#define TEST_AREA_SIZE   PARTITION_SIZE(TEST_AREA)
#define TEST_AREA_DEVICE PARTITION_DEVICE(TEST_AREA)
#else
#error "Unsupported configuration"
#endif

#define BUF_SIZE        512
#define TEST_ITERATIONS 100
#if defined(CONFIG_SOC_FLASH_NRF_THROTTLING)
/* Expected delay in ms: In this expression CONFIG_NRF_RRAM_THROTTLING_DELAY
 * is expressed in microseconds and CONFIG_NRF_RRAM_THROTTLING_DATA_BLOCK is in
 * number of write lines of 16 bytes each.
 */
#define EXPECTED_DELAY                                                                             \
	((TEST_ITERATIONS * BUF_SIZE * CONFIG_NRF_RRAM_THROTTLING_DELAY) /                         \
	 (CONFIG_NRF_RRAM_THROTTLING_DATA_BLOCK * 16 * 1000))
#endif
#if !defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && !defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#error There is no flash device enabled or it is missing Kconfig options
#endif

#define WRITE_LINE_SIZE 16
#define ERASE_VALUE     0xFF

/* Larger than the production WRITE_BUFFER_MAX_SIZE on nrf54l15
 * (NRF_RRAM_WRITE_BUFFER_SIZE * 16 = 32 * 16 = 512 B), so that the
 * radio-sync write_op() must reschedule across at least two time slots
 * per iteration. With RADIO_SYNC_NONE the call still validates a single
 * large rram_write() with full write-buffer commit semantics.
 */
#define MULTISLOT_BUF_SIZE 1024
#define MULTISLOT_ITERS    20

#ifdef CONFIG_NRF_RRAM_FILL_SKIP_IF_FILLED
/* The skip-if-filled timing assertion needs enough work in the dirty
 * branch to leave a clear gap above the clean branch even at high
 * resolutions; 4 KiB still fits comfortably in TEST_AREA_SIZE
 * (0x9000 = 36 KiB) and exposes a measurable difference in every
 * driver configuration we test (THROTTLING, RADIO_SYNC, both, neither).
 */
#define SKIP_BUF_SIZE 4096
#endif

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static uint8_t __aligned(4) buf[BUF_SIZE];
static uint8_t __aligned(4) verify_buf[BUF_SIZE];
static uint8_t __aligned(4) multislot_buf[MULTISLOT_BUF_SIZE];
static uint8_t __aligned(4) multislot_verify[MULTISLOT_BUF_SIZE];
#ifdef CONFIG_NRF_RRAM_FILL_SKIP_IF_FILLED
static uint8_t __aligned(4) skip_buf[SKIP_BUF_SIZE];
static uint8_t __aligned(4) skip_verify_buf[SKIP_BUF_SIZE];
#endif

static void *rram_setup(void)
{
	zassert_true(device_is_ready(flash_dev));

#if defined(CONFIG_BT)
	/* Initialise BLE LL: this starts the ticker so
	 * nrf_flash_sync_is_required() returns true and every flash
	 * operation goes through the radio-sync write_op() path instead
	 * of rram_write() directly. Required to validate defensive
	 * RRAMC config_set in write_op() against active radio activity.
	 */
	int err = bt_enable(NULL);

	zassert_equal(err, 0, "bt_enable failed: %d", err);

	err = bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0, NULL, 0);
	zassert_equal(err, 0, "bt_le_adv_start failed: %d", err);

	TC_PRINT("BLE non-connectable advertising started; flash operations "
		 "will use the radio-sync (write_op) path\n");
#endif

	TC_PRINT("Test will run on device %s\n", flash_dev->name);
	TC_PRINT("TEST_AREA_OFFSET = 0x%lx\n", (unsigned long)TEST_AREA_OFFSET);
	TC_PRINT("TEST_AREA_SIZE   = 0x%lx\n", (unsigned long)TEST_AREA_SIZE);

	return NULL;
}

/* Leave TEST_AREA in a known (fully erased) state before every test so the
 * suite does not depend on declaration / execution order. ztest does not
 * guarantee that tests run in source order; without this hook tests that
 * pre-fill the region would silently rely on the previous test's residue.
 */
static void rram_before(void *fixture)
{
	ARG_UNUSED(fixture);

	int rc = flash_flatten(flash_dev, TEST_AREA_OFFSET, TEST_AREA_SIZE);

	zassert_equal(rc, 0, "%s: flash_flatten failed: %d", __func__, rc);
}

static void rram_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

#if defined(CONFIG_BT)
	/* Stop the advertising started in setup so the controller is left
	 * in a clean state when the test binary is reused (e.g. when the
	 * suite runs back-to-back in the same boot, or when another suite
	 * piggybacks on the same image). bt_disable() is intentionally not
	 * called - several controllers do not support a full bring-down /
	 * bring-up cycle from the host side and leaving the LL initialised
	 * matches what real-world apps do.
	 */
	int err = bt_le_adv_stop();

	if (err) {
		TC_PRINT("bt_le_adv_stop returned %d (already stopped?)\n", err);
	}
#endif
}

ZTEST(rram, test_flash_throttling)
{
#if !defined(CONFIG_SOC_FLASH_NRF_THROTTLING)
	ztest_test_skip();
#else
	int rc;
	uint32_t start = TEST_AREA_OFFSET;

	int64_t ts = k_uptime_get();

	for (int i = 0; i < TEST_ITERATIONS; i++) {
		rc = flash_write(flash_dev, start, buf, BUF_SIZE);
		zassert_equal(rc, 0, "Cannot write to flash");
	}
	/* Delay measured in milliseconds */
	int64_t delta = k_uptime_delta(&ts);

	zassert_true(delta > EXPECTED_DELAY, "Invalid delay, expected > %d, measured: %lld",
		     EXPECTED_DELAY, delta);
#endif
}

/* flash_flatten() on no-erase devices must dispatch through the
 * driver's api->fill callback (one bulk operation, one config_set,
 * one commit) instead of falling back to flash_fill()'s generic
 * write-loop emulation. Verify both that the resulting bytes are
 * erase_value AND that performance is at least as good as a write of
 * the same length (often much better since the fill path skips a
 * memcpy).
 */
ZTEST(rram, test_flatten_throughput)
{
	const struct flash_parameters *params = flash_get_parameters(flash_dev);
	const uint8_t erase_val = params->erase_value;
	const size_t flatten_size = MIN((size_t)BUF_SIZE, (size_t)TEST_AREA_SIZE);
	uint32_t start = TEST_AREA_OFFSET;
	int rc;

	memset(buf, 0xA5, sizeof(buf));
	rc = flash_write(flash_dev, start, buf, flatten_size);
	zassert_equal(rc, 0, "Pre-fill write failed: %d", rc);

	uint32_t cyc_flatten_start = k_cycle_get_32();

	rc = flash_flatten(flash_dev, start, flatten_size);

	uint32_t cyc_flatten = k_cycle_get_32() - cyc_flatten_start;

	zassert_equal(rc, 0, "flash_flatten failed: %d", rc);

	rc = flash_read(flash_dev, start, verify_buf, flatten_size);
	zassert_equal(rc, 0, "Read-back failed: %d", rc);
	for (size_t i = 0; i < flatten_size; i++) {
		zassert_equal(verify_buf[i], erase_val,
			      "Byte %zu after flatten == 0x%02x, expected 0x%02x",
			      i, verify_buf[i], erase_val);
	}

	uint32_t cyc_write_start = k_cycle_get_32();

	memset(buf, erase_val, flatten_size);
	rc = flash_write(flash_dev, start, buf, flatten_size);
	uint32_t cyc_write = k_cycle_get_32() - cyc_write_start;

	zassert_equal(rc, 0, "Reference write failed: %d", rc);

	uint64_t us_flatten = k_cyc_to_us_ceil64(cyc_flatten);
	uint64_t us_write = k_cyc_to_us_ceil64(cyc_write);

	TC_PRINT("flatten: %lld us, equivalent write: %lld us (size=%zu)\n",
		 us_flatten, us_write, flatten_size);

	/* flash_flatten should not be dramatically slower than the equivalent
	 * write. Allow a 2x safety margin to absorb cache / interrupt jitter
	 * but catch a regression to the generic flash_fill emulation path
	 * (loop of CONFIG_FLASH_FILL_BUFFER_SIZE-sized api->write() calls)
	 * which was typically >5x slower.
	 */
	if (!IS_ENABLED(CONFIG_BT)) {
		zassert_true(us_flatten <= 2 * us_write,
			     "flash_flatten regressed: %lld us vs write %lld us",
			     us_flatten, us_write);
	}
}

/* flash_flatten() on no-erase devices must accept any write-line
 * aligned range. The api->fill callback is constrained by
 * write_block_size (16 B), independently of the page-aligned
 * constraint that explicit erase still imposes, so a sub-page flatten
 * must succeed and must not corrupt unrelated lines on the same page.
 */
ZTEST(rram, test_flatten_subpage)
{
	const struct flash_parameters *params = flash_get_parameters(flash_dev);
	const uint8_t erase_val = params->erase_value;
	uint32_t start = TEST_AREA_OFFSET;
	const size_t pre_fill = MIN((size_t)BUF_SIZE, (size_t)TEST_AREA_SIZE);
	int rc;

	zassert_true(pre_fill >= 4 * WRITE_LINE_SIZE,
		     "Test partition too small for sub-page test");

	memset(buf, 0x5A, pre_fill);
	rc = flash_write(flash_dev, start, buf, pre_fill);
	zassert_equal(rc, 0, "Pre-fill write failed: %d", rc);

	rc = flash_flatten(flash_dev, start + WRITE_LINE_SIZE, WRITE_LINE_SIZE);
	zassert_equal(rc, 0, "flash_flatten of one write-line failed: %d", rc);

	rc = flash_read(flash_dev, start, verify_buf, pre_fill);
	zassert_equal(rc, 0, "Read-back failed: %d", rc);

	for (size_t i = 0; i < WRITE_LINE_SIZE; i++) {
		zassert_equal(verify_buf[i], 0x5A,
			      "Line 0 byte %zu = 0x%02x, expected 0x5A "
			      "(unrelated line corrupted by sub-page flatten)",
			      i, verify_buf[i]);
	}
	for (size_t i = WRITE_LINE_SIZE; i < 2 * WRITE_LINE_SIZE; i++) {
		zassert_equal(verify_buf[i], erase_val,
			      "Line 1 byte %zu = 0x%02x, expected 0x%02x "
			      "(target line not flattened)",
			      i, verify_buf[i], erase_val);
	}
	for (size_t i = 2 * WRITE_LINE_SIZE; i < pre_fill; i++) {
		zassert_equal(verify_buf[i], 0x5A,
			      "Line >=2 byte %zu = 0x%02x, expected 0x5A "
			      "(unrelated line corrupted by sub-page flatten)",
			      i, verify_buf[i]);
	}

	rc = flash_flatten(flash_dev, start + WRITE_LINE_SIZE / 2, WRITE_LINE_SIZE);
	zassert_not_equal(rc, 0,
			  "flash_flatten with unaligned offset must fail "
			  "(returned %d)", rc);
}

/* With CONFIG_NRF_RRAM_FILL_SKIP_IF_FILLED enabled, flatten of an
 * already-erased region must be measurably faster than flatten of a
 * fully-dirty region of the same size, because the optimisation skips
 * the physical write entirely.
 */
ZTEST(rram, test_skip_if_filled)
{
#ifndef CONFIG_NRF_RRAM_FILL_SKIP_IF_FILLED
	ztest_test_skip();
#else
	const struct flash_parameters *params = flash_get_parameters(flash_dev);
	const uint8_t erase_val = params->erase_value;
	const size_t flatten_size = MIN((size_t)SKIP_BUF_SIZE, (size_t)TEST_AREA_SIZE);
	uint32_t start = TEST_AREA_OFFSET;
	int rc;

	memset(skip_buf, 0xC3, flatten_size);
	rc = flash_write(flash_dev, start, skip_buf, flatten_size);
	zassert_equal(rc, 0, "Pre-fill write failed: %d", rc);

	uint32_t cyc_dirty_start = k_cycle_get_32();

	rc = flash_flatten(flash_dev, start, flatten_size);

	uint32_t cyc_dirty = k_cycle_get_32() - cyc_dirty_start;

	zassert_equal(rc, 0, "flash_flatten of dirty region failed: %d", rc);

	uint32_t cyc_clean_start = k_cycle_get_32();

	rc = flash_flatten(flash_dev, start, flatten_size);

	uint32_t cyc_clean = k_cycle_get_32() - cyc_clean_start;

	zassert_equal(rc, 0, "flash_flatten of clean region failed: %d", rc);

	rc = flash_read(flash_dev, start, skip_verify_buf, flatten_size);
	zassert_equal(rc, 0, "Read-back failed: %d", rc);
	for (size_t i = 0; i < flatten_size; i++) {
		zassert_equal(skip_verify_buf[i], erase_val,
			      "Byte %zu = 0x%02x, expected 0x%02x", i,
			      skip_verify_buf[i], erase_val);
	}

	uint64_t us_dirty = k_cyc_to_us_ceil64(cyc_dirty);
	uint64_t us_clean = k_cyc_to_us_ceil64(cyc_clean);

	TC_PRINT("flatten dirty: %llu us, flatten already-erased: %llu us "
		 "(size=%zu)\n",
		 us_dirty, us_clean, flatten_size);

	if (!IS_ENABLED(CONFIG_BT)) {
		zassert_true(us_clean * 4 < us_dirty,
			     "skip-if-filled optimisation regressed: "
			     "clean=%llu us is not at least 4x faster than dirty=%llu us "
			     "(size=%zu)",
			     us_clean, us_dirty, flatten_size);
	}
#endif
}

/* Stress the radio-sync path with writes that do not fit in a single
 * WRITE_BUFFER_MAX_SIZE slot. With NRF_RRAM_WRITE_BUFFER_SIZE = 32 (the
 * default selected by MCUboot for nrf54l15) every iteration of this loop
 * uses two consecutive radio time slots, exercising:
 *
 *   - write_op() reentry across slots (FLASH_OP_ONGOING -> next slot);
 *   - the defensive nrf_rramc_config_set(write=true) at the start of
 *     every slot, which makes write_op() self-contained instead of
 *     relying on the RRAMC config persisting across the radio gap;
 *   - end-to-end commit_changes() with sizes that are multiples of
 *     WRITE_BUFFER_MAX_SIZE (no explicit commit) and not (explicit
 *     commit), interleaved at iteration granularity.
 *
 * In RADIO_SYNC_NONE the same workload still validates large
 * single-call rram_write() and flatten paths, so the test is meaningful
 * for every variant of the suite.
 */
ZTEST(rram, test_multislot_stress)
{
	const struct flash_parameters *params = flash_get_parameters(flash_dev);
	const uint8_t erase_val = params->erase_value;
	const uint32_t start = TEST_AREA_OFFSET;
	int rc;

	if ((size_t)TEST_AREA_SIZE < MULTISLOT_BUF_SIZE) {
		ztest_test_skip();
	}

	int64_t ts = k_uptime_get();

	for (uint32_t i = 0; i < MULTISLOT_ITERS; i++) {
		const uint8_t pattern = (uint8_t)(0xA0 + (i & 0x1F));

		memset(multislot_buf, pattern, MULTISLOT_BUF_SIZE);

		rc = flash_write(flash_dev, start, multislot_buf, MULTISLOT_BUF_SIZE);
		zassert_equal(rc, 0, "iter %u: flash_write failed: %d", i, rc);

		rc = flash_read(flash_dev, start, multislot_verify, MULTISLOT_BUF_SIZE);
		zassert_equal(rc, 0, "iter %u: flash_read after write failed: %d", i, rc);
		zassert_mem_equal(multislot_buf, multislot_verify, MULTISLOT_BUF_SIZE,
				  "iter %u: data mismatch after write", i);

		rc = flash_flatten(flash_dev, start, MULTISLOT_BUF_SIZE);
		zassert_equal(rc, 0, "iter %u: flash_flatten failed: %d", i, rc);

		rc = flash_read(flash_dev, start, multislot_verify, MULTISLOT_BUF_SIZE);
		zassert_equal(rc, 0, "iter %u: flash_read after flatten failed: %d",
			      i, rc);
		for (size_t j = 0; j < MULTISLOT_BUF_SIZE; j++) {
			zassert_equal(multislot_verify[j], erase_val,
				      "iter %u byte %zu = 0x%02x, expected 0x%02x",
				      i, j, multislot_verify[j], erase_val);
		}
	}

	int64_t delta = k_uptime_delta(&ts);

	TC_PRINT("multislot stress: %u iters x (write+verify+flatten+verify) "
		 "of %d B in %lld ms (avg %lld ms/iter)\n",
		 MULTISLOT_ITERS, MULTISLOT_BUF_SIZE, delta,
		 delta / MULTISLOT_ITERS);
}

ZTEST_SUITE(rram, NULL, rram_setup, rram_before, NULL, rram_teardown);
