/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#if defined(CONFIG_SMP)
#include <zephyr/arch/cpu.h>
#endif

#define SAMPLE_NO 32
#define TIMEOUT 2000
#define FRAME_CLK_FREQ 8000
#define BLOCK_SIZE (2 * sizeof(int16_t) * SAMPLE_NO)
#define NUM_RX_BLOCKS 4
#define NUM_TX_BLOCKS 4
#define I2S_PM_MIN_IDLE_MS 100
#define I2S_PM_IDLE_MARGIN_MS 10

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)

#ifdef CONFIG_DCACHE_LINE_SIZE
#define SLAB_ALIGN MAX(32, CONFIG_DCACHE_LINE_SIZE)
#else
#define SLAB_ALIGN 32
#endif

K_MEM_SLAB_DEFINE_STATIC(rx_mem_slab, BLOCK_SIZE, NUM_RX_BLOCKS, SLAB_ALIGN);
K_MEM_SLAB_DEFINE_STATIC(tx_mem_slab, BLOCK_SIZE, NUM_TX_BLOCKS, SLAB_ALIGN);

/* The data_l represent a sine wave */
static const int16_t data_l[SAMPLE_NO] = {
	6392,  12539,  18204,  23169,  27244,  30272,  32137,  32767,  32137, 30272,  27244,
	23169, 18204,  12539,  6392,   0,     -6393,  -12540, -18205, -23170, -27245, -30273,
	-32138, -32767, -32138, -30273, -27245, -23170, -18205, -12540, -6393,  -1,
};

/* The data_r represent a sine wave with double the frequency of data_l */
static const int16_t data_r[SAMPLE_NO] = {
	12539,  23169,  30272,  32767,  30272,  23169,  12539,  0,     -12540, -23170, -30273,
	-32767, -30273, -23170, -12540, -1,    12539,  23169,  30272,  32767,  30272,  23169,
	12539,  0,      -12540, -23170, -30273, -32767, -30273, -23170, -12540, -1,
};

static atomic_t i2s_pm_entries[PM_STATE_COUNT];
static bool dir_both_supported;

static void count_state_entry(enum pm_state state)
{
	if (state < PM_STATE_COUNT) {
		atomic_inc(&i2s_pm_entries[state]);
	}
}

static struct pm_notifier i2s_pm_notifier = {
	.state_entry = count_state_entry,
};

static void reset_state_counts(void)
{
	for (int state = 0; state < PM_STATE_COUNT; state++) {
		atomic_clear(&i2s_pm_entries[state]);
	}
}

static const char *get_state_name(enum pm_state state)
{
	switch (state) {
	case PM_STATE_ACTIVE:
		return "PM_STATE_ACTIVE";
	case PM_STATE_RUNTIME_IDLE:
		return "PM_STATE_RUNTIME_IDLE";
	case PM_STATE_SUSPEND_TO_IDLE:
		return "PM_STATE_SUSPEND_TO_IDLE";
	case PM_STATE_STANDBY:
		return "PM_STATE_STANDBY";
	case PM_STATE_SUSPEND_TO_RAM:
		return "PM_STATE_SUSPEND_TO_RAM";
	case PM_STATE_SUSPEND_TO_DISK:
		return "PM_STATE_SUSPEND_TO_DISK";
	case PM_STATE_SOFT_OFF:
		return "PM_STATE_SOFT_OFF";
	default:
		return "unknown";
	}
}

static bool report_state_counts(void)
{
	bool entered = false;

	for (int state = 0; state < PM_STATE_COUNT; state++) {
		atomic_val_t count = atomic_get(&i2s_pm_entries[state]);

		if (count > 0) {
			printk("entered %s, count %ld\n", get_state_name((enum pm_state)state),
			       (long)count);
			entered = true;
		}
	}

	return entered;
}

static uint8_t get_cpu_id(void)
{
#if defined(CONFIG_SMP)
	return arch_curr_cpu()->id;
#else
	return 0U;
#endif
}

