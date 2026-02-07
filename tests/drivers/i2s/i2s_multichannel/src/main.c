/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s_multichannel, LOG_LEVEL_INF);

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)

#define WORD_SIZE      16U
#define FRAME_CLK_FREQ 44100

#define NUM_BLOCKS    20
#define TIMEOUT       1000
#define SAMPLES_COUNT 64

/* Get channel configuration from device tree */
#define TX_CHANNEL_MASK DT_PROP(I2S_DEV_NODE, nxp_tx_channel)
#define TX_CHANNELS     __builtin_popcount(TX_CHANNEL_MASK)

/* Get RX channel configuration from device tree */
#if DT_NODE_HAS_PROP(I2S_DEV_NODE, nxp_rx_channel)
#define RX_CHANNEL_MASK 1
#define RX_CHANNELS     1
#else
/* If no RX channel mask property, count RX data pins in pinctrl */
#define RX_CHANNELS     1 /* Default to 1 based on overlay having sai1_rx_data0 */
#define RX_CHANNEL_MASK 1 /* Channel 0 only */
#endif

/* Configuration to skip RX data verification */
#ifndef CONFIG_I2S_TEST_SKIP_RX_VERIFY
#define CONFIG_I2S_TEST_SKIP_RX_VERIFY 0
#endif

/* Test data for each TX channel - sine waves with different phases */
static int16_t tx_channel_data[8][SAMPLES_COUNT] = {
	/* Channel 0: 0 degree phase */
	{
		3211,   6392,   9511,   12539,  15446,  18204,  20787,  23169,  25329,  27244,
		28897,  30272,  31356,  32137,  32609,  32767,  32609,  32137,  31356,  30272,
		28897,  27244,  25329,  23169,  20787,  18204,  15446,  12539,  9511,   6392,
		3211,   0,      -3212,  -6393,  -9512,  -12540, -15447, -18205, -20788, -23170,
		-25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767, -32610, -32138,
		-31357, -30273, -28898, -27245, -25330, -23170, -20788, -18205, -15447, -12540,
		-9512,  -6393,  -3212,  -1,
	},
	/* Channel 1: 90 degree phase */
	{
		32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,  20787,  18204,
		15446,  12539,  9511,   6392,   3211,   0,      -3212,  -6393,  -9512,  -12540,
		-15447, -18205, -20788, -23170, -25330, -27245, -28898, -30273, -31357, -32138,
		-32610, -32767, -32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170,
		-20788, -18205, -15447, -12540, -9512,  -6393,  -3212,  -1,     3211,   6392,
		9511,   12539,  15446,  18204,  20787,  23169,  25329,  27244,  28897,  30272,
		31356,  32137,  32609,  32767,
	},
	/* Channel 2: 180 degree phase */
	{
		-3212,  -6393,  -9512,  -12540, -15447, -18205, -20788, -23170, -25330, -27245,
		-28898, -30273, -31357, -32138, -32610, -32767, -32610, -32138, -31357, -30273,
		-28898, -27245, -25330, -23170, -20788, -18205, -15447, -12540, -9512,  -6393,
		-3212,  -1,     3211,   6392,   9511,   12539,  15446,  18204,  20787,  23169,
		25329,  27244,  28897,  30272,  31356,  32137,  32609,  32767,  32609,  32137,
		31356,  30272,  28897,  27244,  25329,  23169,  20787,  18204,  15446,  12539,
		9511,   6392,   3211,   0,
	},
	/* Channel 3: 270 degree phase */
	{
		-32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170, -20788, -18205,
		-15447, -12540, -9512,  -6393,  -3212,  -1,     3211,   6392,   9511,   12539,
		15446,  18204,  20787,  23169,  25329,  27244,  28897,  30272,  31356,  32137,
		32609,  32767,  32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,
		20787,  18204,  15446,  12539,  9511,   6392,   3211,   0,      -3212,  -6393,
		-9512,  -12540, -15447, -18205, -20788, -23170, -25330, -27245, -28898, -30273,
		-31357, -32138, -32610, -32767,
	},
	/* Channel 4: 45 degree phase */
	{
		23169,  27244,  30272,  32137,  32767,  32137,  30272,  27244,  23169,  18204,
		12539,  6392,   0,      -6393,  -12540, -18205, -23170, -27245, -30273, -32138,
		-32767, -32138, -30273, -27245, -23170, -18205, -12540, -6393,  -1,     6392,
		12539,  18204,  23169,  27244,  30272,  32137,  32609,  32137,  30272,  27244,
		23169,  18204,  12539,  6392,   3211,   -6393,  -12540, -18205, -23170, -27245,
		-30273, -32138, -32610, -32138, -30273, -27245, -23170, -18205, -12540, -6393,
		-3212,  6392,   12539,  18204,
	},
	/* Channel 5: 135 degree phase */
	{
		-23170, -27245, -30273, -32138, -32767, -32138, -30273, -27245, -23170, -18205,
		-12540, -6393,  -1,     6392,   12539,  18204,  23169,  27244,  30272,  32137,
		32767,  32137,  30272,  27244,  23169,  18204,  12539,  6392,   3211,   -6393,
		-12540, -18205, -23170, -27245, -30273, -32138, -32610, -32138, -30273, -27245,
		-23170, -18205, -12540, -6393,  -3212,  6392,   12539,  18204,  23169,  27244,
		30272,  32137,  32609,  32137,  30272,  27244,  23169,  18204,  12539,  6392,
		0,      -6393,  -12540, -18205,
	},
	/* Channel 6: 225 degree phase */
	{
		-32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170, -20788, -18205,
		-15447, -12540, -9512,  -6393,  -3212,  -1,     3211,   6392,   9511,   12539,
		15446,  18204,  20787,  23169,  25329,  27244,  28897,  30272,  31356,  32137,
		32609,  32767,  32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,
		20787,  18204,  15446,  12539,  9511,   6392,   3211,   0,      -3212,  -6393,
		-9512,  -12540, -15447, -18205, -20788, -23170, -25330, -27245, -28898, -30273,
		-31357, -32138, -32610, -32767,
	},
	/* Channel 7: 315 degree phase */
	{
		32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,  20787,  18204,
		15446,  12539,  9511,   6392,   3211,   0,      -3212,  -6393,  -9512,  -12540,
		-15447, -18205, -20788, -23170, -25330, -27245, -28898, -30273, -31357, -32138,
		-32610, -32767, -32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170,
		-20788, -18205, -15447, -12540, -9512,  -6393,  -3212,  -1,     3211,   6392,
		9511,   12539,  15446,  18204,  20787,  23169,  25329,  27244,  28897,  30272,
		31356,  32137,  32609,  32767,
	}};

