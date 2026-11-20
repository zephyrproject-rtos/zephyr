/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>

#define SAMPLE_NO      32
#define FRAME_CLK_FREQ 8000U
#define TIMEOUT_MS     50
#define RX_BLOCK_SIZE  256
#define TX_BLOCK_SIZE  (2 * SAMPLE_NO * sizeof(int16_t))
#define BIG_BLOCK_SIZE 1200
#define VAL_L          11
#define VAL_R          22

static const struct device *const lp_dev = DEVICE_DT_GET(DT_NODELABEL(lp_i2s));

K_MEM_SLAB_DEFINE(rx_slab, RX_BLOCK_SIZE, 4, 4);

static void rx_config(struct i2s_config *cfg, struct k_mem_slab *slab, size_t block_size,
		      uint8_t word_size, i2s_opt_t options)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->word_size = word_size;
	cfg->channels = 2U;
	cfg->format = I2S_FMT_DATA_FORMAT_I2S;
	cfg->options = options;
	cfg->frame_clk_freq = FRAME_CLK_FREQ;
	cfg->mem_slab = slab;
	cfg->block_size = block_size;
	cfg->timeout = TIMEOUT_MS;
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(lp_dev), "LP I2S device is not ready");
	return NULL;
}

static void suite_before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)i2s_trigger(lp_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
}

ZTEST(i2s_lp, test_write_rejected)
{
	uint8_t buf[4] = {0};

	zassert_equal(i2s_write(lp_dev, buf, sizeof(buf)), -ENOTSUP);
}

ZTEST(i2s_lp, test_configure_rejects_invalid)
{
	struct i2s_config cfg;
	int ret;

	rx_config(&cfg, &rx_slab, RX_BLOCK_SIZE, 16U,
		  I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET);
	ret = i2s_configure(lp_dev, I2S_DIR_TX, &cfg);
	zassert_equal(ret, -EINVAL);

	ret = i2s_configure(lp_dev, I2S_DIR_BOTH, &cfg);
	zassert_equal(ret, -EINVAL);

	rx_config(&cfg, &rx_slab, RX_BLOCK_SIZE, 24U,
		  I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET);
	ret = i2s_configure(lp_dev, I2S_DIR_RX, &cfg);
	zassert_equal(ret, -EINVAL);

	rx_config(&cfg, &rx_slab, RX_BLOCK_SIZE, 16U,
		  I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET);
	cfg.block_size = 0U;
	ret = i2s_configure(lp_dev, I2S_DIR_RX, &cfg);
	zassert_equal(ret, -EINVAL);
}

ZTEST(i2s_lp, test_target_start_times_out)
{
	struct i2s_config cfg;
	void *block = NULL;
	size_t size = 0U;
	int ret;

	rx_config(&cfg, &rx_slab, RX_BLOCK_SIZE, 16U,
		  I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET);
	ret = i2s_configure(lp_dev, I2S_DIR_RX, &cfg);
	zassert_equal(ret, 0);

	ret = i2s_trigger(lp_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0);

	ret = i2s_configure(lp_dev, I2S_DIR_RX, &cfg);
	zassert_equal(ret, -EIO);

	ret = i2s_read(lp_dev, &block, &size);
	zassert_equal(ret, -EAGAIN);
	zassert_is_null(block);

	ret = i2s_trigger(lp_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
	zassert_equal(ret, 0);

	ret = i2s_configure(lp_dev, I2S_DIR_RX, &cfg);
	zassert_equal(ret, 0);
}

ZTEST(i2s_lp, test_controller_configure)
{
	struct i2s_config cfg;
	int ret;

	rx_config(&cfg, &rx_slab, RX_BLOCK_SIZE, 16U,
		  I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER);
	ret = i2s_configure(lp_dev, I2S_DIR_RX, &cfg);
	zassert_equal(ret, 0);
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2s0), okay)

static const struct device *const hp_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));

K_MEM_SLAB_DEFINE(tx_slab, TX_BLOCK_SIZE, 6, 4);
K_MEM_SLAB_DEFINE(rx_big_slab, BIG_BLOCK_SIZE, 4, 4);

