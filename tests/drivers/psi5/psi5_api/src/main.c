/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/psi5/psi5.h>

#define PSI5_NODE    DT_ALIAS(psi5_node)
#define PSI5_CHANNEL 1

const struct device *dev = DEVICE_DT_GET(PSI5_NODE);
struct k_sem tx_callback_sem;

static void *psi5_setup(void)
{
	k_sem_init(&tx_callback_sem, 0, 1);

	zassert_true(device_is_ready(dev), "PSI5 device is not ready");

	return NULL;
}

void rx_cb(const struct device *dev, uint8_t channel_id, struct psi5_frame *frame,
	   enum psi5_status status, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(frame);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);
}

void tx_cb(const struct device *dev, uint8_t channel_id, enum psi5_status status, void *user_data)
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

	err = psi5_start_sync(dev, PSI5_CHANNEL);
	zassert_equal(err, 0, "Failed to start sync (err %d)", err);

	err = psi5_start_sync(dev, PSI5_CHANNEL);
	zassert_not_equal(err, 0, "Started sync while started");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);
}

/**
 * @brief Test stopping sync is not allowed while stopped.
 */
ZTEST_USER(psi5_api, test_stop_sync_while_stopped)
{
	int err;

	err = psi5_stop_sync(dev, PSI5_CHANNEL);
	zassert_equal(err, 0, "Failed to stop sync (err %d)", err);

	err = psi5_stop_sync(dev, PSI5_CHANNEL);
	zassert_not_equal(err, 0, "Stopped sync while stopped");
	zassert_equal(err, -EALREADY, "Wrong error return code (err %d)", err);

	err = psi5_start_sync(dev, PSI5_CHANNEL);
	zassert_equal(err, 0, "Failed to start sync (err %d)", err);
}

/**
 * @brief Test setting the rx callback.
 */
ZTEST(psi5_api, test_set_rx_callback)
{
	int err;

	err = psi5_add_rx_callback(dev, PSI5_CHANNEL, rx_cb, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);

	psi5_add_rx_callback(dev, PSI5_CHANNEL, NULL, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);

	err = psi5_add_rx_callback(dev, PSI5_CHANNEL, rx_cb, NULL);
	zassert_equal(err, 0, "Failed to set rx callback (err %d)", err);
}

/**
 * @brief Test sending data with callback.
 */
ZTEST(psi5_api, test_send_callback)
{
	int err;
	uint64_t send_data = 0x1234;

	k_sem_reset(&tx_callback_sem);

	err = psi5_start_sync(dev, PSI5_CHANNEL);
	zassert_equal(err, 0, "Failed to start sync (err %d)", err);

	err = psi5_send(dev, PSI5_CHANNEL, send_data, K_MSEC(100), tx_cb, NULL);
	zassert_equal(err, 0, "Failed to send (err %d)", err);

	k_sleep(K_MSEC(100));

	err = psi5_stop_sync(dev, PSI5_CHANNEL);
	zassert_equal(err, 0, "Failed to stop sync (err %d)", err);

	err = k_sem_take(&tx_callback_sem, K_MSEC(100));
	zassert_equal(err, 0, "missing TX callback");
}

/**
 * @brief Test sending data without callback.
 */
ZTEST(psi5_api, test_send_without_callback)
{
	int err;
	uint64_t send_data = 0x1234;

	err = psi5_start_sync(dev, PSI5_CHANNEL);
	zassert_equal(err, 0, "Failed to start sync (err %d)", err);

	err = psi5_send(dev, PSI5_CHANNEL, send_data, K_MSEC(100), NULL, NULL);
	zassert_equal(err, 0, "Failed to send (err %d)", err);

	k_sleep(K_MSEC(100));

	err = psi5_stop_sync(dev, PSI5_CHANNEL);
	zassert_equal(err, 0, "Failed to stop sync (err %d)", err);
}

/**
 * @brief Test sending data is not allowed while stopped sync.
 */
ZTEST(psi5_api, test_send_while_stopped_sync)
{
	int err;
	uint64_t send_data = 0x1234;

	err = psi5_send(dev, PSI5_CHANNEL, send_data, K_MSEC(100), NULL, NULL);
	zassert_not_equal(err, 0, "Sent data while stopped sync");
	zassert_equal(err, -ENETDOWN, "Wrong error return code (err %d)", err);
}

ZTEST_SUITE(psi5_api, NULL, psi5_setup, NULL, NULL, NULL);