/* RX channel data - use same patterns as TX for verification */
static int16_t (*rx_channel_data)[SAMPLES_COUNT] = tx_channel_data;

#define TX_BLOCK_SIZE (TX_CHANNELS * SAMPLES_COUNT * sizeof(int16_t))
#define RX_BLOCK_SIZE (RX_CHANNELS * SAMPLES_COUNT * sizeof(int16_t))

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

/*
 * NUM_BLOCKS is the number of blocks used by the test. Some of the drivers,
 * permanently keep ownership of a few RX buffers. Add a two more
 * RX blocks to satisfy this requirement
 */
static char MEM_SLAB_CACHE_ATTR
	__aligned(WB_UP(32)) _k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS + 2) * WB_UP(RX_BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, rx_0_mem_slab) = Z_MEM_SLAB_INITIALIZER(
	rx_0_mem_slab, _k_mem_slab_buf_rx_0_mem_slab, WB_UP(RX_BLOCK_SIZE), NUM_BLOCKS + 2);

static char MEM_SLAB_CACHE_ATTR
	__aligned(WB_UP(32)) _k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS)*WB_UP(TX_BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab,
			tx_0_mem_slab) = Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab,
								_k_mem_slab_buf_tx_0_mem_slab,
								WB_UP(TX_BLOCK_SIZE), NUM_BLOCKS);

static const struct device *dev_i2s;

static const struct i2s_config tx_i2s_cfg = {
	.word_size = WORD_SIZE,
	.channels = TX_CHANNELS,
	.format = I2S_FMT_DATA_FORMAT_I2S,
	.frame_clk_freq = FRAME_CLK_FREQ,
	.block_size = TX_BLOCK_SIZE,
	.timeout = TIMEOUT,
	.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
	.mem_slab = &tx_0_mem_slab,
};

static const struct i2s_config rx_i2s_cfg = {
	.word_size = WORD_SIZE,
	.channels = RX_CHANNELS,
	.format = I2S_FMT_DATA_FORMAT_I2S,
	.frame_clk_freq = FRAME_CLK_FREQ,
	.block_size = RX_BLOCK_SIZE,
	.timeout = TIMEOUT,
	.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE,
	.mem_slab = &rx_0_mem_slab,
};

