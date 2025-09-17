/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)

#define WORD_SIZE 16U
#define NUMBER_OF_CHANNELS 2
#define FRAME_CLK_FREQ 44100

#define NUM_BLOCKS 2
#define TIMEOUT 1000

#define SAMPLES_COUNT 4
/* Each word has one bit set */
static const int16_t data[SAMPLES_COUNT] = {16, 32, 64, 128};

#define BLOCK_SIZE (2 * sizeof(data))

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[NUM_BLOCKS * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

static const struct device *dev_i2s;

static const struct i2s_config default_i2s_cfg = {
	.word_size = WORD_SIZE,
	.channels = NUMBER_OF_CHANNELS,
	.format = I2S_FMT_DATA_FORMAT_I2S,
	.frame_clk_freq = FRAME_CLK_FREQ,
	.block_size = BLOCK_SIZE,
	.timeout = TIMEOUT,
	.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
	.mem_slab = &tx_0_mem_slab,
};

/** @brief Check actual PCM rate at frame_clk_freq=8000.
 *
 * - Configure I2S stream.
 */
ZTEST(drivers_i2s_clk_div, test_i2s_frame_clk_freq_08000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	int ret;

	i2s_cfg.frame_clk_freq = 8000;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_ok(ret, "i2s_configure() returned %d", ret);
}

/** @brief Check actual PCM rate at frame_clk_freq=16000.
 *
 * - Configure I2S stream.
 */
ZTEST(drivers_i2s_clk_div, test_i2s_frame_clk_freq_16000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	int ret;

	i2s_cfg.frame_clk_freq = 16000;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_ok(ret, "i2s_configure() returned %d", ret);
}

/** @brief Check actual PCM rate at frame_clk_freq=32000.
 *
 * - Configure I2S stream.
 */
ZTEST(drivers_i2s_clk_div, test_i2s_frame_clk_freq_32000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	int ret;

	i2s_cfg.frame_clk_freq = 32000;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_ok(ret, "i2s_configure() returned %d", ret);
}

/** @brief Check actual PCM rate at frame_clk_freq=44100.
 *
 * - Configure I2S stream.
 */
ZTEST(drivers_i2s_clk_div, test_i2s_frame_clk_freq_44100)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	int ret;

	i2s_cfg.frame_clk_freq = 44100;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_ok(ret, "i2s_configure() returned %d", ret);
}

/** @brief Check actual PCM rate at frame_clk_freq=48000.
 *
 * - Configure I2S stream.
 */
ZTEST(drivers_i2s_clk_div, test_i2s_frame_clk_freq_48000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	int ret;

	i2s_cfg.frame_clk_freq = 48000;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_ok(ret, "i2s_configure() returned %d", ret);
}

/** @brief Check actual PCM rate at frame_clk_freq=88200.
 *
 * - Configure I2S stream.
 */
ZTEST(drivers_i2s_clk_div, test_i2s_frame_clk_freq_88200)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	int ret;

	i2s_cfg.frame_clk_freq = 88200;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_ok(ret, "i2s_configure() returned %d", ret);
}

/** @brief Check actual PCM rate at frame_clk_freq=96000.
 *
 * - Configure I2S stream.
 */
ZTEST(drivers_i2s_clk_div, test_i2s_frame_clk_freq_96000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	int ret;

	i2s_cfg.frame_clk_freq = 96000;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_ok(ret, "i2s_configure() returned %d", ret);
}

static void *suite_setup(void)
{
	/* Check I2S Device. */
	dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);
	zassert_not_null(dev_i2s, "I2S device not found");
	zassert(device_is_ready(dev_i2s), "I2S device not ready");

	return 0;
}

ZTEST_SUITE(drivers_i2s_clk_div, NULL, suite_setup, NULL, NULL, NULL);
