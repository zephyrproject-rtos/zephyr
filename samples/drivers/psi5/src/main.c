/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psi5_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/psi5/psi5.h>

#define PSI5_NODE       DT_ALIAS(psi5_node)
#define PSI5_CHANNEL_RX 1
#define PSI5_CHANNEL_TX 2

void tx_cb(const struct device *dev, uint8_t channel_id, enum psi5_status status, void *user_data)
{
	LOG_INF("Tx channel %d completed\n", channel_id);
}

void rx_cb(const struct device *dev, uint8_t channel_id, struct psi5_frame *frame,
	   enum psi5_status status, void *user_data)
{

	LOG_INF("Rx channel %d completed\n", channel_id);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(PSI5_NODE);
	uint64_t send_data = 0x1234;

	/* Test receive data */
	psi5_add_rx_callback(dev, PSI5_CHANNEL_RX, rx_cb, NULL);

	psi5_start_sync(dev, PSI5_CHANNEL_RX);

	k_sleep(K_MSEC(100));

	psi5_stop_sync(dev, PSI5_CHANNEL_RX);

	/* Test send data */
	psi5_start_sync(dev, PSI5_CHANNEL_TX);

	psi5_send(dev, PSI5_CHANNEL_TX, send_data, K_MSEC(100), tx_cb, NULL);

	k_sleep(K_MSEC(100));

	psi5_stop_sync(dev, PSI5_CHANNEL_TX);

	return 0;
}