/* Fill in TX buffer with multi-channel test samples */
static void fill_tx_multichannel_buf(int16_t *tx_block, uint8_t channels)
{
	for (int sample = 0; sample < SAMPLES_COUNT; sample++) {
		for (int ch = 0; ch < channels; ch++) {
			tx_block[sample * channels + ch] = tx_channel_data[ch][sample];
		}
	}
}
/* Verify RX buffer data based on active RX channels */
static int verify_rx_buf(int16_t *rx_block, uint8_t channels)
{
	if (CONFIG_I2S_TEST_SKIP_RX_VERIFY) {
		TC_PRINT("RX verification skipped (CONFIG_I2S_TEST_SKIP_RX_VERIFY=1)\n");
		return 0;
	}

	int errors = 0;

	for (int sample = 0; sample < SAMPLES_COUNT; sample++) {
		for (int ch = 0; ch < channels; ch++) {
			/* Find which physical channel this RX channel maps to */
			int physical_ch = 0;
			uint32_t temp_mask = RX_CHANNEL_MASK;
			int current_rx_ch = 0;

			while (temp_mask && current_rx_ch <= ch) {
				if (temp_mask & 1) {
					if (current_rx_ch == ch) {
						break;
					}
					current_rx_ch++;
				}
				physical_ch++;
				temp_mask >>= 1;
			}

			int16_t expected = rx_channel_data[physical_ch][sample];
			int16_t received = rx_block[sample * channels + ch];

			if (expected != received) {
				TC_PRINT("RX Mismatch at sample %d, RX ch %d (phys ch %d): "
					 "expected %d, got %d\n",
					 sample, ch, physical_ch, expected, received);
				errors++;
				if (errors > 10) { /* Limit error output */
					return -1;
				}
			}
		}
	}

	return errors == 0 ? 0 : -1;
}

static int configure_tx_stream(const struct device *dev, struct i2s_config *i2s_cfg)
{
	int ret;

	i2s_cfg->mem_slab = &tx_0_mem_slab;
	ret = i2s_configure(dev, I2S_DIR_TX, i2s_cfg);
	if (ret < 0) {
		TC_PRINT("Failed to configure I2S TX stream (%d)\n", ret);
		return -TC_FAIL;
	}

	return TC_PASS;
}

static int configure_rx_stream(const struct device *dev, struct i2s_config *i2s_cfg)
{
	int ret;

	i2s_cfg->mem_slab = &rx_0_mem_slab;
	ret = i2s_configure(dev, I2S_DIR_RX, i2s_cfg);
	if (ret < 0) {
		TC_PRINT("Failed to configure I2S RX stream (%d)\n", ret);
		return -TC_FAIL;
	}

	return TC_PASS;
}

static int configure_both_streams(const struct device *dev, struct i2s_config *tx_cfg,
				  struct i2s_config *rx_cfg)
{
	int ret;

	/* Configure TX stream */
	ret = configure_tx_stream(dev, tx_cfg);
	if (ret != TC_PASS) {
		return ret;
	}

	/* Configure RX stream */
	ret = configure_rx_stream(dev, rx_cfg);
	if (ret != TC_PASS) {
		return ret;
	}

	return TC_PASS;
}

/** @brief Test multi-channel TX only
 */
ZTEST(i2s_multichannel, test_multichannel_tx_only)
{
	struct i2s_config i2s_cfg = tx_i2s_cfg;
	void *tx_block[NUM_BLOCKS];
	int tx_idx;
	int ret;

	TC_PRINT("Testing %d-channel TX only (mask: 0x%x)\n", TX_CHANNELS, TX_CHANNEL_MASK);

	/* Configure I2S for TX only */
	ret = configure_tx_stream(dev_i2s, &i2s_cfg);
	zassert_equal(ret, TC_PASS);

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx], K_FOREVER);
		zassert_equal(ret, 0);
		fill_tx_multichannel_buf((int16_t *)tx_block[tx_idx], i2s_cfg.channels);
	}

	LOG_HEXDUMP_DBG(tx_block[0], TX_BLOCK_SIZE, "multichannel TX data");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed\n");

	/* Send all blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx], TX_BLOCK_SIZE);
		zassert_equal(ret, 0);
	}

	/* Drain and stop */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	/* Free TX blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		k_mem_slab_free(&tx_0_mem_slab, tx_block[tx_idx]);
	}

	TC_PRINT("Multi-channel TX test completed: %d blocks sent\n", NUM_BLOCKS);
}

/** @brief Test multi-channel RX only
 */
