/*
 * Copyright (c) 2017 comsuisse AG
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include "i2s_api_test.h"

/* The test cases here are copied from test_i2s_states.c and adapted for use
 * on devices that cannot independently start and stop the RX and TX streams
 * and require the use of the I2S_DIR_BOTH value for RX/TX transfers.
 */

static ZTEST_DMEM const struct device *dev_i2s;
static ZTEST_DMEM bool dir_both_supported;

void test_i2s_dir_both_transfer_configure_1(void)
{
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME_RX);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME_RX " not found");

	ret = configure_stream(dev_i2s, I2S_DIR_BOTH);
	zassert_equal(ret, TC_PASS);

	/* Check if the tested driver supports the I2S_DIR_BOTH value.
	 * Use the DROP trigger for this, as in the current state of the driver
	 * (READY, both TX and RX queues empty) it is actually a no-op.
	 */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	dir_both_supported = (ret == 0);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		zassert_true(dir_both_supported,
			     "I2S_DIR_BOTH value is supposed to be supported.");
	}
}

#define TEST_I2S_STATE_RUNNING_NEG_REPEAT_COUNT  5

/** @brief Verify all failure cases in RUNNING state.
 *
 * - Sending START, PREPARE trigger in RUNNING state returns failure.
 */
void test_i2s_dir_both_state_running_neg(void)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	for (int i = 0; i < TEST_I2S_STATE_RUNNING_NEG_REPEAT_COUNT; i++) {
		ret = tx_block_write(dev_i2s, 0, 0);
		zassert_equal(ret, TC_PASS);

		ret = rx_block_read(dev_i2s, 0);
		zassert_equal(ret, TC_PASS);

		/* Send invalid triggers, expect failure */
		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
		zassert_equal(ret, -EIO);
		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_PREPARE);
		zassert_equal(ret, -EIO);
	}

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS);
}

/** @brief Verify all failure cases in STOPPING state.
 *
 * - Sending START, STOP, DRAIN, PREPARE trigger in STOPPING state returns
 *   failure.
 */
void test_i2s_dir_both_state_stopping_neg(void)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS);

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS);

	/* All data written, all but one data block read, flush TX queue and
	 * stop both streams.
	 */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO);

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS);

	/* This is incase the RX channel is stuck in STOPPING state.
	 * Clear out the state before running the next test.
	 */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	zassert_equal(ret, 0, "RX/TX DROP trigger failed");
}

/** @brief Verify all failure cases in ERROR state.
 *
 * - Sending START, STOP, DRAIN trigger in ERROR state returns failure.
 */
void test_i2s_dir_both_state_error_neg(void)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	size_t rx_size;
	int ret;
	char rx_buf[BLOCK_SIZE];

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	for (int i = 0; i < NUM_RX_BLOCKS; i++) {
		ret = tx_block_write(dev_i2s, 0, 0);
		zassert_equal(ret, TC_PASS);
	}

	/* Wait for transmission to finish */
	k_sleep(K_MSEC(200));

	/* Read one data block, expect success even if RX queue is already in
	 * the error state.
	 */
	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS);

	/* Attempt to read more data blocks than are available in the RX queue */
	for (int i = 0; i < NUM_RX_BLOCKS; i++) {
		ret = i2s_buf_read(dev_i2s, rx_buf, &rx_size);
		if (ret != 0) {
			break;
		}
	}
	zassert_equal(ret, -EIO, "RX overrun error not detected");

	/* Write one more TX data block, expect an error */
	ret = tx_block_write(dev_i2s, 2, -EIO);
	zassert_equal(ret, TC_PASS);

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);

	/* Recover from ERROR state */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, 0, "RX/TX PREPARE trigger failed");

	/* Transmit and receive one more data block */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed");
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");
	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS);

	k_sleep(K_MSEC(200));
}
