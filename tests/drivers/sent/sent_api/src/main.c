/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sent/sent.h>

#define SENT_NODE    DT_ALIAS(sent_node)
#define SENT_CHANNEL 1

const struct device *dev = DEVICE_DT_GET(SENT_NODE);

static void *sent_setup(void)
{
	zassert_true(device_is_ready(dev), "SENT device is not ready");

	return NULL;
}

void rx_cb(const struct device *dev, uint8_t channel_id, struct sent_frame *frame,
	   enum sent_status status, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(frame);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);
}

/**
 * @brief Test starting rx is not allowed while started.
 */
ZTEST_USER(sent_api, test_start_rx_while_started)
{
	int err;

	err = sent_start_rx(dev, SENT_CHANNEL);
	zassert_equal(err, 0, "Failed to start rx (err %d)", err);

	err = sent_start_rx(dev, SENT_CHANNEL);
	zassert_not_equal(err, 0, "Started rx while started");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);
}

/**
 * @brief Test stopping rx is not allowed while stopped.
 */
ZTEST_USER(sent_api, test_stop_rx_while_stoped)
{
	int err;

	err = sent_stop_rx(dev, SENT_CHANNEL);
	zassert_equal(err, 0, "Failed to stop rx (err %d)", err);

	err = sent_stop_rx(dev, SENT_CHANNEL);
	zassert_not_equal(err, 0, "Stopped rx while stopped");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);

	err = sent_start_rx(dev, SENT_CHANNEL);
	zassert_equal(err, 0, "Failed to start rx (err %d)", err);
}

/**
 * @brief Test setting the SENT rx callback.
 */
ZTEST(sent_api, test_set_rx_callback)
{
	int err;

	err = sent_add_rx_callback(dev, SENT_CHANNEL, rx_cb, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);

	sent_add_rx_callback(dev, SENT_CHANNEL, NULL, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);

	err = sent_add_rx_callback(dev, SENT_CHANNEL, rx_cb, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);
}

ZTEST_SUITE(sent_api, NULL, sent_setup, NULL, NULL, NULL);