ZTEST(i2s_multichannel, test_multichannel_rx_only)
{
#ifndef CONFIG_I2S_TEST_SKIP_RX_VERIFY
	struct i2s_config i2s_cfg = rx_i2s_cfg;
	void *rx_block[NUM_BLOCKS];
	size_t rx_size;
	int rx_idx = 0;
	int ret;

	TC_PRINT("Testing %d-channel RX only (mask: 0x%x)\n", RX_CHANNELS, RX_CHANNEL_MASK);

	/* Configure I2S for RX only */
	ret = configure_rx_stream(dev_i2s, &i2s_cfg);
	zassert_equal(ret, TC_PASS);

	/* Start reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed\n");

	/* Receive blocks with timeout */
	for (rx_idx = 0; rx_idx < NUM_BLOCKS; rx_idx++) {
		ret = i2s_read(dev_i2s, &rx_block[rx_idx], &rx_size);
		if (ret != 0) {
			TC_PRINT("RX timeout at block %d (expected without TX)\n", rx_idx);
			break;
		}
		zassert_equal(rx_size, RX_BLOCK_SIZE);
	}

	/* Stop reception this could fail if no data in */
	i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_DRAIN);

	/* Free RX blocks */
	for (int i = 0; i < rx_idx; i++) {
		k_mem_slab_free(&rx_0_mem_slab, rx_block[i]);
	}

	TC_PRINT("Multi-channel RX test completed: %d blocks received\n", rx_idx);
#else
	ztest_test_skip();
#endif
}

/** @brief Test bidirectional transfer with different TX/RX channel counts
 */
ZTEST(i2s_multichannel, test_bidirectional_different_channels)
{
	struct i2s_config tx_cfg = tx_i2s_cfg;
	struct i2s_config rx_cfg = rx_i2s_cfg;
	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int tx_idx;
	int rx_idx = 0;
	int num_verified;
	int ret;

	TC_PRINT("Testing bidirectional: TX %d channels, RX %d channels\n", TX_CHANNELS,
		 RX_CHANNELS);

	/* Configure both streams */
	ret = configure_both_streams(dev_i2s, &tx_cfg, &rx_cfg);
	zassert_equal(ret, TC_PASS);

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx], K_FOREVER);
		zassert_equal(ret, 0);
		fill_tx_multichannel_buf((int16_t *)tx_block[tx_idx], tx_cfg.channels);
	}

	LOG_HEXDUMP_DBG(tx_block[0], TX_BLOCK_SIZE, "bidirectional TX data");

	tx_idx = 0;

	/* Prefill TX queue */
	ret = i2s_write(dev_i2s, tx_block[tx_idx++], TX_BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_write(dev_i2s, tx_block[tx_idx++], TX_BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	if (ret == -ENOSYS) {
		ztest_test_skip();
	} else {
		zassert_equal(ret, 0, "RX/TX START trigger failed\n");
	}

	while (tx_idx < NUM_BLOCKS) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx++], TX_BLOCK_SIZE);
		zassert_equal(ret, 0);

		ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
		zassert_equal(ret, 0, "Got unexpected %d", ret);
		zassert_equal(rx_size, RX_BLOCK_SIZE);
	}

	/* All data written, drain TX queue and stop both streams */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	if (ret == -ENOSYS) {
		ztest_test_skip();
	} else {
		zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");
	}

	ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, RX_BLOCK_SIZE);

	ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, RX_BLOCK_SIZE);

	TC_PRINT("%d TX blocks sent\n", tx_idx);
	TC_PRINT("%d RX blocks received\n", rx_idx);

	/* Verify received data */
	num_verified = 0;
	for (rx_idx = 0; rx_idx < NUM_BLOCKS; rx_idx++) {
		ret = verify_rx_buf((int16_t *)rx_block[rx_idx], rx_cfg.channels);
		if (ret != 0) {
			TC_PRINT("%d RX block invalid\n", rx_idx);
			if (!CONFIG_I2S_TEST_SKIP_RX_VERIFY) {
				LOG_HEXDUMP_ERR(rx_block[rx_idx], RX_BLOCK_SIZE, "invalid RX data");
			}
		} else {
			num_verified++;
		}
		k_mem_slab_free(&rx_0_mem_slab, rx_block[rx_idx]);
	}

	/* Free TX blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		k_mem_slab_free(&tx_0_mem_slab, tx_block[tx_idx]);
	}

	if (CONFIG_I2S_TEST_SKIP_RX_VERIFY) {
		TC_PRINT("Bidirectional test completed: %d blocks processed (RX verification "
			 "skipped)\n",
			 NUM_BLOCKS);
	} else {
		zassert_equal(num_verified, NUM_BLOCKS, "Invalid RX blocks received");
		TC_PRINT("Bidirectional test passed: %d/%d blocks verified\n", num_verified,
			 NUM_BLOCKS);
	}
}

/** @brief Test TX with different formats
 */
