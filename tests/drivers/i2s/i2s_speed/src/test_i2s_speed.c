/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <ztest.h>
#include <drivers/i2s.h>

#define I2S_DEV_NAME "I2S_0"
#define NUM_BLOCKS 20
#define SAMPLE_NO 64

/* The data_l represent a sine wave */
static int16_t data_l[SAMPLE_NO] = {
	  3211,   6392,   9511,  12539,  15446,  18204,  20787,  23169,
	 25329,  27244,  28897,  30272,  31356,  32137,  32609,  32767,
	 32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,
	 20787,  18204,  15446,  12539,   9511,   6392,   3211,      0,
	 -3212,  -6393,  -9512, -12540, -15447, -18205, -20788, -23170,
	-25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767,
	-32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170,
	-20788, -18205, -15447, -12540,  -9512,  -6393,  -3212,     -1,
};

/* The data_r represent a sine wave shifted by 90 deg to data_l sine wave */
static int16_t data_r[SAMPLE_NO] = {
	 32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,
	 20787,  18204,  15446,  12539,   9511,   6392,   3211,      0,
	 -3212,  -6393,  -9512, -12540, -15447, -18205, -20788, -23170,
	-25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767,
	-32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170,
	-20788, -18205, -15447, -12540,  -9512,  -6393,  -3212,     -1,
	  3211,   6392,   9511,  12539,  15446,  18204,  20787,  23169,
	 25329,  27244,  28897,  30272,  31356,  32137,  32609,  32767,
};

#define BLOCK_SIZE (2 * sizeof(data_l))

K_MEM_SLAB_DEFINE(rx_0_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 32);
K_MEM_SLAB_DEFINE(tx_0_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 32);

static void fill_buf(int16_t *tx_block, int att)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = data_l[i] >> att;
		tx_block[2 * i + 1] = data_r[i] >> att;
	}
}

static int verify_buf(int16_t *rx_block, int att)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		if (rx_block[2 * i] != data_l[i] >> att) {
			TC_PRINT("Error: att %d: data_l mismatch at position "
				 "%d, expected %d, actual %d\n",
				 att, i, data_l[i] >> att, rx_block[2 * i]);
			return -TC_FAIL;
		}
		if (rx_block[2 * i + 1] != data_r[i] >> att) {
			TC_PRINT("Error: att %d: data_r mismatch at position "
				 "%d, expected %d, actual %d\n",
				 att, i, data_r[i] >> att, rx_block[2 * i + 1]);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

#define TIMEOUT          2000
#define FRAME_CLK_FREQ   44000

/** Configure I2S TX transfer. */
void test_i2s_tx_transfer_configure(void)
{
	struct device *dev_i2s;
	struct i2s_config i2s_cfg;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	/* Configure */

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.mem_slab = &tx_0_mem_slab;
	i2s_cfg.timeout = TIMEOUT;
	i2s_cfg.options = I2S_OPT_LOOPBACK;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S TX stream");
}

/** Configure I2S RX transfer. */
void test_i2s_rx_transfer_configure(void)
{
	struct device *dev_i2s;
	struct i2s_config i2s_cfg;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	/* Configure */

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.mem_slab = &rx_0_mem_slab;
	i2s_cfg.timeout = TIMEOUT;
	i2s_cfg.options = I2S_OPT_LOOPBACK;

	ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S RX stream");
}

/** @brief Short I2S transfer.
 *
 * - TX stream START trigger starts transmission.
 * - RX stream START trigger starts reception.
 * - sending / receiving a short sequence of data returns success.
 * - TX stream DRAIN trigger empties the transmit queue.
 * - RX stream STOP trigger stops reception.
 */
void test_i2s_transfer_short(void)
{
	struct device *dev_i2s;
	void *rx_block[3];
	void *tx_block;
	size_t rx_size;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	/* Prefill TX queue */
	for (int i = 0; i < 3; i++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
		zassert_equal(ret, 0, NULL);
		fill_buf((uint16_t *)tx_block, i);

		ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE);
		zassert_equal(ret, 0, NULL);

		TC_PRINT("%d->OK\n", i);
	}

	/* Start reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	/* All data written, drain TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s, &rx_block[0], &rx_size);
	zassert_equal(ret, 0, NULL);
	zassert_equal(rx_size, BLOCK_SIZE, NULL);

	ret = i2s_read(dev_i2s, &rx_block[1], &rx_size);
	zassert_equal(ret, 0, NULL);
	zassert_equal(rx_size, BLOCK_SIZE, NULL);

	/* All but one data block read, stop reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	ret = i2s_read(dev_i2s, &rx_block[2], &rx_size);
	zassert_equal(ret, 0, NULL);
	zassert_equal(rx_size, BLOCK_SIZE, NULL);

	/* Verify received data */
	ret = verify_buf((uint16_t *)rx_block[0], 0);
	zassert_equal(ret, 0, NULL);
	k_mem_slab_free(&rx_0_mem_slab, &rx_block[0]);
	TC_PRINT("%d<-OK\n", 1);

	ret = verify_buf((uint16_t *)rx_block[1], 1);
	zassert_equal(ret, 0, NULL);
	k_mem_slab_free(&rx_0_mem_slab, &rx_block[1]);
	TC_PRINT("%d<-OK\n", 2);

	ret = verify_buf((uint16_t *)rx_block[2], 2);
	zassert_equal(ret, 0, NULL);
	k_mem_slab_free(&rx_0_mem_slab, &rx_block[2]);
	TC_PRINT("%d<-OK\n", 3);
}

/** @brief Long I2S transfer.
 *
 * - TX stream START trigger starts transmission.
 * - RX stream START trigger starts reception.
 * - sending / receiving a long sequence of data returns success.
 * - TX stream DRAIN trigger empties the transmit queue.
 * - RX stream STOP trigger stops reception.
 */
void test_i2s_transfer_long(void)
{
	struct device *dev_i2s;
	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int tx_idx;
	int rx_idx = 0;
	int num_verified;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx],
				       K_FOREVER);
		zassert_equal(ret, 0, NULL);
		fill_buf((uint16_t *)tx_block[tx_idx], tx_idx % 3);
	}

	tx_idx = 0;

	/* Prefill TX queue */
	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0, NULL);

	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0, NULL);

	/* Start reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	for (; tx_idx < NUM_BLOCKS; ) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
		zassert_equal(ret, 0, NULL);

		ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
		zassert_equal(ret, 0, NULL);
		zassert_equal(rx_size, BLOCK_SIZE, NULL);
	}

	/* All data written, flush TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0, NULL);
	zassert_equal(rx_size, BLOCK_SIZE, NULL);

	/* All but one data block read, stop reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0, NULL);
	zassert_equal(rx_size, BLOCK_SIZE, NULL);

	TC_PRINT("%d TX blocks sent\n", tx_idx);
	TC_PRINT("%d RX blocks received\n", rx_idx);

	/* Verify received data */
	num_verified = 0;
	for (rx_idx = 0; rx_idx < NUM_BLOCKS; rx_idx++) {
		ret = verify_buf((uint16_t *)rx_block[rx_idx], rx_idx % 3);
		if (ret != 0) {
			TC_PRINT("%d RX block invalid\n", rx_idx);
		} else {
			num_verified++;
		}
		k_mem_slab_free(&rx_0_mem_slab, &rx_block[rx_idx]);
	}
	zassert_equal(num_verified, NUM_BLOCKS, "Invalid RX blocks received");
}
