/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>

#define SAMPLE_NO      32
#define TIMEOUT        2000
#define FRAME_CLK_FREQ 8000

#define NUM_RX_BLOCKS 4
#define NUM_TX_BLOCKS 4

#define VAL_L 11
#define VAL_R 22

#define TRANSFER_REPEAT_COUNT 100

#define I2S_DEV_NODE_RX DT_ALIAS(i2s_node0)
#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
#define I2S_DEV_NODE_TX DT_ALIAS(i2s_node1)
#else
#define I2S_DEV_NODE_TX DT_ALIAS(i2s_node0)
#endif

ZTEST_DMEM const struct device *dev_i2s_rx = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
ZTEST_DMEM const struct device *dev_i2s_tx = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_TX);
ZTEST_DMEM const struct device *dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);

#define BLOCK_SIZE (2 * SAMPLE_NO * sizeof(int16_t))

K_MEM_SLAB_DEFINE(rx_mem_slab, BLOCK_SIZE, NUM_RX_BLOCKS, 32);
K_MEM_SLAB_DEFINE(tx_mem_slab, BLOCK_SIZE, NUM_TX_BLOCKS, 32);

void fill_buf(int16_t *tx_block, int16_t val_l, int16_t val_r)
{
	for (int16_t i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = val_l + i;
		tx_block[2 * i + 1] = val_r + i;
	}
}

int verify_buf(int16_t *rx_block, int16_t val_l, int16_t val_r)
{
	int sample_no = SAMPLE_NO;

#if (CONFIG_I2S_TEST_ALLOWED_DATA_DISCARD > 0)
	static ZTEST_DMEM int offset = -1;

	if (offset < 0) {
		do {
			++offset;
			if (offset > CONFIG_I2S_TEST_ALLOWED_DATA_DISCARD) {
				TC_PRINT("Allowed data discard exceeded\n");
				return -TC_FAIL;
			}
		} while ((rx_block[0] != val_l + offset) || (rx_block[1] != val_r + offset));
	}

	val_l += offset;
	val_r += offset;
	sample_no -= offset;
#endif

	for (int16_t i = 0; i < sample_no; i++) {
		if (rx_block[2 * i] != (val_l + i)) {
			TC_PRINT("Error: data_l mismatch at position "
				 "%d, expected %d, actual %d\n",
				 i, (int)(val_l + i), (int)rx_block[2 * i]);
			return -TC_FAIL;
		}
		if (rx_block[2 * i + 1] != (val_r + i)) {
			TC_PRINT("Error: data_r mismatch at position "
				 "%d, expected %d, actual %d\n",
				 i, (int)(val_r + i), (int)rx_block[2 * i + 1]);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

static int tx_block_write_slab(const struct device *i2s_dev, int16_t val_l, int16_t val_r, int err,
			       struct k_mem_slab *slab)
{
	char tx_block[BLOCK_SIZE];
	int ret;

	fill_buf((uint16_t *)tx_block, val_l, val_r);
	ret = i2s_buf_write(i2s_dev, tx_block, BLOCK_SIZE);
	if (ret != err) {
		TC_PRINT("Error: i2s_write failed expected %d, actual %d\n", err, ret);
		return -TC_FAIL;
	}

	return TC_PASS;
}

int tx_block_write(const struct device *i2s_dev, int16_t val_l, int16_t val_r, int err)
{
	return tx_block_write_slab(i2s_dev, val_l, val_r, err, &tx_mem_slab);
}

static int rx_block_read_slab(const struct device *i2s_dev, int16_t val_l, int16_t val_r,
			      struct k_mem_slab *slab)
{
	char rx_block[BLOCK_SIZE];
	size_t rx_size;
	int ret;

	ret = i2s_buf_read(i2s_dev, rx_block, &rx_size);
	if (ret < 0 || rx_size != BLOCK_SIZE) {
		TC_PRINT("Error: Read failed\n");
		return -TC_FAIL;
	}
	ret = verify_buf((uint16_t *)rx_block, val_l, val_r);
	if (ret < 0) {
		TC_PRINT("Error: Verify failed\n");
		return -TC_FAIL;
	}

	return TC_PASS;
}

int rx_block_read(const struct device *i2s_dev, int16_t val_l, int16_t val_r)
{
	return rx_block_read_slab(i2s_dev, val_l, val_r, &rx_mem_slab);
}

int configure_stream(const struct device *i2s_dev, enum i2s_dir dir)
{
	int ret;
	struct i2s_config i2s_cfg = {0};

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;

	if (dir == I2S_DIR_TX) {
		/* Configure the Transmit port as Master */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	} else if (dir == I2S_DIR_RX) {
		/* Configure the Receive port as Slave */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
	} else { /* dir == I2S_DIR_BOTH */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	}

	if (!IS_ENABLED(CONFIG_I2S_TEST_USE_GPIO_LOOPBACK)) {
		i2s_cfg.options |= I2S_OPT_LOOPBACK;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &tx_mem_slab;
		ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S TX stream (%d)\n", ret);
			return -TC_FAIL;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &rx_mem_slab;
		ret = i2s_configure(i2s_dev, I2S_DIR_RX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S RX stream (%d)\n", ret);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

static void *setup(void)
{
	k_thread_access_grant(k_current_get(), &rx_mem_slab, &tx_mem_slab);
	k_object_access_grant(dev_i2s_rx, k_current_get());
	k_object_access_grant(dev_i2s_tx, k_current_get());

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	int ret;

	zassert_not_null(dev_i2s_rx, "RX device not found");
	zassert_true(device_is_ready(dev_i2s_rx), "device %s is not ready", dev_i2s_rx->name);

	zassert_not_null(dev_i2s_tx, "TX device not found");
	zassert_true(device_is_ready(dev_i2s_tx), "device %s is not ready", dev_i2s_tx->name);

	ret = configure_stream(dev_i2s_rx, I2S_DIR_RX);
	zassert_equal(ret, TC_PASS);

	ret = configure_stream(dev_i2s_tx, I2S_DIR_TX);
	zassert_equal(ret, TC_PASS);
}

/** @brief I2S transfer.
 *
 * - START trigger starts both the transmission and reception.
 * - sending / receiving a sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST_USER(i2s_loopback, test_i2s_transfer)
{
	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, VAL_L, VAL_R, 0);
	zassert_equal(ret, TC_PASS);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	for (int i = 0; i < TRANSFER_REPEAT_COUNT; i++) {
		ret = tx_block_write(dev_i2s_tx, VAL_L, VAL_R, 0);
		zassert_equal(ret, TC_PASS);

		ret = rx_block_read(dev_i2s_rx, VAL_L, VAL_R);
		zassert_equal(ret, TC_PASS);
	}

	/* All data written, all but one data block read, flush TX queue
	 * and stop both streams.
	 */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");
}

ZTEST_SUITE(i2s_loopback, NULL, setup, before, NULL, NULL);
