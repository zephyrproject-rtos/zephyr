/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/ztest.h>

#define BLOCK_SIZE 4096

static const struct device *const i2s = DEVICE_DT_GET(DT_ALIAS(i2s_tx));
K_MEM_SLAB_DEFINE_STATIC(tx_slab, BLOCK_SIZE, 4, 4);
K_MEM_SLAB_DEFINE_STATIC(rx_slab, BLOCK_SIZE, 4, 4);
static bool external_ref;

static void configure(enum i2s_dir dir)
{
	struct i2s_config config = {
		.word_size = 16,
		.channels = 2,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER,
		.frame_clk_freq = 8000,
		.mem_slab = dir == I2S_DIR_TX ? &tx_slab : &rx_slab,
		.block_size = BLOCK_SIZE,
		.timeout = 0,
	};

	zassert_ok(i2s_configure(i2s, dir, &config));
}

static void start_tx(void)
{
	void *block;
	int ret;

	zassert_ok(k_mem_slab_alloc(&tx_slab, &block, K_NO_WAIT));
	memset(block, 0, BLOCK_SIZE);
	ret = i2s_write(i2s, block, BLOCK_SIZE);
	if (ret < 0) {
		k_mem_slab_free(&tx_slab, block);
	}
	zassert_ok(ret);
	zassert_ok(i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_START));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_true(device_is_ready(i2s));
	zassert_true(pm_device_runtime_is_enabled(i2s));
	zassert_equal(pm_device_runtime_usage(i2s), 0);
	zassert_ok(pm_device_runtime_get(i2s));
	external_ref = true;
	configure(I2S_DIR_RX);
	configure(I2S_DIR_TX);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_DROP);
	(void)i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (external_ref) {
		external_ref = false;
		(void)pm_device_runtime_put_async(i2s, K_NO_WAIT);
	}
	zexpect_equal(pm_device_runtime_usage(i2s), 0);
}

ZTEST(i2s_pm, test_ready_drop_preserves_external_reference)
{
	zassert_ok(i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_DROP));
	zassert_equal(pm_device_runtime_usage(i2s), 1);
	zassert_ok(i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP));
	zassert_equal(pm_device_runtime_usage(i2s), 1);
}

ZTEST(i2s_pm, test_repeated_drop)
{
	start_tx();
	zassert_ok(i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP));
	zassert_equal(pm_device_runtime_usage(i2s), 1);
	zassert_ok(i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP));
	zassert_equal(pm_device_runtime_usage(i2s), 1);
}

ZTEST(i2s_pm, test_idle_rx_drop_preserves_tx_reference)
{
	unsigned int key;
	int before_drop;
	int after_drop;
	int ret;

	start_tx();
	key = irq_lock();
	before_drop = pm_device_runtime_usage(i2s);
	ret = i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_DROP);
	after_drop = pm_device_runtime_usage(i2s);
	irq_unlock(key);
	zassume_equal(before_drop, 2, "TX completed before the reference was sampled");
	zassert_ok(ret);
	zassert_equal(after_drop, 2);
}

ZTEST(i2s_pm, test_idle_tx_drop_preserves_rx_reference)
{
	unsigned int key;
	int before_drop;
	int after_drop;
	int ret;

	zassert_ok(i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_START));
	key = irq_lock();
	before_drop = pm_device_runtime_usage(i2s);
	ret = i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP);
	after_drop = pm_device_runtime_usage(i2s);
	irq_unlock(key);
	zassume_equal(before_drop, 2, "RX completed before the reference was sampled");
	zassert_ok(ret);
	zassert_equal(after_drop, 2);
}

ZTEST(i2s_pm, test_drop_after_completion)
{
	int64_t deadline = k_uptime_get() + 2000;

	start_tx();
	while (pm_device_runtime_usage(i2s) != 1 && k_uptime_get() < deadline) {
		k_msleep(1);
	}
	zassert_equal(pm_device_runtime_usage(i2s), 1);
	zassert_ok(i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP));
	zassert_equal(pm_device_runtime_usage(i2s), 1);
}

ZTEST(i2s_pm, test_drop_after_failed_start)
{
	zassert_not_equal(i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_START), 0);
	zassert_equal(pm_device_runtime_usage(i2s), 1);
	zassert_ok(i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP));
	zassert_equal(pm_device_runtime_usage(i2s), 1);
}

ZTEST_SUITE(i2s_pm, NULL, NULL, before, after, NULL);