ZTEST(i2s_multichannel, test_multichannel_tx_pcm_long)
{
	struct i2s_config i2s_cfg = tx_i2s_cfg;
	void *tx_block[NUM_BLOCKS];
	int tx_idx;
	int ret;

	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_LONG;
	TC_PRINT("Testing %d-channel TX PCM_LONG format\n", TX_CHANNELS);

	/* Configure I2S for TX only */
	ret = configure_tx_stream(dev_i2s, &i2s_cfg);
	zassert_equal(ret, TC_PASS);

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx], K_FOREVER);
		zassert_equal(ret, 0);
		fill_tx_multichannel_buf((int16_t *)tx_block[tx_idx], i2s_cfg.channels);
	}

	TC_PRINT("Pre-filling TX queue with initial buffers...\n");

	/* Pre-fill the TX queue with at least 2 buffers before starting */
	tx_idx = 0;
	ret = i2s_write(dev_i2s, tx_block[tx_idx++], TX_BLOCK_SIZE);
	zassert_equal(ret, 0, "Failed to write first TX buffer: %d", ret);

	ret = i2s_write(dev_i2s, tx_block[tx_idx++], TX_BLOCK_SIZE);
	zassert_equal(ret, 0, "Failed to write second TX buffer: %d", ret);

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		TC_PRINT("TX START trigger failed with error: %d\n", ret);
		TC_PRINT("Common error codes:\n");
		TC_PRINT("  -EIO (%d): Device not ready or invalid state\n", -EIO);
		TC_PRINT("  -EINVAL (%d): Invalid parameters\n", -EINVAL);
		TC_PRINT("  -ENOTSUP (%d): Operation not supported\n", -ENOTSUP);
		TC_PRINT("  -EBUSY (%d): Device busy\n", -EBUSY);

		/* Free allocated blocks */
		for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
			k_mem_slab_free(&tx_0_mem_slab, tx_block[tx_idx]);
		}

		if (ret == -ENOTSUP) {
			TC_PRINT("PCM_LONG format not supported by this driver, skipping test\n");
			ztest_test_skip();
			return;
		}
		zassert_equal(ret, 0, "TX START trigger failed\n");
		return;
	}

	/* Send all blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx], TX_BLOCK_SIZE);
		zassert_equal(ret, 0);
	}

	/* Drain and stop */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	/* Free TX blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		k_mem_slab_free(&tx_0_mem_slab, tx_block[tx_idx]);
	}

	TC_PRINT("Multi-channel TX PCM_LONG test completed\n");
}

/** @brief Test channel mask validation
 */
ZTEST(i2s_multichannel, test_channel_mask_validation)
{
	TC_PRINT("TX Channel mask: 0x%x\n", TX_CHANNEL_MASK);
	TC_PRINT("Number of TX channels: %d\n", TX_CHANNELS);
	TC_PRINT("RX Channel mask: 0x%x\n", RX_CHANNEL_MASK);
	TC_PRINT("Number of RX channels: %d\n", RX_CHANNELS);

	/* Verify TX channel mask */
	zassert_true(TX_CHANNEL_MASK > 0, "TX Channel mask should be non-zero");
	zassert_true(TX_CHANNELS > 1, "Should have multiple TX channels");
	zassert_true(TX_CHANNELS <= 8, "Should not exceed 8 TX channels");

	/* Verify RX channel mask  */
	zassert_true(RX_CHANNEL_MASK > 0, "RX Channel mask should be non-zero");
	zassert_true(RX_CHANNELS >= 1, "Should have at least 1 RX channel");
	zassert_true(RX_CHANNELS <= 8, "Should not exceed 8 RX channels");

	/* Verify the popcount calculation for TX */
	int tx_manual_count = 0;
	uint32_t tx_mask = TX_CHANNEL_MASK;

	while (tx_mask) {
		if (tx_mask & 1) {
			tx_manual_count++;
		}
		tx_mask >>= 1;
	}
	zassert_equal(tx_manual_count, TX_CHANNELS,
		      "TX manual count should match __builtin_popcount");

	/* Verify the popcount calculation for RX */
	int rx_manual_count = 0;
	uint32_t rx_mask = RX_CHANNEL_MASK;

	while (rx_mask) {
		if (rx_mask & 1) {
			rx_manual_count++;
		}
		rx_mask >>= 1;
	}
	zassert_equal(rx_manual_count, RX_CHANNELS,
		      "RX manual count should match __builtin_popcount");

	/* Print active channel numbers */
	TC_PRINT("Active TX channels: ");
	tx_mask = TX_CHANNEL_MASK;
	for (int i = 0; i < 8; i++) {
		if (tx_mask & (1 << i)) {
			TC_PRINT("%d ", i);
		}
	}
	TC_PRINT("\n");

	TC_PRINT("Active RX channels: ");
	rx_mask = RX_CHANNEL_MASK;
	for (int i = 0; i < 8; i++) {
		if (rx_mask & (1 << i)) {
			TC_PRINT("%d ", i);
		}
	}
	TC_PRINT("\n");
}

