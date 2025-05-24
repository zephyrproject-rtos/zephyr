/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(saej2716_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/saej2716/saej2716.h>

#define SAEJ2716_NODE          DT_ALIAS(saej2716_node)
#define SAEJ2716_CHANNEL       1
#define SAEJ2716_MAX_RX_BUFFER 1

struct saej2716_frame serial_frame[SAEJ2716_MAX_RX_BUFFER];
struct saej2716_frame fast_frame[SAEJ2716_MAX_RX_BUFFER];

void rx_serial_frame_cb(const struct device *dev, uint8_t channel_id, uint32_t num_frame,
			void *user_data)
{
	if (num_frame == SAEJ2716_MAX_RX_BUFFER) {
		LOG_INF("Received a frame on channel %d, "
			"id: %d, data: 0x%X, timestamp: 0x%X",
			channel_id, serial_frame->serial.id, serial_frame->serial.data,
			serial_frame->timestamp);
	} else {
		LOG_INF("Error received on channel %d", channel_id);
	}
}

void rx_fast_frame_cb(const struct device *dev, uint8_t channel_id, uint32_t num_frame,
		      void *user_data)
{
	if (num_frame == SAEJ2716_MAX_RX_BUFFER) {
		LOG_INF("Received a frame on channel %d, "
			"data: 0x%X, timestamp: 0x%X",
			channel_id, fast_frame->fast.data, fast_frame->timestamp);
	} else {
		LOG_INF("Error received on channel %d", channel_id);
	}
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

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(SAEJ2716_NODE);

	saej2716_register_callback(dev, SAEJ2716_CHANNEL, callback_configs);

	saej2716_start_rx(dev, SAEJ2716_CHANNEL);

	while (true) {
		/* To receive data */
	}

	return 0;
}
