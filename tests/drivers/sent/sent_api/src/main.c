/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sent/sent.h>

#define SENT_NODE          DT_ALIAS(sent0)
#define SENT_CHANNEL       1
#define SENT_MAX_RX_BUFFER 1

const struct device *sent_dev = DEVICE_DT_GET(SENT_NODE);

struct sent_frame serial_frame[SENT_MAX_RX_BUFFER];

struct sent_frame fast_frame[SENT_MAX_RX_BUFFER];

static void *sent_setup(void)
{
	int err;

	zassert_true(device_is_ready(sent_dev), "SENT device is not ready");

	err = sent_start_listening(sent_dev, SENT_CHANNEL);
	zassert_ok(err, "Failed to start rx (err %d)", err);

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
ZTEST_USER(sent_api, test_start_listening_while_started)
{
	int err;

	err = sent_start_listening(sent_dev, SENT_CHANNEL);
	zassert_not_ok(err, "Started rx while started");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);
}

/**
 * @brief Test stopping rx is not allowed while stopped.
 */
ZTEST_USER(sent_api, test_stop_listening_while_stopped)
{
	int err;

	err = sent_stop_listening(sent_dev, SENT_CHANNEL);
	zassert_ok(err, "Failed to stop rx (err %d)", err);

	err = sent_stop_listening(sent_dev, SENT_CHANNEL);
	zassert_not_ok(err, "Stopped rx while stopped");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);

	err = sent_start_listening(sent_dev, SENT_CHANNEL);
	zassert_ok(err, "Failed to start rx (err %d)", err);
}

struct sent_rx_callback_config serial_cb_cfg = {
	.callback = rx_serial_frame_cb,
	.frame = &serial_frame[0],
	.max_num_frame = SENT_MAX_RX_BUFFER,
	.user_data = NULL,
};

struct sent_rx_callback_config fast_cb_cfg = {
	.callback = rx_fast_frame_cb,
	.frame = &fast_frame[0],
	.max_num_frame = SENT_MAX_RX_BUFFER,
	.user_data = NULL,
};

struct sent_rx_callback_configs callback_configs = {
	.serial = &serial_cb_cfg,
	.fast = &fast_cb_cfg,
};

/**
 * @brief Test setting the rx callback.
 */
ZTEST(sent_api, test_set_rx_callback)
{
	int err;

	err = sent_register_callback(sent_dev, SENT_CHANNEL, callback_configs);
	zassert_ok(err, "Failed to set rx callback (err %d)", err);

	callback_configs.serial = NULL;
	callback_configs.fast = NULL;

	err = sent_register_callback(sent_dev, SENT_CHANNEL, callback_configs);
	zassert_ok(err, "Failed to set rx callback (err %d)", err);

	callback_configs.serial = &serial_cb_cfg;
	callback_configs.fast = &fast_cb_cfg;

	err = sent_register_callback(sent_dev, SENT_CHANNEL, callback_configs);
	zassert_ok(err, "Failed to set rx callback (err %d)", err);
}

ZTEST_SUITE(sent_api, NULL, sent_setup, NULL, NULL, NULL);
