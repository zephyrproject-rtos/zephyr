/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>

#define I2S_DEV_NODE_RX DT_ALIAS(i2s_node0)
#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
#define I2S_DEV_NODE_TX DT_ALIAS(i2s_node1)
#else
#define I2S_DEV_NODE_TX DT_ALIAS(i2s_node0)
#endif

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

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

/*
 * NUM_BLOCKS is the number of blocks used by the test. Some of the drivers,
 * e.g. i2s_mcux_flexcomm, permanently keep ownership of a few RX buffers. Add a few more
 * RX blocks to satisfy this requirement
 */

char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, rx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(rx_0_mem_slab, _k_mem_slab_buf_rx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

static const struct device *dev_i2s_rx;
static const struct device *dev_i2s_tx;
static const struct device *dev_i2s_rxtx;
static bool dir_both_supported;

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

#define TIMEOUT          2000
#define FRAME_CLK_FREQ   44000

static int configure_stream(const struct device *dev_i2s, enum i2s_dir dir)
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
		i2s_cfg.mem_slab = &tx_0_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S TX stream (%d)\n",
				 ret);
			return -TC_FAIL;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &rx_0_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S RX stream (%d)\n",
				 ret);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}


/** @brief Short I2S transfer.
 *
 * - TX stream START trigger starts transmission.
 * - RX stream START trigger starts reception.
 * - sending / receiving a short sequence of data returns success.
 * - TX stream DRAIN trigger empties the transmit queue.
 * - RX stream STOP trigger stops reception.
 */
ZTEST(drivers_i2s_speed, test_i2s_transfer_short)
{
	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		TC_PRINT("RX/TX transfer requires use of I2S_DIR_BOTH.\n");
		ztest_test_skip();
		return;
	}

	void *rx_block[3];
	void *tx_block;
	size_t rx_size;
	int ret;

	/* Prefill TX queue */
	for (int i = 0; i < 3; i++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint16_t *)tx_block, i);

		ret = i2s_write(dev_i2s_tx, tx_block, BLOCK_SIZE);
		zassert_equal(ret, 0);

		TC_PRINT("%d->OK\n", i);
	}

	/* Start reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	/* All data written, drain TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s_rx, &rx_block[0], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	ret = i2s_read(dev_i2s_rx, &rx_block[1], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	/* All but one data block read, stop reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	ret = i2s_read(dev_i2s_rx, &rx_block[2], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	/* Verify received data */
	ret = verify_buf((uint16_t *)rx_block[0], 0);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, &rx_block[0]);
	TC_PRINT("%d<-OK\n", 1);

	ret = verify_buf((uint16_t *)rx_block[1], 1);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, &rx_block[1]);
	TC_PRINT("%d<-OK\n", 2);

	ret = verify_buf((uint16_t *)rx_block[2], 2);
	zassert_equal(ret, 0);
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
ZTEST(drivers_i2s_speed, test_i2s_transfer_long)
{
	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		TC_PRINT("RX/TX transfer requires use of I2S_DIR_BOTH.\n");
		ztest_test_skip();
		return;
	}

	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int tx_idx;
	int rx_idx = 0;
	int num_verified;
	int ret;

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx],
				       K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint16_t *)tx_block[tx_idx], tx_idx % 3);
	}

	tx_idx = 0;

	/* Prefill TX queue */
	ret = i2s_write(dev_i2s_tx, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_write(dev_i2s_tx, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	/* Start reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	for (; tx_idx < NUM_BLOCKS; ) {
		ret = i2s_write(dev_i2s_tx, tx_block[tx_idx++], BLOCK_SIZE);
		zassert_equal(ret, 0);

		ret = i2s_read(dev_i2s_rx, &rx_block[rx_idx++], &rx_size);
		zassert_equal(ret, 0);
		zassert_equal(rx_size, BLOCK_SIZE);
	}

	/* All data written, flush TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s_rx, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	/* All but one data block read, stop reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	ret = i2s_read(dev_i2s_rx, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

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


/** @brief Short I2S transfer using I2S_DIR_BOTH.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(drivers_i2s_speed_both_rxtx, test_i2s_dir_both_transfer_short)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	void *rx_block[3];
	void *tx_block;
	size_t rx_size;
	int ret;

	/* Prefill TX queue */
	for (int i = 0; i < 3; i++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint16_t *)tx_block, i);

		ret = i2s_write(dev_i2s_rxtx, tx_block, BLOCK_SIZE);
		zassert_equal(ret, 0);

		TC_PRINT("%d->OK\n", i);
	}

	ret = i2s_trigger(dev_i2s_rxtx, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s_rxtx, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s_rxtx, &rx_block[0], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	ret = i2s_read(dev_i2s_rxtx, &rx_block[1], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	ret = i2s_read(dev_i2s_rxtx, &rx_block[2], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	/* Verify received data */
	ret = verify_buf((uint16_t *)rx_block[0], 0);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, &rx_block[0]);
	TC_PRINT("%d<-OK\n", 1);

	ret = verify_buf((uint16_t *)rx_block[1], 1);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, &rx_block[1]);
	TC_PRINT("%d<-OK\n", 2);

	ret = verify_buf((uint16_t *)rx_block[2], 2);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, &rx_block[2]);
	TC_PRINT("%d<-OK\n", 3);
}

