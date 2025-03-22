/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psi5_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/psi5/psi5.h>

#define PSI5_NODE    DT_ALIAS(psi5_node)
#define PSI5_CHANNEL 1

void tx_cb(const struct device *dev, uint8_t channel_id, enum psi5_status status, void *user_data)
{
	LOG_INF("Transmitted data on channel %d", channel_id);
}

void rx_serial_frame_cb(const struct device *dev, uint8_t channel_id, struct psi5_frame *frame,
			enum psi5_status status, void *user_data)
{

	if (status == PSI5_RX_SERIAL_FRAME) {
		LOG_INF("Received a frame on channel %d, "
			"id: %d, data: %d, timestamp: %d, slot: %d",
			channel_id, frame->serial.id, frame->serial.data, frame->timestamp,
			frame->slot_number);
	} else {
		LOG_INF("Error received on channel %d", channel_id);
	}
}

void rx_data_frame_cb(const struct device *dev, uint8_t channel_id, struct psi5_frame *frame,
		      enum psi5_status status, void *user_data)
{

	if (status == PSI5_RX_DATA_FRAME) {
		LOG_INF("Received a frame on channel %d, "
			"data: %d, timestamp: %d",
			channel_id, frame->data, frame->timestamp);
	} else {
		LOG_INF("Error received on channel %d", channel_id);
	}
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(PSI5_NODE);
	uint64_t send_data = 0x1234;

	psi5_add_rx_callback(dev, PSI5_CHANNEL, rx_serial_frame_cb, rx_data_frame_cb, NULL);

	psi5_start_sync(dev, PSI5_CHANNEL);

	psi5_send(dev, PSI5_CHANNEL, send_data, K_MSEC(100), tx_cb, NULL);

	while (true) {
		/* To transmit and receive data */
	}

	return 0;
}