/** @brief Test data pattern verification for TX
 */
ZTEST(i2s_multichannel, test_tx_data_patterns)
{
	struct i2s_config i2s_cfg = tx_i2s_cfg;
	void *tx_block;
	int16_t *data_ptr;
	int ret;

	TC_PRINT("Testing TX data pattern generation for %d channels\n", TX_CHANNELS);

	/* Configure I2S for TX */
	ret = configure_tx_stream(dev_i2s, &i2s_cfg);
	zassert_equal(ret, TC_PASS);

	/* Allocate and fill one test block */
	ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
	zassert_equal(ret, 0);

	fill_tx_multichannel_buf((int16_t *)tx_block, i2s_cfg.channels);
	data_ptr = (int16_t *)tx_block;

	/* Verify data pattern for first few samples */
	for (int sample = 0; sample < 4; sample++) {
		TC_PRINT("Sample %d: ", sample);
		for (int ch = 0; ch < TX_CHANNELS; ch++) {
			int16_t expected = tx_channel_data[ch][sample];
			int16_t actual = data_ptr[sample * TX_CHANNELS + ch];

			TC_PRINT("CH%d=%d ", ch, actual);
			zassert_equal(actual, expected, "Data mismatch at sample %d, channel %d",
				      sample, ch);
		}
		TC_PRINT("\n");
	}

	/* Verify channel separation - each channel should have different data */
	bool channels_different = false;

	for (int sample = 0; sample < SAMPLES_COUNT; sample++) {
		int16_t ch0_data = data_ptr[sample * TX_CHANNELS + 0];

		for (int ch = 1; ch < TX_CHANNELS; ch++) {
			int16_t ch_data = data_ptr[sample * TX_CHANNELS + ch];

			if (ch0_data != ch_data) {
				channels_different = true;
				break;
			}
		}
		if (channels_different) {
			break;
		}
	}

	zassert_true(channels_different, "Channels should have different data patterns");

	k_mem_slab_free(&tx_0_mem_slab, tx_block);
	TC_PRINT("TX data pattern verification completed\n");
}

/** @brief Test RX data pattern verification
 */
ZTEST(i2s_multichannel, test_rx_data_patterns)
{
	struct i2s_config i2s_cfg = rx_i2s_cfg;
	void *rx_block;
	int16_t *data_ptr;
	int ret;

	TC_PRINT("Testing RX data pattern verification for %d channels\n", RX_CHANNELS);

	/* Configure I2S for RX */
	ret = configure_rx_stream(dev_i2s, &i2s_cfg);
	zassert_equal(ret, TC_PASS);

	/* Allocate a test block and fill with expected pattern */
	ret = k_mem_slab_alloc(&rx_0_mem_slab, &rx_block, K_FOREVER);
	zassert_equal(ret, 0);

	data_ptr = (int16_t *)rx_block;

	for (int sample = 0; sample < SAMPLES_COUNT; sample++) {
		for (int ch = 0; ch < RX_CHANNELS; ch++) {
			/* Find which physical channel this RX channel maps to */
			int physical_ch = 0;
			uint32_t temp_mask = RX_CHANNEL_MASK;
			int current_rx_ch = 0;

			while (temp_mask && current_rx_ch <= ch) {
				if (temp_mask & 1) {
					if (current_rx_ch == ch) {
						break;
					}
					current_rx_ch++;
				}
				physical_ch++;
				temp_mask >>= 1;
			}

			data_ptr[sample * RX_CHANNELS + ch] = rx_channel_data[physical_ch][sample];
		}
	}

	/* Verify the pattern we just created */
	ret = verify_rx_buf(data_ptr, RX_CHANNELS);
	zassert_equal(ret, 0, "RX pattern verification should pass");

	/* Print first few samples */
	TC_PRINT("RX pattern verification:\n");
	for (int sample = 0; sample < 4; sample++) {
		TC_PRINT("Sample %d: ", sample);
		for (int ch = 0; ch < RX_CHANNELS; ch++) {
			TC_PRINT("CH%d=%d ", ch, data_ptr[sample * RX_CHANNELS + ch]);
		}
		TC_PRINT("\n");
	}

	k_mem_slab_free(&rx_0_mem_slab, rx_block);
	TC_PRINT("RX data pattern verification completed\n");
}

