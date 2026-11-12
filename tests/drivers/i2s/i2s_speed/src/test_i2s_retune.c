/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>

#define DEV_NODE_RX DT_ALIAS(i2s_node0)
#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
#define DEV_NODE_TX DT_ALIAS(i2s_node1)
#else
#define DEV_NODE_TX DEV_NODE_RX
#endif
#define BLOCK_SIZE (64 * 2 * sizeof(int16_t))
#define NUM_BLOCKS 8
#define TIMEOUT 2000

#if DT_NODE_HAS_COMPAT(DEV_NODE_RX, nxp_mcux_i2s) && \
	DT_NODE_HAS_COMPAT(DEV_NODE_TX, nxp_mcux_i2s) && !defined(CONFIG_I2S_HAS_PLL_SETTING)
#define RETUNE_TEST_SUPPORTED 1
#else
#define RETUNE_TEST_SUPPORTED 0
#endif

#ifdef CONFIG_NOCACHE_MEMORY
#define SLAB_ATTR __nocache
#else
#define SLAB_ATTR
#endif

static char SLAB_ATTR __aligned(WB_UP(32)) tx_buf[NUM_BLOCKS * WB_UP(BLOCK_SIZE)];
static char SLAB_ATTR __aligned(WB_UP(32)) rx_buf[(NUM_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, retune_tx_slab) =
	Z_MEM_SLAB_INITIALIZER(retune_tx_slab, tx_buf, WB_UP(BLOCK_SIZE), NUM_BLOCKS);
STRUCT_SECTION_ITERABLE(k_mem_slab, retune_rx_slab) =
	Z_MEM_SLAB_INITIALIZER(retune_rx_slab, rx_buf, WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

static const struct device *dev_rx;
static const struct device *dev_tx;

static const struct device *device_for_dir(enum i2s_dir dir)
{
	return dir == I2S_DIR_TX ? dev_tx : dev_rx;
}

static int configure(enum i2s_dir dir, uint32_t rate)
{
	struct i2s_config cfg = {
		.word_size = 16,
		.channels = 2,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.frame_clk_freq = rate,
		.block_size = BLOCK_SIZE,
		.timeout = TIMEOUT,
		.options = I2S_OPT_LOOPBACK,
		.mem_slab = dir == I2S_DIR_TX ? &retune_tx_slab : &retune_rx_slab,
	};

	if (dir == I2S_DIR_TX) {
		cfg.options |= I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
	} else {
		cfg.options |= I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
	}

	return i2s_configure(device_for_dir(dir), dir, &cfg);
}

static void reset_stream(enum i2s_dir dir)
{
	struct i2s_config cfg = {0};

	const struct device *dev = device_for_dir(dir);

	(void)i2s_trigger(dev, dir, I2S_TRIGGER_DROP);
	(void)i2s_configure(dev, dir, &cfg);
}

static void reset_all(void)
{
	reset_stream(I2S_DIR_TX);
	reset_stream(I2S_DIR_RX);
}

static void *setup(void)
{
	dev_rx = DEVICE_DT_GET_OR_NULL(DEV_NODE_RX);
	dev_tx = DEVICE_DT_GET_OR_NULL(DEV_NODE_TX);
	zassert_not_null(dev_rx);
	zassert_not_null(dev_tx);
	zassert_true(device_is_ready(dev_rx));
	zassert_true(device_is_ready(dev_tx));
	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	if (!RETUNE_TEST_SUPPORTED) {
		ztest_test_skip();
	}
	reset_all();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_all();
}

ZTEST(retune, test_idle_peer_is_invalidated_and_purged)
{
	void *block;
	uint32_t free_before;
	int ret;

	zassert_ok(configure(I2S_DIR_TX, 48000));
	free_before = k_mem_slab_num_free_get(&retune_tx_slab);

	for (int i = 0; i < 2; i++) {
		zassert_ok(k_mem_slab_alloc(&retune_tx_slab, &block, K_NO_WAIT));
		zassert_ok(i2s_write(dev_tx, block, BLOCK_SIZE));
	}

	ret = configure(I2S_DIR_RX, 44100);
	zassert_ok(ret, "idle peer retune failed: %d", ret);
	zassert_equal(k_mem_slab_num_free_get(&retune_tx_slab), free_before,
		      "invalidated peer buffers were not purged");
	zassert_equal(i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_START), -EIO);
}

ZTEST(retune, test_running_peer_retune_is_rejected)
{
	void *block;
	int ret;

	zassert_ok(configure(I2S_DIR_TX, 48000));
	zassert_ok(k_mem_slab_alloc(&retune_tx_slab, &block, K_NO_WAIT));
	zassert_ok(i2s_write(dev_tx, block, BLOCK_SIZE));
	zassert_ok(i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_START));

	ret = configure(I2S_DIR_RX, 44100);
	zassert_equal(ret, -EINVAL, "running peer retune returned %d", ret);
}

ZTEST_SUITE(retune, NULL, setup, before, after, NULL);
