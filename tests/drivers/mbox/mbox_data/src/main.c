/*
 * Copyright 2024-2025 NXP
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

static uint64_t g_mbox_received_data;
static uint64_t g_mbox_expected_data;
static uint32_t g_mbox_received_channel;
static uint32_t g_mbox_expected_channel;

static bool g_received_size_error;
static size_t g_received_size;
static int g_max_transfer_size_bytes;

#define TX_CHANNEL_INDEX 0
#define RX_CHANNEL_INDEX 1

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_mbox_consumer)
#define MBOX_CONSUMER_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_mbox_consumer)
#elif DT_HAS_COMPAT_STATUS_OKAY(vnd_mbox_consumer)
#define MBOX_CONSUMER_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(vnd_mbox_consumer)
#else
#error "No zephyr,mbox-consumer or vnd,mbox-consumer node in the devicetree"
#endif

BUILD_ASSERT(DT_PROP_LEN(MBOX_CONSUMER_NODE, mboxes) % 2 == 0,
	     "mboxes must contain tx/rx channel pairs");

#define CHANNELS_TO_TEST (DT_PROP_LEN(MBOX_CONSUMER_NODE, mboxes) / 2)

#if CHANNELS_TO_TEST > 4
#error "The test suite covers at most 4 tx/rx channel pairs"
#endif

/* On loopback platforms sent data comes back unmodified instead of being
 * incremented by a remote core.
 */
#define TEST_LOOPBACK DT_PROP_OR(MBOX_CONSUMER_NODE, loopback, 0)

#define CHANNEL_ENTRY(_i)                                                                          \
	{                                                                                          \
		MBOX_DT_SPEC_GET(MBOX_CONSUMER_NODE, CONCAT(tx, _i)),                              \
		MBOX_DT_SPEC_GET(MBOX_CONSUMER_NODE, CONCAT(rx, _i)),                              \
	}

static const struct mbox_dt_spec channels[CHANNELS_TO_TEST][2] = {
	CHANNEL_ENTRY(0),
#if CHANNELS_TO_TEST >= 2
	CHANNEL_ENTRY(1),
#endif
#if CHANNELS_TO_TEST >= 3
	CHANNEL_ENTRY(2),
#endif
#if CHANNELS_TO_TEST >= 4
	CHANNEL_ENTRY(3),
#endif
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
	zassert_false(current_channel_index >= CHANNELS_TO_TEST,
		      "Channel to test is out of range");

	const struct mbox_dt_spec *tx_channel = &channels[current_channel_index][TX_CHANNEL_INDEX];
	const struct mbox_dt_spec *rx_channel = &channels[current_channel_index][RX_CHANNEL_INDEX];
	int ret_val = 0;

	g_max_transfer_size_bytes = mbox_mtu_get_dt(tx_channel);
	/* Test currently supports only transfer size up to 8 bytes */
	if ((g_max_transfer_size_bytes < 0) || (g_max_transfer_size_bytes > 8)) {
		printk("mbox_mtu_get() error\n");
		zassert_false(1, "mbox invalid maximum transfer unit: %d",
			      g_max_transfer_size_bytes);
	}

	ret_val = mbox_register_callback_dt(rx_channel, callback, NULL);
	zassert_false(ret_val != 0, "mbox failed to register callback. ret_val: %d", ret_val);

	ret_val = mbox_set_enabled_dt(rx_channel, 1);
	zassert_false(ret_val != 0, "mbox failed to enable mbox. ret_val: %d", ret_val);
}

static void mbox_data_tests_after(void *f)
{
	zassert_false(current_channel_index >= CHANNELS_TO_TEST,
		      "Channel to test is out of range");

	const struct mbox_dt_spec *rx_channel = &channels[current_channel_index][RX_CHANNEL_INDEX];

	/* Disable channel after test end */
	int ret_val = mbox_set_enabled_dt(rx_channel, 0);

	zassert_false(ret_val != 0, "mbox failed to disable mbox. ret_val: %d", ret_val);

	/* Increment current channel index to its prepared for next test */
	current_channel_index++;
}

static void mbox_test(const uint64_t data)
{
	struct mbox_msg msg = {0};
	uint64_t test_data = data;
	int test_count = 0;
	int ret_val = 0;

	while (test_count < 100) {
		const struct mbox_dt_spec *tx_channel =
			&channels[current_channel_index][TX_CHANNEL_INDEX];

		/* Main core prepare test data */
		msg.data = &test_data;
		msg.size = g_max_transfer_size_bytes;

		/* Main core send test data */
		ret_val = mbox_send_dt(tx_channel, &msg);
		zassert_false(ret_val < 0, "mbox failed to send. ret_val: %d", ret_val);

		/*
		 * Determine expected received data based on the configured Maximum
		 * Transfer Unit (MTU). Supported MTU sizes are 1-8 bytes.
		 * On loopback channels the received data should match the sent
		 * data. Otherwise, it is expected to be incremented by one.
		 */
		g_mbox_expected_data = test_data & GENMASK64((g_max_transfer_size_bytes * 8) - 1,
							   0); /* Mask data to MTU size */
		if (!TEST_LOOPBACK) {
			g_mbox_expected_data++;
		}

		k_sem_take(&g_mbox_data_rx_sem, K_FOREVER);

		if (g_received_size_error) {
			zassert_false(1, "mbox received invalid size in callback: %zd",
				      g_received_size);
		}

		test_data = g_mbox_received_data;

		/* Main core check received data */
		zassert_equal(g_mbox_expected_data, test_data,
			      "Received test_data does not match!: Expected: %08llX, Got: %08llX",
			      g_mbox_expected_data, test_data);

		/* Expect reception of data on current RX channel */
		g_mbox_expected_channel =
			channels[current_channel_index][RX_CHANNEL_INDEX].channel_id;
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

#if CHANNELS_TO_TEST >= 2

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

#endif

#if CHANNELS_TO_TEST >= 3

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

#endif

#if CHANNELS_TO_TEST >= 4

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

#endif

ZTEST_SUITE(mbox_data_tests, NULL, NULL, mbox_data_tests_before, mbox_data_tests_after, NULL);