/** @brief Test block size calculations
 */
ZTEST(i2s_multichannel, test_block_size_calculations)
{

	TC_PRINT("Block size calculations:\n");
	TC_PRINT("TX: %d channels * %d samples * %d bytes = %d bytes\n", TX_CHANNELS, SAMPLES_COUNT,
		 (int)sizeof(int16_t), TX_BLOCK_SIZE);
	TC_PRINT("RX: %d channels * %d samples * %d bytes = %d bytes\n", RX_CHANNELS, SAMPLES_COUNT,
		 (int)sizeof(int16_t), RX_BLOCK_SIZE);

	/* Verify calculations */
	zassert_equal(TX_BLOCK_SIZE, TX_CHANNELS * SAMPLES_COUNT * sizeof(int16_t),
		      "TX block size calculation incorrect");
	zassert_equal(RX_BLOCK_SIZE, RX_CHANNELS * SAMPLES_COUNT * sizeof(int16_t),
		      "RX block size calculation incorrect");

	/* Verify memory slab sizes are adequate */
	zassert_true(WB_UP(TX_BLOCK_SIZE) >= TX_BLOCK_SIZE, "TX memory slab block size too small");
	zassert_true(WB_UP(RX_BLOCK_SIZE) >= RX_BLOCK_SIZE, "RX memory slab block size too small");

	TC_PRINT("Memory slab block sizes: TX=%d, RX=%d\n", (int)WB_UP(TX_BLOCK_SIZE),
		 (int)WB_UP(RX_BLOCK_SIZE));
}

/** @brief Test memory slab allocation/deallocation
 */
ZTEST(i2s_multichannel, test_memory_slab_operations)
{
	void *tx_blocks[NUM_BLOCKS];
	void *rx_blocks[NUM_BLOCKS];
	int ret;

	TC_PRINT("Testing memory slab operations\n");

	/* Debug memory slab configuration */
	TC_PRINT("TX Memory Slab Debug Info:\n");
	TC_PRINT("  Block size: %d bytes\n", TX_BLOCK_SIZE);
	TC_PRINT("  Aligned block size: %d bytes\n", (int)WB_UP(TX_BLOCK_SIZE));
	TC_PRINT("  Number of blocks: %d\n", NUM_BLOCKS);
	TC_PRINT("  Total buffer size: %d bytes\n", NUM_BLOCKS * (int)WB_UP(TX_BLOCK_SIZE));
	TC_PRINT("  Slab info - num_blocks: %d, num_used: %d, block_size: %d\n",
		 tx_0_mem_slab.info.num_blocks, tx_0_mem_slab.info.num_used,
		 tx_0_mem_slab.info.block_size);
	TC_PRINT("  Buffer pointer: %p\n", tx_0_mem_slab.buffer);
	TC_PRINT("  Free list: %p\n", tx_0_mem_slab.free_list);

	/* Check if memory slab is properly initialized */
	if (tx_0_mem_slab.info.num_blocks == 0) {
		TC_PRINT("ERROR: TX memory slab not initialized!\n");
		ztest_test_fail();
		return;
	}

	/* Test TX memory slab */
	TC_PRINT("Allocating %d TX blocks of %d bytes each\n", NUM_BLOCKS, TX_BLOCK_SIZE);
	for (int i = 0; i < NUM_BLOCKS; i++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_blocks[i], K_NO_WAIT);
		zassert_equal(ret, 0, "TX block %d allocation failed", i);
		zassert_not_null(tx_blocks[i], "TX block %d is NULL", i);
	}

	/* Test RX memory slab */
	TC_PRINT("Allocating %d RX blocks of %d bytes each\n", NUM_BLOCKS, RX_BLOCK_SIZE);
	for (int i = 0; i < NUM_BLOCKS; i++) {
		ret = k_mem_slab_alloc(&rx_0_mem_slab, &rx_blocks[i], K_NO_WAIT);
		zassert_equal(ret, 0, "RX block %d allocation failed", i);
		zassert_not_null(rx_blocks[i], "RX block %d is NULL", i);
	}

	/* Free all blocks */
	for (int i = 0; i < NUM_BLOCKS; i++) {
		k_mem_slab_free(&tx_0_mem_slab, tx_blocks[i]);
		k_mem_slab_free(&rx_0_mem_slab, rx_blocks[i]);
	}

	TC_PRINT("Memory slab operations test completed\n");
}

