/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/i2s.h>
#include "i2s_api_test.h"

/* The data_l represent a sine wave */
s16_t data_l[SAMPLE_NO] = {
	  6392,  12539,  18204,  23169,  27244,  30272,  32137,  32767,  32137,
	 30272,  27244,  23169,  18204,  12539,   6392,      0,  -6393, -12540,
	-18205, -23170, -27245, -30273, -32138, -32767, -32138, -30273, -27245,
	-23170, -18205, -12540,  -6393,     -1,
};

/* The data_r represent a sine wave with double the frequency of data_l */
s16_t data_r[SAMPLE_NO] = {
	 12539,  23169,  30272,  32767,  30272,  23169,  12539,      0, -12540,
	-23170, -30273, -32767, -30273, -23170, -12540,     -1,  12539,  23169,
	 30272,  32767,  30272,  23169,  12539,      0, -12540, -23170, -30273,
	-32767, -30273, -23170, -12540,     -1,
};

static void fill_buf(s16_t *tx_block, int att)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = data_l[i] >> att;
		tx_block[2 * i + 1] = data_r[i] >> att;
	}
}

static int verify_buf(s16_t *rx_block, int att)
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

void fill_buf_const(s16_t *tx_block, s16_t val_l, s16_t val_r)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = val_l;
		tx_block[2 * i + 1] = val_r;
	}
}

int verify_buf_const(s16_t *rx_block, s16_t val_l, s16_t val_r)
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

int tx_block_write_slab(struct device *dev_i2s, int att, int err,
			struct k_mem_slab *slab)
{
	char tx_block[BLOCK_SIZE];
	int ret;

	fill_buf((u16_t *)tx_block, att);
	ret = i2s_buf_write(dev_i2s, tx_block, BLOCK_SIZE);
	if (ret != err) {
		TC_PRINT("Error: i2s_write failed expected %d, actual %d\n",
			 err, ret);
		return -TC_FAIL;
	}

	return TC_PASS;
}

int rx_block_read_slab(struct device *dev_i2s, int att,
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
	ret = verify_buf((u16_t *)rx_block, att);
	if (ret < 0) {
		TC_PRINT("Error: Verify failed\n");
		return -TC_FAIL;
	}

	return TC_PASS;
}
