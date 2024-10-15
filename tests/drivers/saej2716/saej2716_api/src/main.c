/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/saej2716/saej2716.h>

#define SAEJ2716_NODE          DT_ALIAS(saej2716_node)
#define SAEJ2716_CHANNEL       1
#define SAEJ2716_MAX_RX_BUFFER 1

const struct device *saej2716_dev = DEVICE_DT_GET(SAEJ2716_NODE);

struct saej2716_frame serial_frame[SAEJ2716_MAX_RX_BUFFER];

struct saej2716_frame fast_frame[SAEJ2716_MAX_RX_BUFFER];

static void *saej2716_setup(void)
{
	zassert_true(device_is_ready(saej2716_dev), "SAEJ2716 device is not ready");

	return NULL;
}

void rx_serial_frame_cb(const struct device *dev, uint8_t channel_id, uint32_t num_frame,
			void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(num_frame);
	ARG_UNUSED(user_data);
}

void rx_fast_frame_cb(const struct device *dev, uint8_t channel_id, uint32_t num_frame,
		      void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(num_frame);
	ARG_UNUSED(user_data);
}

/**
 * @brief Test starting rx is not allowed while started.
 */
ZTEST_USER(saej2716_api, test_start_rx_while_started)
{
	int err;

	err = saej2716_start_rx(saej2716_dev, SAEJ2716_CHANNEL);
	zassert_equal(err, 0, "Failed to start rx (err %d)", err);

	err = saej2716_start_rx(saej2716_dev, SAEJ2716_CHANNEL);
	zassert_not_equal(err, 0, "Started rx while started");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);
}

/**
 * @brief Test stopping rx is not allowed while stopped.
 */
ZTEST_USER(saej2716_api, test_stop_rx_while_stopped)
{
	int err;

	err = saej2716_stop_rx(saej2716_dev, SAEJ2716_CHANNEL);
	zassert_equal(err, 0, "Failed to stop rx (err %d)", err);

	err = saej2716_stop_rx(saej2716_dev, SAEJ2716_CHANNEL);
	zassert_not_equal(err, 0, "Stopped rx while stopped");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);

	err = saej2716_start_rx(saej2716_dev, SAEJ2716_CHANNEL);
	zassert_equal(err, 0, "Failed to start rx (err %d)", err);
}

struct saej2716_rx_callback_config serial_cb_cfg = {
	.callback = rx_serial_frame_cb,
	.frame = &serial_frame[0],
	.max_num_frame = SAEJ2716_MAX_RX_BUFFER,
	.user_data = NULL,
};

struct saej2716_rx_callback_config fast_cb_cfg = {
	.callback = rx_fast_frame_cb,
	.frame = &fast_frame[0],
	.max_num_frame = SAEJ2716_MAX_RX_BUFFER,
	.user_data = NULL,
};

struct saej2716_rx_callback_configs callback_configs = {
	.serial = &serial_cb_cfg,
	.fast = &fast_cb_cfg,
};

/**
 * @brief Test setting the rx callback.
 */
ZTEST(saej2716_api, test_set_rx_callback)
{
	int err;

	err = saej2716_register_callback(saej2716_dev, SAEJ2716_CHANNEL, callback_configs);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);

	callback_configs.serial = NULL;
	callback_configs.fast = NULL;

	err = saej2716_register_callback(saej2716_dev, SAEJ2716_CHANNEL, callback_configs);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);

	callback_configs.serial = &serial_cb_cfg;
	callback_configs.fast = &fast_cb_cfg;

	err = saej2716_register_callback(saej2716_dev, SAEJ2716_CHANNEL, callback_configs);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);
}

ZTEST_SUITE(saej2716_api, NULL, saej2716_setup, NULL, NULL, NULL);