/** @brief Long I2S transfer using I2S_DIR_BOTH.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a long sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(drivers_i2s_speed_both_rxtx, test_i2s_dir_both_transfer_long)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int tx_idx;
	int rx_idx = 0;
	int num_verified;
	int ret;

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx],
				       K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint16_t *)tx_block[tx_idx], tx_idx % 3);
	}

	tx_idx = 0;

	/* Prefill TX queue */
	ret = i2s_write(dev_i2s_rxtx, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_write(dev_i2s_rxtx, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_trigger(dev_i2s_rxtx, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	for (; tx_idx < NUM_BLOCKS; ) {
		ret = i2s_write(dev_i2s_rxtx, tx_block[tx_idx++], BLOCK_SIZE);
		zassert_equal(ret, 0);

		ret = i2s_read(dev_i2s_rxtx, &rx_block[rx_idx++], &rx_size);
		zassert_equal(ret, 0);
		zassert_equal(rx_size, BLOCK_SIZE);
	}

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s_rxtx, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s_rxtx, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	ret = i2s_read(dev_i2s_rxtx, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

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

static void *test_i2s_speed_configure(void)
{
	/* Configure I2S TX transfer. */
	int ret;

	dev_i2s_tx = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_TX);
	zassert_not_null(dev_i2s_tx, "transfer device not found");
	zassert(device_is_ready(dev_i2s_tx), "transfer device not ready");

	ret = configure_stream(dev_i2s_tx, I2S_DIR_TX);
	zassert_equal(ret, TC_PASS);

	/* Configure I2S RX transfer. */
	dev_i2s_rx = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
	zassert_not_null(dev_i2s_rx, "receive device not found");
	zassert(device_is_ready(dev_i2s_rx), "receive device not ready");

	ret = configure_stream(dev_i2s_rx, I2S_DIR_RX);
	zassert_equal(ret, TC_PASS);

	return 0;
}

static void *test_i2s_speed_rxtx_configure(void)
{
	int ret;

	/* Configure I2S Dir Both transfer. */
	dev_i2s_rxtx = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
	zassert_not_null(dev_i2s_rxtx, "receive device not found");
	zassert(device_is_ready(dev_i2s_rxtx), "receive device not ready");

	ret = configure_stream(dev_i2s_rxtx, I2S_DIR_BOTH);
	zassert_equal(ret, TC_PASS);

	/* Check if the tested driver supports the I2S_DIR_BOTH value.
	 * Use the DROP trigger for this, as in the current state of the driver
	 * (READY, both TX and RX queues empty) it is actually a no-op.
	 */
	ret = i2s_trigger(dev_i2s_rxtx, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	dir_both_supported = (ret == 0);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		zassert_true(dir_both_supported,
			     "I2S_DIR_BOTH value is supposed to be supported.");
	}

	return 0;
}


ZTEST_SUITE(drivers_i2s_speed, NULL, test_i2s_speed_configure, NULL, NULL, NULL);
ZTEST_SUITE(drivers_i2s_speed_both_rxtx, NULL, test_i2s_speed_rxtx_configure, NULL, NULL, NULL);
