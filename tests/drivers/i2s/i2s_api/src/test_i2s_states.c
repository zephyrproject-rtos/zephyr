/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_i2s_api
 * @{
 * @defgroup t_i2s_states test_i2s_states
 * @brief TestPurpose: verify handling of API calls in all defined interface
 *        states.
 * @}
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/i2s.h>
#include "i2s_api_test.h"

#define NUM_RX_BLOCKS 4
#define NUM_TX_BLOCKS 4

K_MEM_SLAB_DEFINE(rx_1_mem_slab, BLOCK_SIZE, NUM_RX_BLOCKS, 32);
K_MEM_SLAB_DEFINE(tx_1_mem_slab, BLOCK_SIZE, NUM_TX_BLOCKS, 32);

static int tx_block_write(struct device *dev_i2s, int att, int err)
{
	return tx_block_write_slab(dev_i2s, att, err, &tx_1_mem_slab);
}
static int rx_block_read(struct device *dev_i2s, int att)
{
	return rx_block_read_slab(dev_i2s, att, &rx_1_mem_slab);
}

/** Configure I2S TX transfer. */
void test_i2s_tx_transfer_configure_1(void)
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
	i2s_cfg.mem_slab = &tx_1_mem_slab;
	i2s_cfg.timeout = TIMEOUT;
	i2s_cfg.options = I2S_OPT_LOOPBACK;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S TX stream");
}

/** Configure I2S RX transfer. */
void test_i2s_rx_transfer_configure_1(void)
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
	i2s_cfg.mem_slab = &rx_1_mem_slab;
	i2s_cfg.timeout = TIMEOUT;
	i2s_cfg.options = I2S_OPT_LOOPBACK;

	ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S RX stream");
}

/** @brief Verify all failure cases in NOT_READY state.
 *
 * - Sending START, DRAIN, STOP, DROP, PREPARE trigger in NOT_READY state
 *   returns failure.
 * - An attempt to read RX block in NOT_READY state returns failure.
 * - An attempt to write TX block in NOT_READY state returns failure.
 */
void test_i2s_state_not_ready_neg(void)
{
	struct device *dev_i2s;
	struct i2s_config i2s_cfg;
	size_t rx_size;
	int ret;
	char rx_buf[BLOCK_SIZE];

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	i2s_cfg.frame_clk_freq = 0U;
	i2s_cfg.mem_slab = &rx_1_mem_slab;

	ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S RX stream");

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_DROP);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_buf_read(dev_i2s, rx_buf, &rx_size);
	zassert_equal(ret, -EIO, NULL);

	i2s_cfg.frame_clk_freq = 0U;
	i2s_cfg.mem_slab = &tx_1_mem_slab;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S TX stream");

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DROP);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO, NULL);

	ret = tx_block_write(dev_i2s, 2, -EIO);
	zassert_equal(ret, TC_PASS, NULL);
}

/** @brief Verify all failure cases in READY state.
 *
 * - Sending DRAIN, STOP, PREPARE trigger in READY state returns failure.
 */
void test_i2s_state_ready_neg(void)
{
	struct device *dev_i2s;
	struct i2s_config i2s_cfg;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	/* Configure RX stream changing its state to READY */

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
	i2s_cfg.frame_clk_freq = 8000U;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.mem_slab = &rx_1_mem_slab;
	i2s_cfg.timeout = TIMEOUT;

	ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S RX stream");

	/* Send RX stream triggers */

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO, NULL);

	/* Configure TX stream changing its state to READY */

	i2s_cfg.options = I2S_OPT_LOOPBACK;
	i2s_cfg.mem_slab = &tx_1_mem_slab;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S RX stream");

	/* Send TX stream triggers */

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO, NULL);
}

#define TEST_I2S_STATE_RUNNING_NEG_REPEAT_COUNT  5

/** @brief Verify all failure cases in RUNNING state.
 *
 * - Sending START, PREPARE trigger in RUNNING state returns failure.
 */
void test_i2s_state_running_neg(void)
{
	struct device *dev_i2s;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);

	/* Start reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	for (int i = 0; i < TEST_I2S_STATE_RUNNING_NEG_REPEAT_COUNT; i++) {
		ret = tx_block_write(dev_i2s, 0, 0);
		zassert_equal(ret, TC_PASS, NULL);

		ret = rx_block_read(dev_i2s, 0);
		zassert_equal(ret, TC_PASS, NULL);

		/* Send invalid triggers, expect failure */
		ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
		zassert_equal(ret, -EIO, NULL);
		ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
		zassert_equal(ret, -EIO, NULL);
		ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
		zassert_equal(ret, -EIO, NULL);
		ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
		zassert_equal(ret, -EIO, NULL);
	}

	/* All data written, flush TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	/* All but one data block read, stop reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);
}

/** @brief Verify all failure cases in STOPPING state.
 *
 * - Sending START, STOP, DRAIN, PREPARE trigger in STOPPING state returns
 *   failure.
 */
void test_i2s_state_stopping_neg(void)
{
	struct device *dev_i2s;
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);

	/* Start reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);

	/* All data written, flush TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO, NULL);

	/* All but one data block read, stop reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO, NULL);

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);
}

#define TEST_I2S_STATE_ERROR_NEG_PAUSE_LENGTH_US  200

/** @brief Verify all failure cases in ERROR state.
 *
 * - Sending START, STOP, DRAIN trigger in ERROR state returns failure.
 */
void test_i2s_state_error_neg(void)
{
	struct device *dev_i2s;
	size_t rx_size;
	int ret;
	char rx_buf[BLOCK_SIZE];

	dev_i2s = device_get_binding(I2S_DEV_NAME);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME " not found");

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);

	/* Start reception */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	for (int i = 0; i < NUM_RX_BLOCKS; i++) {
		ret = tx_block_write(dev_i2s, 0, 0);
		zassert_equal(ret, TC_PASS, NULL);
	}

	/* Wait for transmission to finish */
	k_sleep(TEST_I2S_STATE_ERROR_NEG_PAUSE_LENGTH_US);

	/* Read all available data blocks in RX queue */
	for (int i = 0; i < NUM_RX_BLOCKS; i++) {
		ret = rx_block_read(dev_i2s, 0);
		zassert_equal(ret, TC_PASS, NULL);
	}

	/* Attempt to read one more data block, expect an error */
	ret = i2s_buf_read(dev_i2s, rx_buf, &rx_size);
	zassert_equal(ret, -EIO, "RX overrun error not detected");

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO, NULL);

	/* Recover from ERROR state */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, 0, "RX PREPARE trigger failed");

	/* Write one more TX data block, expect an error */
	ret = tx_block_write(dev_i2s, 2, -EIO);
	zassert_equal(ret, TC_PASS, NULL);

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO, NULL);

	/* Recover from ERROR state */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, 0, "TX PREPARE trigger failed");

	/* Transmit and receive one more data block */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");
	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);

	k_sleep(K_MSEC(200));
}
