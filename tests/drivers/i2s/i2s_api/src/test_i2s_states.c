/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include "i2s_api_test.h"

/** @brief Verify all failure cases in NOT_READY state.
 *
 * - Sending START, DRAIN, STOP, DROP, PREPARE trigger in NOT_READY state
 *   returns failure.
 * - An attempt to read RX block in NOT_READY state returns failure.
 * - An attempt to write TX block in NOT_READY state returns failure.
 */
ZTEST_USER(i2s_states, test_i2s_state_not_ready_neg)
{
	struct i2s_config i2s_cfg;
	size_t rx_size;
	int ret;
	char rx_buf[BLOCK_SIZE];

	i2s_cfg.frame_clk_freq = 0U;
	i2s_cfg.mem_slab = &rx_mem_slab;

	ret = i2s_configure(dev_i2s_rx, I2S_DIR_RX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S RX stream");

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DROP);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO);

	ret = i2s_buf_read(dev_i2s_rx, rx_buf, &rx_size);
	zassert_equal(ret, -EIO);

	i2s_cfg.frame_clk_freq = 0U;
	i2s_cfg.mem_slab = &tx_mem_slab;

	ret = i2s_configure(dev_i2s_tx, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, 0, "Failed to configure I2S TX stream");

	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO);

	ret = tx_block_write(dev_i2s_tx, 2, -EIO);
	zassert_equal(ret, TC_PASS);
}

/** @brief Verify all failure cases in READY state.
 *
 * - Sending DRAIN, STOP, PREPARE trigger in READY state returns failure.
 */
ZTEST_USER(i2s_states, test_i2s_state_ready_neg)
{
	int ret;

	/* Configure RX stream changing its state to READY */
	ret = configure_stream(dev_i2s_rx, I2S_DIR_RX);
	zassert_equal(ret, TC_PASS);

	/* Send RX stream triggers */

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO);

	/* Configure TX stream changing its state to READY */
	ret = configure_stream(dev_i2s_tx, I2S_DIR_TX);
	zassert_equal(ret, TC_PASS);

	/* Send TX stream triggers */

	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);

	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO);
}

#define TEST_I2S_STATE_RUNNING_NEG_REPEAT_COUNT  5

/** @brief Verify all failure cases in RUNNING state.
 *
 * - Sending START, PREPARE trigger in RUNNING state returns failure.
 */
ZTEST_USER(i2s_states, test_i2s_state_running_neg)
{
	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		TC_PRINT("RX/TX transfer requires use of I2S_DIR_BOTH.\n");
		ztest_test_skip();
		return;
	}

	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s_tx, 0, 0);
	zassert_equal(ret, TC_PASS);

	/* Start reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	for (int i = 0; i < TEST_I2S_STATE_RUNNING_NEG_REPEAT_COUNT; i++) {
		ret = tx_block_write(dev_i2s_tx, 0, 0);
		zassert_equal(ret, TC_PASS);

		ret = rx_block_read(dev_i2s_rx, 0);
		zassert_equal(ret, TC_PASS);

		/* Send invalid triggers, expect failure */
		ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
		zassert_equal(ret, -EIO);
		ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
		zassert_equal(ret, -EIO);
		ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
		zassert_equal(ret, -EIO);
		ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
		zassert_equal(ret, -EIO);
	}

	/* All data written, flush TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	/* All but one data block read, stop reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	ret = rx_block_read(dev_i2s_rx, 0);
	zassert_equal(ret, TC_PASS);
}

/** @brief Verify all failure cases in STOPPING state.
 *
 * - Sending START, STOP, DRAIN, PREPARE trigger in STOPPING state returns
 *   failure.
 */
ZTEST_USER(i2s_states, test_i2s_state_stopping_neg)
{
	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		TC_PRINT("RX/TX transfer requires use of I2S_DIR_BOTH.\n");
		ztest_test_skip();
		return;
	}

	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s_tx, 0, 0);
	zassert_equal(ret, TC_PASS);

	/* Start reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	ret = tx_block_write(dev_i2s_tx, 0, 0);
	zassert_equal(ret, TC_PASS);

	ret = rx_block_read(dev_i2s_rx, 0);
	zassert_equal(ret, TC_PASS);

	/* All data written, flush TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO);

	/* All but one data block read, stop reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, -EIO);

	ret = rx_block_read(dev_i2s_rx, 0);
	zassert_equal(ret, TC_PASS);

	/* This is incase the RX channel is stuck in STOPPING state.
	 * Clear out the state before running the next test.
	 */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DROP);
	zassert_equal(ret, 0, "RX DROP trigger failed");
}

/** @brief Verify all failure cases in ERROR state.
 *
 * - Sending START, STOP, DRAIN trigger in ERROR state returns failure.
 */
ZTEST_USER(i2s_states, test_i2s_state_error_neg)
{
	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		TC_PRINT("RX/TX transfer requires use of I2S_DIR_BOTH.\n");
		ztest_test_skip();
		return;
	}

	size_t rx_size;
	int ret;
	char rx_buf[BLOCK_SIZE];

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s_tx, 0, 0);
	zassert_equal(ret, TC_PASS);

	/* Start reception */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");

	/* Start transmission */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");

	for (int i = 0; i < NUM_RX_BLOCKS; i++) {
		ret = tx_block_write(dev_i2s_tx, 0, 0);
		zassert_equal(ret, TC_PASS);
	}

	/* Wait for transmission to finish */
	k_sleep(K_MSEC(200));

	/* Read one data block, expect success even if RX queue is already in
	 * the error state.
	 */
	ret = rx_block_read(dev_i2s_rx, 0);
	zassert_equal(ret, TC_PASS);

	/* Attempt to read more data blocks than are available in the RX queue */
	for (int i = 0; i < NUM_RX_BLOCKS; i++) {
		ret = i2s_buf_read(dev_i2s_rx, rx_buf, &rx_size);
		if (ret != 0) {
			break;
		}
	}
	zassert_equal(ret, -EIO, "RX overrun error not detected");

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);

	/* Recover from ERROR state */
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, 0, "RX PREPARE trigger failed");

	/* Write one more TX data block, expect an error */
	ret = tx_block_write(dev_i2s_tx, 2, -EIO);
	zassert_equal(ret, TC_PASS);

	/* Send invalid triggers, expect failure */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, -EIO);
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, -EIO);

	/* Recover from ERROR state */
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, 0, "TX PREPARE trigger failed");

	/* Transmit and receive one more data block */
	ret = tx_block_write(dev_i2s_tx, 0, 0);
	zassert_equal(ret, TC_PASS);
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed");
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed");
	ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "TX DRAIN trigger failed");
	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");
	ret = rx_block_read(dev_i2s_rx, 0);
	zassert_equal(ret, TC_PASS);

	k_sleep(K_MSEC(200));
}
