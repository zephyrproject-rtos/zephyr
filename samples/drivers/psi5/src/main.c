/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psi5_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/psi5/psi5.h>

#define PSI5_NODE          DT_ALIAS(psi5_0)
#define PSI5_CHANNEL       1
#define PSI5_MAX_RX_BUFFER 1

struct psi5_frame serial_frame[PSI5_MAX_RX_BUFFER];

struct psi5_frame data_frame[PSI5_MAX_RX_BUFFER];

void tx_cb(const struct device *dev, uint8_t channel_id, int status, void *user_data)
{
	LOG_INF("Transmitted data on channel %d", channel_id);
}

void rx_serial_frame_cb(const struct device *dev, uint8_t channel_id, uint32_t num_frame,
			void *user_data)
{

	if (num_frame == PSI5_MAX_RX_BUFFER) {
		LOG_INF("Received a frame on channel %d, "
			"id: %d, data: 0x%X, timestamp: 0x%X, slot: %d",
			channel_id, serial_frame->serial.id, serial_frame->serial.data,
			serial_frame->timestamp, serial_frame->slot_number);
	} else {
		LOG_INF("Error received on channel %d", channel_id);
	}
}

void rx_data_frame_cb(const struct device *dev, uint8_t channel_id, uint32_t num_frame,
		      void *user_data)
{

	if (num_frame == PSI5_MAX_RX_BUFFER) {
		LOG_INF("Received a frame on channel %d, "
			"data: 0x%X, timestamp: 0x%X",
			channel_id, data_frame->data, data_frame->timestamp);
	} else {
		LOG_INF("Error received on channel %d", channel_id);
	}
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

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(PSI5_NODE);
	uint64_t send_data = 0x1234;

	psi5_register_callback(dev, PSI5_CHANNEL, callback_configs);

	psi5_start_sync(dev, PSI5_CHANNEL);

	psi5_send(dev, PSI5_CHANNEL, send_data, K_MSEC(100), tx_cb, NULL);

	while (true) {
		/* To transmit and receive data */
	}

	return 0;
}
