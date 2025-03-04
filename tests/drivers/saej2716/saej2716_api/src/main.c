/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/saej2716/saej2716.h>

#define SAEJ2716_NODE    DT_ALIAS(saej2716_node)
#define SAEJ2716_CHANNEL 1

const struct device *dev = DEVICE_DT_GET(SAEJ2716_NODE);

static void *saej2716_setup(void)
{
	zassert_true(device_is_ready(dev), "SAEJ2716 device is not ready");

	return NULL;
}

void rx_serial_frame_cb(const struct device *dev, uint8_t channel_id, struct saej2716_frame *frame,
			enum saej2716_status status, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(frame);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);
}

void rx_fast_frame_cb(const struct device *dev, uint8_t channel_id, struct saej2716_frame *frame,
		      enum saej2716_status status, void *user_data)
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
ZTEST_USER(saej2716_api, test_start_rx_while_started)
{
	int err;

	err = saej2716_start_rx(dev, SAEJ2716_CHANNEL);
	zassert_equal(err, 0, "Failed to start rx (err %d)", err);

	err = saej2716_start_rx(dev, SAEJ2716_CHANNEL);
	zassert_not_equal(err, 0, "Started rx while started");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);
}

/**
 * @brief Test stopping rx is not allowed while stopped.
 */
ZTEST_USER(saej2716_api, test_stop_rx_while_stopped)
{
	int err;

	err = saej2716_stop_rx(dev, SAEJ2716_CHANNEL);
	zassert_equal(err, 0, "Failed to stop rx (err %d)", err);

	err = saej2716_stop_rx(dev, SAEJ2716_CHANNEL);
	zassert_not_equal(err, 0, "Stopped rx while stopped");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);

	err = saej2716_start_rx(dev, SAEJ2716_CHANNEL);
	zassert_equal(err, 0, "Failed to start rx (err %d)", err);
}

/**
 * @brief Test setting the rx callback.
 */
ZTEST(saej2716_api, test_set_rx_callback)
{
	int err;

	err = saej2716_add_rx_callback(dev, SAEJ2716_CHANNEL, rx_serial_frame_cb, rx_fast_frame_cb, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);

	saej2716_add_rx_callback(dev, SAEJ2716_CHANNEL, NULL, NULL, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);

	err = saej2716_add_rx_callback(dev, SAEJ2716_CHANNEL, rx_serial_frame_cb, rx_fast_frame_cb, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);
}

ZTEST_SUITE(saej2716_api, NULL, saej2716_setup, NULL, NULL, NULL);