static void fill_buf(int16_t *tx_block)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[(2 * i)] = (int16_t)(VAL_L + i);
		tx_block[(2 * i) + 1] = (int16_t)(VAL_R + i);
	}
}

static int pattern_found(const int16_t *buf, size_t samples)
{
	if (samples < 4U) {
		return -1;
	}

	for (size_t i = 0U; i + 3U < samples; i += 2U) {
		if ((buf[i] == VAL_L) && (buf[i + 1] == VAL_R) &&
		    (buf[i + 2] == (int16_t)(VAL_L + 1)) && (buf[i + 3] == (int16_t)(VAL_R + 1))) {
			return 0;
		}
	}

	return -1;
}

static int configure_pair(struct k_mem_slab *rx, size_t rx_block)
{
	struct i2s_config cfg;
	int ret;

	memset(&cfg, 0, sizeof(cfg));
	cfg.word_size = 16U;
	cfg.channels = 2U;
	cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	cfg.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	cfg.frame_clk_freq = FRAME_CLK_FREQ;
	cfg.mem_slab = &tx_slab;
	cfg.block_size = TX_BLOCK_SIZE;
	cfg.timeout = 2000;
	ret = i2s_configure(hp_dev, I2S_DIR_TX, &cfg);
	if (ret != 0) {
		return ret;
	}

	cfg.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	cfg.mem_slab = rx;
	cfg.block_size = rx_block;
	cfg.timeout = 2000;
	return i2s_configure(lp_dev, I2S_DIR_RX, &cfg);
}

static int write_pattern(void)
{
	int16_t tx_block[SAMPLE_NO * 2];

	fill_buf(tx_block);
	return i2s_buf_write(hp_dev, tx_block, TX_BLOCK_SIZE);
}

static void loopback_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_true(device_is_ready(hp_dev), "HP I2S device is not ready");
	(void)i2s_trigger(lp_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
	(void)i2s_trigger(hp_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
}

static void exchange(struct k_mem_slab *rx, size_t rx_block, int writes, int reads)
{
	int primed = (writes < 4) ? writes : 4;
	int ret;

	ret = configure_pair(rx, rx_block);
	zassert_equal(ret, 0);

	for (int i = 0; i < primed; i++) {
		ret = write_pattern();
		zassert_equal(ret, 0, "TX prime %d failed (%d)", i, ret);
	}

	ret = i2s_trigger(lp_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0);
	ret = i2s_trigger(hp_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0);

	for (int i = primed; i < writes; i++) {
		ret = write_pattern();
		zassert_equal(ret, 0, "TX write %d failed (%d)", i, ret);
	}

	for (int i = 0; i < reads; i++) {
		int16_t rx_copy[BIG_BLOCK_SIZE / sizeof(int16_t)];
		size_t size = 0U;
		int found;

		ret = i2s_buf_read(lp_dev, rx_copy, &size);
		zassert_equal(ret, 0, "RX read %d failed (%d)", i, ret);
		zassert_equal(size, rx_block);
		found = pattern_found(rx_copy, size / sizeof(int16_t));
		zassert_equal(found, 0, "pattern missing in block %d", i);
	}

	ret = i2s_trigger(hp_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	zassert_equal(ret, 0);
	ret = i2s_trigger(lp_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
	zassert_equal(ret, 0);
}

ZTEST(i2s_lp_loopback, test_gpio_loopback)
{
	exchange(&rx_slab, RX_BLOCK_SIZE, 4, 2);
}

ZTEST(i2s_lp_loopback, test_large_block)
{
	exchange(&rx_big_slab, BIG_BLOCK_SIZE, 16, 1);
}

ZTEST_SUITE(i2s_lp_loopback, NULL, NULL, loopback_before, NULL, NULL);

#endif /* DT_NODE_HAS_STATUS(i2s0, okay) */

ZTEST_SUITE(i2s_lp, NULL, suite_setup, suite_before, NULL, NULL);
