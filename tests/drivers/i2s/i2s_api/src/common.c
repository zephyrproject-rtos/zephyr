/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/drivers/i2s.h>
#include "i2s_api_test.h"

K_MEM_SLAB_DEFINE(rx_mem_slab, BLOCK_SIZE, NUM_RX_BLOCKS, 32);
K_MEM_SLAB_DEFINE(tx_mem_slab, BLOCK_SIZE, NUM_TX_BLOCKS, 32);

/* The data_l represent a sine wave */
ZTEST_DMEM int16_t data_l[SAMPLE_NO] = {
	  6392,  12539,  18204,  23169,  27244,  30272,  32137,  32767,  32137,
	 30272,  27244,  23169,  18204,  12539,   6392,      0,  -6393, -12540,
	-18205, -23170, -27245, -30273, -32138, -32767, -32138, -30273, -27245,
	-23170, -18205, -12540,  -6393,     -1,
};

/* The data_r represent a sine wave with double the frequency of data_l */
ZTEST_DMEM int16_t data_r[SAMPLE_NO] = {
	 12539,  23169,  30272,  32767,  30272,  23169,  12539,      0, -12540,
	-23170, -30273, -32767, -30273, -23170, -12540,     -1,  12539,  23169,
	 30272,  32767,  30272,  23169,  12539,      0, -12540, -23170, -30273,
	-32767, -30273, -23170, -12540,     -1,
};

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
	static ZTEST_DMEM int offset = -1;

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

void fill_buf_const(int16_t *tx_block, int16_t val_l, int16_t val_r)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = val_l;
		tx_block[2 * i + 1] = val_r;
	}
}

int verify_buf_const(int16_t *rx_block, int16_t val_l, int16_t val_r)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		if (rx_block[2 * i] != val_l) {
			TC_PRINT("Error: data_l mismatch at position "
				 "%d, expected %d, actual %d\n",
				 i, val_l, rx_block[2 * i]);
			return -TC_FAIL;
		}
		if (rx_block[2 * i + 1] != val_r) {
			TC_PRINT("Error: data_r mismatch at position "
				 "%d, expected %d, actual %d\n",
				 i, val_r, rx_block[2 * i + 1]);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

static int tx_block_write_slab(const struct device *dev_i2s, int att, int err,
			       struct k_mem_slab *slab)
{
	char tx_block[BLOCK_SIZE];
	int ret;

	fill_buf((uint16_t *)tx_block, att);
	ret = i2s_buf_write(dev_i2s, tx_block, BLOCK_SIZE);
	if (ret != err) {
		TC_PRINT("Error: i2s_write failed expected %d, actual %d\n",
			 err, ret);
		return -TC_FAIL;
	}

	return TC_PASS;
}

int tx_block_write(const struct device *dev_i2s, int att, int err)
{
	return tx_block_write_slab(dev_i2s, att, err, &tx_mem_slab);
}

static int rx_block_read_slab(const struct device *dev_i2s, int att,
			      struct k_mem_slab *slab)
{
	char rx_block[BLOCK_SIZE];
	size_t rx_size;
	int ret;

	ret = i2s_buf_read(dev_i2s, rx_block, &rx_size);
	if (ret < 0 || rx_size != BLOCK_SIZE) {
		TC_PRINT("Error: Read failed\n");
		return -TC_FAIL;
	}
	ret = verify_buf((uint16_t *)rx_block, att);
	if (ret < 0) {
		TC_PRINT("Error: Verify failed\n");
		return -TC_FAIL;
	}

	return TC_PASS;
}

int rx_block_read(const struct device *dev_i2s, int att)
{
	return rx_block_read_slab(dev_i2s, att, &rx_mem_slab);
}

int configure_stream(const struct device *dev_i2s, enum i2s_dir dir)
{
	int ret;
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;

	if (dir == I2S_DIR_TX) {
		/* Configure the Transmit port as Master */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER;
	} else if (dir == I2S_DIR_RX) {
		/* Configure the Receive port as Slave */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE
				| I2S_OPT_BIT_CLK_SLAVE;
	} else { /* dir == I2S_DIR_BOTH */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER;
	}

	if (!IS_ENABLED(CONFIG_I2S_TEST_USE_GPIO_LOOPBACK)) {
		i2s_cfg.options |= I2S_OPT_LOOPBACK;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &tx_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S TX stream (%d)\n",
				 ret);
			return -TC_FAIL;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &rx_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S RX stream (%d)\n",
				 ret);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}