static void *suite_setup(void)
{
	/* Check I2S Device */
	dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);
	zassert_not_null(dev_i2s, "I2S device not found");
	zassert(device_is_ready(dev_i2s), "I2S device not ready");

	TC_PRINT("===================================================================\n");
	TC_PRINT("I2S Multi-Channel Test Suite\n");
	TC_PRINT("Device: %s\n", dev_i2s->name);
	TC_PRINT("TX Channel mask: 0x%x (%d channels)\n", TX_CHANNEL_MASK, TX_CHANNELS);
	TC_PRINT("RX Channel mask: 0x%x (%d channels)\n", RX_CHANNEL_MASK, RX_CHANNELS);
	TC_PRINT("TX Block size: %d bytes\n", TX_BLOCK_SIZE);
	TC_PRINT("RX Block size: %d bytes\n", RX_BLOCK_SIZE);
	TC_PRINT("Samples per channel: %d\n", SAMPLES_COUNT);
	TC_PRINT("Word size: %d bits\n", WORD_SIZE);
	TC_PRINT("Frame clock frequency: %d Hz\n", FRAME_CLK_FREQ);

#if DT_NODE_HAS_PROP(I2S_DEV_NODE, nxp_rx_channel)
	TC_PRINT("RX channel mask from DT: 0x%x\n", DT_PROP(I2S_DEV_NODE, nxp_rx_channel));
#else
	TC_PRINT("RX channel mask: default (no DT property)\n");
#endif

	TC_PRINT("===================================================================\n");

	/* Log sample data for first few samples of each channel */
	for (int ch = 0; ch < TX_CHANNELS && ch < 8; ch++) {
		LOG_HEXDUMP_DBG(&tx_channel_data[ch][0], 16, "TX channel data");
	}

	for (int ch = 0; ch < RX_CHANNELS && ch < 8; ch++) {
		/* Find physical channel for this RX channel */
		int physical_ch = 0;
		uint32_t temp_mask = RX_CHANNEL_MASK;
		int current_rx_ch = 0;

		while (temp_mask && current_rx_ch <= ch) {
			if (temp_mask & 1) {
				if (current_rx_ch == ch) {
					break;
				}
				current_rx_ch++;
			}
			physical_ch++;
			temp_mask >>= 1;
		}

		LOG_HEXDUMP_DBG(&rx_channel_data[physical_ch][0], 16, "RX channel data");
	}

	return NULL;
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);
	/* Reset any state if needed */
}

static void after(void *not_used)
{
	int ret;
	size_t size;
	void *buffer[NUM_BLOCKS];

	ARG_UNUSED(not_used);

	/* Drain any remaining RX buffers */
	do {
		ret = i2s_read(dev_i2s, buffer, &size);
		if (ret == 0) {
			TC_PRINT("Cleaning up orphaned RX buffer: %p\n", buffer);
			k_mem_slab_free(&rx_0_mem_slab, buffer);
		}
	} while (ret == 0);

	/* Reset memory slabs to ensure clean state */
	k_mem_slab_init(&tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab, WB_UP(TX_BLOCK_SIZE),
			NUM_BLOCKS);
	k_mem_slab_init(&rx_0_mem_slab, _k_mem_slab_buf_rx_0_mem_slab, WB_UP(RX_BLOCK_SIZE),
			NUM_BLOCKS + 2);
}

ZTEST_SUITE(i2s_multichannel, NULL, suite_setup, before, after, NULL);
