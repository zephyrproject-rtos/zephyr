/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/pm/pm.h>
#if defined(CONFIG_SOC_ESP32_PM_SLEEP_STATS)
#include <pmstats.h>
#endif
#include "i2s_api_test.h"

static atomic_t standby_entries;

static void pm_state_entry(enum pm_state state)
{
	if (state == PM_STATE_STANDBY) {
		atomic_inc(&standby_entries);
	}
}

static struct pm_notifier pm_light_sleep_notifier = {
	.state_entry = pm_state_entry,
};

/* Stay idle and check that the chip really entered light sleep. */
static void check_light_sleep(void)
{
#if defined(CONFIG_SOC_ESP32_PM_SLEEP_STATS)
	struct esp32_sleep_window win;
	uint32_t seq = esp32_sleep_stats_get(NULL);
#endif

	atomic_clear(&standby_entries);
	k_sleep(K_MSEC(100));
	zassert_true(atomic_get(&standby_entries) > 0,
		     "System stayed active during the idle window");

#if defined(CONFIG_SOC_ESP32_PM_SLEEP_STATS)
	zassert_not_equal(esp32_sleep_stats_get(&win), seq,
			  "No light sleep window completed");
	zassert_true(win.slept > 0, "Light sleep was entered but nothing actually slept");
	zassert_equal(win.err, 0, "Light sleep failed with HAL error 0x%x",
		      (unsigned int)win.err);
#endif
}

static int run_i2s_loopback_once(void)
{
	int ret;

	ret = tx_block_write(dev_i2s, 0, 0);
	if (ret != TC_PASS) {
		return ret;
	}
	ret = tx_block_write(dev_i2s, 1, 0);
	if (ret != TC_PASS) {
		return ret;
	}

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START failed");

	ret = rx_block_read(dev_i2s, 0);
	if (ret != TC_PASS) {
		return ret;
	}

	ret = tx_block_write(dev_i2s, 2, 0);
	if (ret != TC_PASS) {
		return ret;
	}

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN failed");

	ret = rx_block_read(dev_i2s, 1);
	if (ret != TC_PASS) {
		return ret;
	}

	return rx_block_read(dev_i2s, 2);
}

static void *pm_setup(void)
{
	k_thread_access_grant(k_current_get(), &rx_mem_slab, &tx_mem_slab);
	k_object_access_grant(dev_i2s_rx, k_current_get());
	k_object_access_grant(dev_i2s_tx, k_current_get());

	return NULL;
}

static void pm_before(void *fixture)
{
	ARG_UNUSED(fixture);

	int ret;

	zassert_not_null(dev_i2s, "TX/RX device not found");
	zassert_true(device_is_ready(dev_i2s), "device %s is not ready",
		     dev_i2s->name);

	i2s_test_recover(dev_i2s);

	ret = configure_stream(dev_i2s, I2S_DIR_BOTH);
	zassert_equal(ret, TC_PASS);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	dir_both_supported = (ret == 0);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		zassert_true(dir_both_supported,
			     "I2S_DIR_BOTH value is supposed to be supported.");
	}
}

/*
 * Configure once, then check that transfers still work after each light
 * sleep without configuring the stream again.
 */
ZTEST(i2s_pm, test_i2s_pm_light_sleep)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	pm_notifier_register(&pm_light_sleep_notifier);

	for (int cycle = 0; cycle < 2; cycle++) {
		check_light_sleep();
		zassert_equal(run_i2s_loopback_once(), TC_PASS);
	}

	pm_notifier_unregister(&pm_light_sleep_notifier);
}

ZTEST_SUITE(i2s_pm, NULL, pm_setup, pm_before, NULL, NULL);