static uint32_t get_sleep_time_ms(const struct pm_state_info *state)
{
	uint64_t need_us = (uint64_t)state->min_residency_us + state->exit_latency_us;
	uint32_t need_ms = (uint32_t)DIV_ROUND_UP(need_us, 1000U);

	if (need_ms < I2S_PM_MIN_IDLE_MS) {
		return I2S_PM_MIN_IDLE_MS;
	}

	return need_ms + I2S_PM_IDLE_MARGIN_MS;
}

static void lock_other_states(const struct pm_state_info *states, uint8_t count,
			      const struct pm_state_info *keep, bool lock)
{
	for (uint8_t i = 0; i < count; i++) {
		if (&states[i] == keep) {
			continue;
		}

		if (lock) {
			pm_policy_state_lock_get(states[i].state, states[i].substate_id);
		} else {
			pm_policy_state_lock_put(states[i].state, states[i].substate_id);
		}
	}
}

static void fill_buf(int16_t *tx_block, int att)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = data_l[i] >> att;
		tx_block[2 * i + 1] = data_r[i] >> att;
	}
}

static int verify_buf(int16_t *rx_block, int att)
{
	int sample_no = SAMPLE_NO;

#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
	static int offset = -1;

	if (offset < 0) {
		do {
			++offset;
			if (offset > CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET) {
				TC_PRINT("Allowed data offset exceeded\n");
				return -TC_FAIL;
			}
		} while (rx_block[2 * offset] != data_l[0] >> att);

		TC_PRINT("Using data offset: %d\n", offset);
	}

	rx_block += 2 * offset;
	sample_no -= offset;
#endif

	for (int i = 0; i < sample_no; i++) {
		if (rx_block[2 * i] != data_l[i] >> att) {
			TC_PRINT("Error: att %d: data_l mismatch at position %d, expected %d, "
				 "actual %d\n",
				 att, i, data_l[i] >> att, rx_block[2 * i]);
			return -TC_FAIL;
		}
		if (rx_block[2 * i + 1] != data_r[i] >> att) {
			TC_PRINT("Error: att %d: data_r mismatch at position %d, expected %d, "
				 "actual %d\n",
				 att, i, data_r[i] >> att, rx_block[2 * i + 1]);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

static int tx_block_write(const struct device *dev, int att)
{
	char tx_block[BLOCK_SIZE];
	int ret;

	fill_buf((int16_t *)tx_block, att);
	ret = i2s_buf_write(dev, tx_block, BLOCK_SIZE);
	if (ret != 0) {
		TC_PRINT("Error: i2s_write failed, actual %d\n", ret);
		return -TC_FAIL;
	}

	return TC_PASS;
}

static int rx_block_read(const struct device *dev, int att)
{
	char rx_block[BLOCK_SIZE];
	size_t rx_size;
	int ret;

	ret = i2s_buf_read(dev, rx_block, &rx_size);
	if ((ret < 0) || (rx_size != BLOCK_SIZE)) {
		TC_PRINT("Error: Read failed\n");
		return -TC_FAIL;
	}

	ret = verify_buf((int16_t *)rx_block, att);
	if (ret < 0) {
		TC_PRINT("Error: Verify failed\n");
		return -TC_FAIL;
	}

	return TC_PASS;
}

static int configure_stream(const struct device *dev)
{
	struct i2s_config i2s_cfg = {0};
	int ret;

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;

	if (!IS_ENABLED(CONFIG_I2S_TEST_USE_GPIO_LOOPBACK)) {
		i2s_cfg.options |= I2S_OPT_LOOPBACK;
	}

	i2s_cfg.mem_slab = &tx_mem_slab;
	ret = i2s_configure(dev, I2S_DIR_TX, &i2s_cfg);
	if (ret < 0) {
		TC_PRINT("Failed to configure I2S TX stream (%d)\n", ret);
		return -TC_FAIL;
	}

	i2s_cfg.mem_slab = &rx_mem_slab;
	ret = i2s_configure(dev, I2S_DIR_RX, &i2s_cfg);
	if (ret < 0) {
		TC_PRINT("Failed to configure I2S RX stream (%d)\n", ret);
		return -TC_FAIL;
	}

	return TC_PASS;
}

static int run_i2s_loopback_once(const struct device *dev)
{
	int ret;

	ret = i2s_trigger(dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	if (ret != 0) {
		TC_PRINT("RX/TX DROP failed (%d)\n", ret);
		return -TC_FAIL;
	}

	ret = tx_block_write(dev, 0);
	if (ret != TC_PASS) {
		return ret;
	}
	ret = tx_block_write(dev, 1);
	if (ret != TC_PASS) {
		return ret;
	}

	ret = i2s_trigger(dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
	if (ret != 0) {
		TC_PRINT("RX/TX START failed (%d)\n", ret);
		return -TC_FAIL;
	}

	ret = rx_block_read(dev, 0);
	if (ret != TC_PASS) {
		return ret;
	}

	ret = tx_block_write(dev, 2);
	if (ret != TC_PASS) {
		return ret;
	}

	ret = i2s_trigger(dev, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	if (ret != 0) {
		TC_PRINT("RX/TX DRAIN failed (%d)\n", ret);
		return -TC_FAIL;
	}

	ret = rx_block_read(dev, 1);
	if (ret != TC_PASS) {
		return ret;
	}

	return rx_block_read(dev, 2);
}

/* Configure once before the sweep. Each idle checks that the state was entered,
 * then a full-duplex transfer checks that the stream still works without a new
 * configure, so retention and the PM lock are both covered.
 */
static int check_loopback_after_idle(const struct device *dev, const struct pm_state_info *state,
				     uint32_t idle_ms)
{
	for (int cycle = 0; cycle < 2; cycle++) {
		int ret;

		reset_state_counts();
		k_msleep(idle_ms);
		(void)report_state_counts();
		if (atomic_get(&i2s_pm_entries[state->state]) <= 0) {
			TC_PRINT("power state %s was not entered\n", get_state_name(state->state));
			return -TC_FAIL;
		}

		ret = run_i2s_loopback_once(dev);
		if (ret != TC_PASS) {
			return ret;
		}
	}

	return TC_PASS;
}

static void run_in_each_state(const struct device *dev,
			      int (*fn)(const struct device *dev,
					const struct pm_state_info *state,
					uint32_t idle_ms))
{
	const struct pm_state_info *states;
	uint8_t count = pm_state_cpu_get_all(get_cpu_id(), &states);
	bool tested = false;

	for (uint8_t i = 0; i < count; i++) {
		const struct pm_state_info *state = &states[i];
		uint32_t sleep_ms;
		int ret;

		if (state->state == PM_STATE_SOFT_OFF) {
			continue;
		}

		sleep_ms = get_sleep_time_ms(state);
		tested = true;
		if (state->substate_id == 0U) {
			printk("testing %s for %u ms\n", get_state_name(state->state), sleep_ms);
		} else {
			printk("testing %s substate %u for %u ms\n", get_state_name(state->state),
			       state->substate_id, sleep_ms);
		}

		lock_other_states(states, count, state, true);
		ret = fn(dev, state, sleep_ms);
		lock_other_states(states, count, state, false);
		zassert_equal(ret, TC_PASS);
	}

	zassert_true(tested, "no idle power state is enabled");
}

static void *i2s_pm_setup(void)
{
	const struct device *dev = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);
	int ret;

	zassert_not_null(dev, "I2S device not found");
	zassert_true(device_is_ready(dev), "device %s is not ready", dev->name);

	ret = configure_stream(dev);
	zassert_equal(ret, TC_PASS);

	ret = i2s_trigger(dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	dir_both_supported = (ret == 0);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		zassert_true(dir_both_supported, "I2S_DIR_BOTH value is supposed to be supported.");
	}

	pm_notifier_register(&i2s_pm_notifier);

	return NULL;
}

static void i2s_pm_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)pm_notifier_unregister(&i2s_pm_notifier);
}

ZTEST(i2s_pm, test_i2s)
{
	const struct device *dev = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);

	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	run_in_each_state(dev, check_loopback_after_idle);
}

ZTEST_SUITE(i2s_pm, NULL, i2s_pm_setup, NULL, NULL, i2s_pm_teardown);
