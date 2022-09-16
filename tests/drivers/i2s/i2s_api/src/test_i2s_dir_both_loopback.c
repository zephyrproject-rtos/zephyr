/*
 * Copyright (c) 2017 comsuisse AG
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/drivers/i2s.h>
#include "i2s_api_test.h"

/* The test cases here are copied from test_i2s_loopback.c and adapted for use
 * on devices that cannot independently start and stop the RX and TX streams
 * and require the use of the I2S_DIR_BOTH value for RX/TX transfers.
 */

static ZTEST_DMEM const struct device *dev_i2s;
static ZTEST_DMEM bool dir_both_supported;

void test_i2s_dir_both_transfer_configure_0(void)
{
	int ret;

	dev_i2s = device_get_binding(I2S_DEV_NAME_RX);
	zassert_not_null(dev_i2s, "device " I2S_DEV_NAME_RX " not found");

	ret = configure_stream(dev_i2s, I2S_DIR_BOTH);
	zassert_equal(ret, TC_PASS, NULL);

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

/** @brief Short I2S transfer.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
void test_i2s_dir_both_transfer_short(void)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d->OK\n", 1);

	ret = tx_block_write(dev_i2s, 1, 0);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d->OK\n", 2);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d<-OK\n", 1);

	ret = tx_block_write(dev_i2s, 2, 0);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d->OK\n", 3);

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = rx_block_read(dev_i2s, 1);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d<-OK\n", 2);

	ret = rx_block_read(dev_i2s, 2);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d<-OK\n", 3);

	/* TODO: Verify the interface is in READY state when i2s_state_get
	 * function is available.
	 */
}

#define TEST_I2S_TRANSFER_LONG_REPEAT_COUNT  100

/** @brief Long I2S transfer.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a long sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
void test_i2s_dir_both_transfer_long(void)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	for (int i = 0; i < TEST_I2S_TRANSFER_LONG_REPEAT_COUNT; i++) {
		ret = tx_block_write(dev_i2s, 0, 0);
		zassert_equal(ret, TC_PASS, NULL);

		ret = rx_block_read(dev_i2s, 0);
		zassert_equal(ret, TC_PASS, NULL);
	}

	/* All data written, all but one data block read, flush TX queue
	 * and stop both streams.
	 */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);

	/* TODO: Verify the interface is in READY state when i2s_state_get
	 * function is available.
	 */
}

/** @brief Re-start I2S transfer.
 *
 * - STOP trigger stops transfer / reception at the end of the current block,
 *   consecutive START trigger restarts transfer / reception with the next data
 *   block.
 */
void test_i2s_dir_both_transfer_restart(void)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d->OK\n", 1);

	ret = tx_block_write(dev_i2s, 1, 0);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d->OK\n", 2);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX/TX STOP trigger failed");

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d<-OK\n", 1);

	TC_PRINT("Stop transmission\n");

	/* Keep interface inactive */
	k_sleep(K_MSEC(1000));

	TC_PRINT("Start transmission\n");

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 2, 0);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d->OK\n", 3);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = rx_block_read(dev_i2s, 1);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d<-OK\n", 2);

	ret = rx_block_read(dev_i2s, 2);
	zassert_equal(ret, TC_PASS, NULL);
	TC_PRINT("%d<-OK\n", 3);
}

/** @brief RX buffer overrun.
 *
 * - In case of RX buffer overrun it is possible to read out RX data blocks
 *   that are stored in the RX queue.
 * - Reading from an empty RX queue when the RX buffer overrun occurred results
 *   in an error.
 * - Sending PREPARE trigger after the RX buffer overrun occurred changes
 *   the interface state to READY.
 */
void test_i2s_dir_both_transfer_rx_overrun(void)
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
	zassert_equal(ret, TC_PASS, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	for (int i = 0; i < NUM_RX_BLOCKS; i++) {
		ret = tx_block_write(dev_i2s, 0, 0);
		zassert_equal(ret, TC_PASS, NULL);
	}

	/* All data written, flush TX queue and stop the transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	/* Wait for transmission to finish */
	k_sleep(K_MSEC(200));

	/* Read one data block, expect success even if RX queue is already in
	 * the error state.
	 */
	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);

	/* Attempt to read more data blocks than are available in the RX queue */
	for (int i = 0; i < NUM_RX_BLOCKS; i++) {
		ret = i2s_buf_read(dev_i2s, rx_buf, &rx_size);
		if (ret != 0) {
			break;
		}
	}
	zassert_equal(ret, -EIO, "RX overrun error not detected");

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, 0, "RX PREPARE trigger failed");

	/* Transmit and receive one more data block */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");
	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);

	k_sleep(K_MSEC(200));
}

/** @brief TX buffer underrun.
 *
 * - An attempt to write to the TX queue when TX buffer underrun has occurred
 *   results in an error.
 * - Sending PREPARE trigger after the TX buffer underrun occurred changes
 *   the interface state to READY.
 */
void test_i2s_dir_both_transfer_tx_underrun(void)
{
	if (!dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		ztest_test_skip();
		return;
	}

	int ret;

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s, 0, 0);
	zassert_equal(ret, TC_PASS, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	ret = rx_block_read(dev_i2s, 0);
	zassert_equal(ret, TC_PASS, NULL);

	k_sleep(K_MSEC(200));

	/* Write one more TX data block, expect an error */
	ret = tx_block_write(dev_i2s, 2, -EIO);
	zassert_equal(ret, TC_PASS, NULL);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_PREPARE);
	zassert_equal(ret, 0, "RX/TX PREPARE trigger failed");

	k_sleep(K_MSEC(200));

	/* Transmit and receive two more data blocks */
	ret = tx_block_write(dev_i2s, 1, 0);
	zassert_equal(ret, TC_PASS, NULL);
	ret = tx_block_write(dev_i2s, 1, 0);
	zassert_equal(ret, TC_PASS, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");
	ret = rx_block_read(dev_i2s, 1);
	zassert_equal(ret, TC_PASS, NULL);
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");
	ret = rx_block_read(dev_i2s, 1);
	zassert_equal(ret, TC_PASS, NULL);

	k_sleep(K_MSEC(200));
}
