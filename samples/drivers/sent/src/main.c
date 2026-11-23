/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sent_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/sent/sent.h>

#define SENT_NODE          DT_ALIAS(sent0)
#define SENT_CHANNEL       1
#define SENT_MAX_RX_BUFFER 1

struct sent_frame serial_frame[SENT_MAX_RX_BUFFER];
struct sent_frame fast_frame[SENT_MAX_RX_BUFFER];

void rx_serial_frame_cb(const struct device *dev, uint8_t channel_id, uint32_t num_frame,
			void *user_data)
{
	if (num_frame == SENT_MAX_RX_BUFFER) {
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
	if (num_frame == SENT_MAX_RX_BUFFER) {
		LOG_INF("Received a frame on channel %d, "
			"data nibble 0: 0x%X, "
			"data nibble 1: 0x%X, "
			"data nibble 2: 0x%X, "
			"data nibble 3: 0x%X, "
			"data nibble 4: 0x%X, "
			"data nibble 5: 0x%X, "
			"timestamp: 0x%X",
			channel_id, fast_frame->fast.data_nibbles[0],
			fast_frame->fast.data_nibbles[1], fast_frame->fast.data_nibbles[2],
			fast_frame->fast.data_nibbles[3], fast_frame->fast.data_nibbles[4],
			fast_frame->fast.data_nibbles[5], fast_frame->timestamp);
	} else {
		LOG_INF("Error received on channel %d", channel_id);
	}
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

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(SENT_NODE);

	sent_register_callback(dev, SENT_CHANNEL, callback_configs);

	sent_start_listening(dev, SENT_CHANNEL);

	while (true) {
		/* To receive data */
	}

	return 0;
}
