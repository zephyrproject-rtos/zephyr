/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>

#include <zephyr/drivers/edac.h>
#include <zephyr/sys/barrier.h>

#if defined(CONFIG_CACHE_MANAGEMENT)
#include <zephyr/cache.h>
#endif

static inline void mecc_cache_clean_invd(uintptr_t addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT)
	(void)sys_cache_data_flush_and_invd_range((void *)addr, size);
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
#endif
}

#define MECC_COMPAT nxp_mecc

/* Use the MECC-protected memory range from devicetree to pick a safe test address. */
#define MECC_NODE       DT_INST(0, MECC_COMPAT)
#define MECC_START_ADDR DT_PROP(MECC_NODE, memory_start_address)
#define MECC_END_ADDR   DT_PROP(MECC_NODE, memory_end_address)

#define MECC_TEST_OFFSET 0x40
#define MECC_TEST_ADDR   (MECC_START_ADDR + MECC_TEST_OFFSET)

static const struct device *mecc_dev = DEVICE_DT_GET(DT_INST(0, MECC_COMPAT));

static volatile int cb_count;

static void mecc_notify_cb(const struct device *dev, void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	cb_count++;
}

static void wait_for_cb_count(int expected, k_timeout_t timeout)
{
	int64_t end = k_uptime_get() + k_ticks_to_ms_floor64(timeout.ticks);

	while (k_uptime_get() < end) {
		if (cb_count >= expected) {
			return;
		}
		k_sleep(K_MSEC(5));
	}

	TC_PRINT("Timeout waiting for MECC callback: expected %d, got %d\n", expected, cb_count);
}

ZTEST(edac_mecc, test_driver_ready)
{
	zassert_true(device_is_ready(mecc_dev), "MECC EDAC device is not ready");
}

ZTEST(edac_mecc, test_mandatory_api)
{
	uint64_t value;
	int ret;

	zassert_true(device_is_ready(mecc_dev), "MECC EDAC device is not ready");

	ret = edac_ecc_error_log_get(mecc_dev, &value);
	zassert_equal(ret, 0, "edac_ecc_error_log_get failed (%d)", ret);

	ret = edac_ecc_error_log_clear(mecc_dev);
	zassert_equal(ret, 0, "edac_ecc_error_log_clear failed (%d)", ret);

	/* MECC does not implement parity log, wrappers must return -ENOSYS */
	ret = edac_parity_error_log_get(mecc_dev, &value);
	zassert_equal(ret, -ENOSYS, "edac_parity_error_log_get expected -ENOSYS (%d)", ret);

	ret = edac_parity_error_log_clear(mecc_dev);
	zassert_equal(ret, -ENOSYS, "edac_parity_error_log_clear expected -ENOSYS (%d)", ret);

	ret = edac_errors_cor_get(mecc_dev);
	zassert_true(ret >= 0, "edac_errors_cor_get failed (%d)", ret);

	ret = edac_errors_uc_get(mecc_dev);
	zassert_true(ret >= 0, "edac_errors_uc_get failed (%d)", ret);

	ret = edac_notify_callback_set(mecc_dev, mecc_notify_cb);
	zassert_equal(ret, 0, "edac_notify_callback_set failed (%d)", ret);
}

