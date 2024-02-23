/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

#include <zephyr/ztest.h>

static K_SEM_DEFINE(g_mbox_data_rx_sem, 0, 1);

static uint32_t g_mbox_received_data;
static uint32_t g_mbox_expected_data;
static uint32_t g_mbox_received_channel;
static uint32_t g_mbox_expected_channel;

static bool g_received_size_error;
static size_t g_received_size;
static int g_max_transfer_size_bytes;

#define CHANNELS_TO_TEST 4
#define TX_CHANNEL_INDEX 0
#define RX_CHANNEL_INDEX 1

static const struct mbox_channel channels[CHANNELS_TO_TEST][2] = {
	{
		MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), tx0),
		MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), rx0),
	},
	{
		MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), tx1),
		MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), rx1),
	},
	{
		MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), tx2),
		MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), rx2),
	},
	{
		MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), tx3),
		MBOX_DT_CHANNEL_GET(DT_PATH(mbox_consumer), rx3),
	},
};

static uint32_t current_channel_index;

static void callback(const struct device *dev, uint32_t channel, void *user_data,
		     struct mbox_msg *data)
{
	/* Handle the case if received invalid size */
	if (data->size > sizeof(g_mbox_received_data)) {
		g_received_size_error = true;
		g_received_size = data->size;
	} else {
		memcpy(&g_mbox_received_data, data->data, data->size);
	}

	g_mbox_received_channel = channel;

	k_sem_give(&g_mbox_data_rx_sem);
}

static void mbox_data_tests_before(void *f)
{
	zassert_false(current_channel_index >= CHANNELS_TO_TEST, "Channel to test is out of range");

	const struct mbox_channel *tx_channel = &channels[current_channel_index][TX_CHANNEL_INDEX];
	const struct mbox_channel *rx_channel = &channels[current_channel_index][RX_CHANNEL_INDEX];
	int ret_val = 0;

	g_max_transfer_size_bytes = mbox_mtu_get(tx_channel->dev);
	/* Test currently supports only transfer size up to 4 bytes */
	if ((g_max_transfer_size_bytes < 0) || (g_max_transfer_size_bytes > 4)) {
		printk("mbox_mtu_get() error\n");
		zassert_false(1, "mbox invalid maximum transfer unit: %d",
			      g_max_transfer_size_bytes);
	}

	ret_val = mbox_register_callback(rx_channel, callback, NULL);
	zassert_false(ret_val != 0, "mbox failed to register callback. ret_val", ret_val);

	ret_val = mbox_set_enabled(rx_channel, 1);
	zassert_false(ret_val != 0, "mbox failed to enable mbox. ret_val: %d", ret_val);
}

static void mbox_data_tests_after(void *f)
{
	zassert_false(current_channel_index >= CHANNELS_TO_TEST, "Channel to test is out of range");

	const struct mbox_channel *rx_channel = &channels[current_channel_index][RX_CHANNEL_INDEX];

	/* Disable channel after test end */
	int ret_val = mbox_set_enabled(rx_channel, 0);

	zassert_false(ret_val != 0, "mbox failed to disable mbox. ret_val: %d", ret_val);

	/* Increment current channel index to its prepared for next test */
	current_channel_index++;
}

static void mbox_test(const uint32_t data)
{
	struct mbox_msg msg = {0};
	uint32_t test_data = data;
	int test_count = 0;
	int ret_val = 0;

	while (test_count < 100) {
		const struct mbox_channel *tx_channel = &channels[current_channel_index][TX_CHANNEL_INDEX];

		/* Main core prepare test data */
		msg.data = &test_data;
		msg.size = g_max_transfer_size_bytes;

		/* Main core send test data */
		ret_val = mbox_send(tx_channel, &msg);
		zassert_false(ret_val < 0, "mbox failed to send. ret_val: %d", ret_val);

		/* Expect next received data will be incremented by one.
		 * And based on Maximum Transfer Unit determine expected data.
		 * Currently supported MTU's are 1, 2, 3, and 4 bytes.
		 */
		g_mbox_expected_data = test_data & ~(0xFFFFFFFF << (g_max_transfer_size_bytes * 8));
		g_mbox_expected_data++;

		k_sem_take(&g_mbox_data_rx_sem, K_FOREVER);

		if (g_received_size_error) {
			zassert_false(1, "mbox received invalid size in callback: %d",
				      g_received_size);
		}

		test_data = g_mbox_received_data;

		/* Main core check received data */
		zassert_equal(g_mbox_expected_data, test_data,
			      "Received test_data does not match!: Expected: %08X, Got: %08X",
			      g_mbox_expected_data, test_data);

		/* Expect reception of data on current RX channel */
		g_mbox_expected_channel = channels[current_channel_index][RX_CHANNEL_INDEX].id;
		zassert_equal(g_mbox_expected_channel, g_mbox_received_channel,
			      "Received channel does not match!: Expected: %d, Got: %d",
			      g_mbox_expected_channel, g_mbox_received_channel);

		/* Increment for next send */
		test_data++;
		test_count++;
	}
}

/**
 * @brief MBOX Data transfer by ping pong for first set of channels
 *
 * This test verifies that the data transfer via MBOX.
 * Main core will transfer test data to remote core.
 * Remote core will increment data by one and transfer it back to Main core.
 * Main core will check that data it sent to remote core was incremented by one.
 * Main core will again increment test data by one, send it to remote core and repeat 100 times.
 */
ZTEST(mbox_data_tests, test_ping_pong_1)
{
	mbox_test(0xADADADAD);
}

/**
 * @brief MBOX Data transfer by ping pong for second set of channels
 *
 * Description same as for test_ping_pong_1
 *
 */
ZTEST(mbox_data_tests, test_ping_pong_2)
{
	mbox_test(0xDADADADA);
}

/**
 * @brief MBOX Data transfer by ping pong for third set of channels
 *
 * Description same as for test_ping_pong_1
 *
 */
ZTEST(mbox_data_tests, test_ping_pong_3)
{
	mbox_test(0xADADADAD);
}

/**
 * @brief MBOX Data transfer by ping pong for forth set of channels
 *
 * Description same as for test_ping_pong_1
 *
 */
ZTEST(mbox_data_tests, test_ping_pong_4)
{
	mbox_test(0xDADADADA);
}

ZTEST_SUITE(mbox_data_tests, NULL, NULL, mbox_data_tests_before, mbox_data_tests_after, NULL);
