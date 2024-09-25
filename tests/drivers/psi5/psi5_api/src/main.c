/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/psi5/psi5.h>

#define PSI5_NODE          DT_ALIAS(psi5_0)
#define PSI5_CHANNEL       1
#define PSI5_MAX_RX_BUFFER 1

const struct device *psi5_dev = DEVICE_DT_GET(PSI5_NODE);
struct k_sem tx_callback_sem;

struct psi5_frame serial_frame[PSI5_MAX_RX_BUFFER];

struct psi5_frame data_frame[PSI5_MAX_RX_BUFFER];

static void *psi5_setup(void)
{
	int err;

	k_sem_init(&tx_callback_sem, 0, 1);

	zassert_true(device_is_ready(psi5_dev), "PSI5 device is not ready");

	err = psi5_start_sync(psi5_dev, PSI5_CHANNEL);
	zassert_ok(err, "Failed to start sync (err %d)", err);

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

void rx_data_frame_cb(const struct device *dev, uint8_t channel_id, uint32_t num_frame,
		      void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(num_frame);
	ARG_UNUSED(user_data);
}

void tx_cb(const struct device *dev, uint8_t channel_id, int status, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);

	k_sem_give(&tx_callback_sem);
}

/**
 * @brief Test starting sync is not allowed while started.
 */
ZTEST_USER(psi5_api, test_start_sync_while_started)
{
	int err;

	err = psi5_start_sync(psi5_dev, PSI5_CHANNEL);
	zassert_not_ok(err, "Started sync while started");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);
}

/**
 * @brief Test stopping sync is not allowed while stopped.
 */
ZTEST_USER(psi5_api, test_stop_sync_while_stopped)
{
	int err;

	err = psi5_stop_sync(psi5_dev, PSI5_CHANNEL);
	zassert_ok(err, "Failed to stop sync (err %d)", err);

	err = psi5_stop_sync(psi5_dev, PSI5_CHANNEL);
	zassert_not_ok(err, "Stopped sync while stopped");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);

	err = psi5_start_sync(psi5_dev, PSI5_CHANNEL);
	zassert_ok(err, "Failed to start sync (err %d)", err);
}

struct psi5_rx_callback_config serial_cb_cfg = {
	.callback = rx_serial_frame_cb,
	.frame = &serial_frame[0],
	.max_num_frame = PSI5_MAX_RX_BUFFER,
	.user_data = NULL,
};

struct psi5_rx_callback_config data_cb_cfg = {
	.callback = rx_data_frame_cb,
	.frame = &data_frame[0],
	.max_num_frame = PSI5_MAX_RX_BUFFER,
	.user_data = NULL,
};

struct psi5_rx_callback_configs callback_configs = {
	.serial_frame = &serial_cb_cfg,
	.data_frame = &data_cb_cfg,
};

/**
 * @brief Test setting the rx callback.
 */
ZTEST(psi5_api, test_set_rx_callback)
{
	int err;

	err = psi5_register_callback(psi5_dev, PSI5_CHANNEL, callback_configs);
	zassert_ok(err, "Failed to set rx callback (err %d)", err);

	callback_configs.serial_frame = NULL;
	callback_configs.data_frame = NULL;

	err = psi5_register_callback(psi5_dev, PSI5_CHANNEL, callback_configs);
	zassert_ok(err, "Failed to set rx callback (err %d)", err);

	callback_configs.serial_frame = &serial_cb_cfg;
	callback_configs.data_frame = &data_cb_cfg;

	err = psi5_register_callback(psi5_dev, PSI5_CHANNEL, callback_configs);
	zassert_ok(err, "Failed to set rx callback (err %d)", err);
}

/**
 * @brief Test sending data with callback.
 */
ZTEST(psi5_api, test_send_callback)
{
	int err;
	uint64_t send_data = 0x1234;

	k_sem_reset(&tx_callback_sem);

	err = psi5_send(psi5_dev, PSI5_CHANNEL, send_data, K_MSEC(100), tx_cb, NULL);
	zassert_ok(err, "Failed to send (err %d)", err);

	err = k_sem_take(&tx_callback_sem, K_MSEC(100));
	zassert_ok(err, "missing TX callback");
}

/**
 * @brief Test sending data without callback.
 */
ZTEST(psi5_api, test_send_without_callback)
{
	int err;
	uint64_t send_data = 0x1234;

	err = psi5_send(psi5_dev, PSI5_CHANNEL, send_data, K_MSEC(100), NULL, NULL);
	zassert_ok(err, "Failed to send (err %d)", err);
}

/**
 * @brief Test sending data is not allowed while stopped sync.
 */
ZTEST(psi5_api, test_send_while_stopped_sync)
{
	int err;
	uint64_t send_data = 0x1234;

	err = psi5_stop_sync(psi5_dev, PSI5_CHANNEL);
	zassert_ok(err, "Failed to stop sync (err %d)", err);

	err = psi5_send(psi5_dev, PSI5_CHANNEL, send_data, K_MSEC(100), NULL, NULL);
	zassert_not_ok(err, "Sent data while stopped sync");
	zassert_equal(err, -ENETDOWN, "Wrong error return code (err %d)", err);

	err = psi5_start_sync(psi5_dev, PSI5_CHANNEL);
	zassert_ok(err, "Failed to start sync (err %d)", err);
}

ZTEST_SUITE(psi5_api, NULL, psi5_setup, NULL, NULL, NULL);