ZTEST(edac_mecc, test_inject_correctable_error)
{
	volatile uint64_t *test_word = (volatile uint64_t *)(uintptr_t)MECC_TEST_ADDR;
	const uint64_t expected = 0x1122334455667788ULL;
	int ret;
	int cor_before, uc_before;
	uint64_t tmp;

	Z_TEST_SKIP_IFNDEF(CONFIG_EDAC_ERROR_INJECT);

	zassert_true(device_is_ready(mecc_dev), "MECC EDAC device is not ready");
	zassert_true(MECC_TEST_ADDR >= MECC_START_ADDR && MECC_TEST_ADDR + 8 <= MECC_END_ADDR,
		     "MECC test address out of range");

	/* Prepare a word in MECC protected RAM. */
	*test_word = expected;

	ret = edac_inject_set_param1(mecc_dev, (uint64_t)MECC_TEST_ADDR);
	zassert_equal(ret, 0, "edac_inject_set_param1 failed (%d)", ret);

	ret = edac_inject_set_param2(mecc_dev, 2);
	zassert_equal(ret, 0, "edac_inject_set_param2 failed (%d)", ret);

	ret = edac_inject_set_error_type(mecc_dev, EDAC_ERROR_TYPE_DRAM_COR);
	zassert_equal(ret, 0, "edac_inject_set_error_type failed (%d)", ret);

	ret = edac_notify_callback_set(mecc_dev, mecc_notify_cb);
	zassert_equal(ret, 0, "edac_notify_callback_set failed (%d)", ret);

	cb_count = 0;
	cor_before = edac_errors_cor_get(mecc_dev);
	uc_before = edac_errors_uc_get(mecc_dev);

	ret = edac_inject_error_trigger(mecc_dev);
	zassert_equal(ret, 0, "edac_inject_error_trigger failed (%d)", ret);

	mecc_cache_clean_invd(MECC_TEST_ADDR, 32);
	*test_word = expected;
	barrier_dsync_fence_full();
	mecc_cache_clean_invd(MECC_TEST_ADDR, 32);
	tmp = *test_word;
	ARG_UNUSED(tmp);

	/* Allow ISR to run */
	wait_for_cb_count(1, K_MSEC(500));

	zassert_equal(cb_count, 1, "Expected exactly one MECC notify callback (%d)", cb_count);

	zassert_equal(edac_errors_cor_get(mecc_dev), cor_before + 1,
		      "Correctable error count did not increment");
	zassert_equal(edac_errors_uc_get(mecc_dev), uc_before,
		      "Uncorrectable error count changed unexpectedly");

	/* Driver should restore the original value saved at trigger time. */
	zassert_equal(*test_word, expected, "test_word not restored after injection");
}

ZTEST(edac_mecc, test_inject_uncorrectable_error)
{
	volatile uint64_t *test_word = (volatile uint64_t *)(uintptr_t)(MECC_TEST_ADDR + 0x8);
	const uint64_t expected = 0xaabbccddeeff0011ULL;
	int ret;
	int cor_before, uc_before;
	uint64_t tmp;

	Z_TEST_SKIP_IFNDEF(CONFIG_EDAC_ERROR_INJECT);

	zassert_true(device_is_ready(mecc_dev), "MECC EDAC device is not ready");
	zassert_true((MECC_TEST_ADDR + 0x8) >=
			MECC_START_ADDR && (MECC_TEST_ADDR + 0x8) + 8 <= MECC_END_ADDR,
		     "MECC test address out of range");

	/* Prepare a word in MECC protected RAM. */
	*test_word = expected;

	ret = edac_inject_set_param1(mecc_dev, (uint64_t)(MECC_TEST_ADDR + 0x8));
	zassert_equal(ret, 0, "edac_inject_set_param1 failed (%d)", ret);

	/* Choose a different bit position from the COR test. */
	ret = edac_inject_set_param2(mecc_dev, 3);
	zassert_equal(ret, 0, "edac_inject_set_param2 failed (%d)", ret);

	ret = edac_inject_set_error_type(mecc_dev, EDAC_ERROR_TYPE_DRAM_UC);
	zassert_equal(ret, 0, "edac_inject_set_error_type failed (%d)", ret);

	ret = edac_notify_callback_set(mecc_dev, mecc_notify_cb);
	zassert_equal(ret, 0, "edac_notify_callback_set failed (%d)", ret);

	cb_count = 0;
	cor_before = edac_errors_cor_get(mecc_dev);
	uc_before = edac_errors_uc_get(mecc_dev);

	ret = edac_inject_error_trigger(mecc_dev);
	zassert_equal(ret, 0, "edac_inject_error_trigger failed (%d)", ret);

	mecc_cache_clean_invd(MECC_TEST_ADDR + 0x8, 32);
	*test_word = expected;
	barrier_dsync_fence_full();
	mecc_cache_clean_invd(MECC_TEST_ADDR + 0x8, 32);
	tmp = *test_word;
	ARG_UNUSED(tmp);

	/* Allow ISR to run */
	wait_for_cb_count(1, K_MSEC(500));

	zassert_equal(cb_count, 1, "Expected exactly one MECC notify callback (%d)", cb_count);

	zassert_equal(edac_errors_cor_get(mecc_dev), cor_before,
		      "Correctable error count changed unexpectedly");
	zassert_equal(edac_errors_uc_get(mecc_dev), uc_before + 1,
		      "Uncorrectable error count did not increment");

	/* Driver should restore the original value saved at trigger time. */
	zassert_equal(*test_word, expected, "test_word not restored after injection");
}

ZTEST_SUITE(edac_mecc, NULL, NULL, NULL, NULL, NULL);
