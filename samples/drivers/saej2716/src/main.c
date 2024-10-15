/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/logging/log.h>
 LOG_MODULE_REGISTER(saej2716_sample, LOG_LEVEL_DBG);

 #include <zephyr/kernel.h>

 #include <zephyr/drivers/saej2716/saej2716.h>

 #define SAEJ2716_NODE    DT_ALIAS(saej2716_node)
 #define SAEJ2716_CHANNEL 1

 void rx_serial_frame_cb(const struct device *dev, uint8_t channel_id, struct saej2716_frame *frame,
			 enum saej2716_status status, void *user_data)
 {
	 if (status == SAEJ2716_RX_SERIAL_FRAME) {
		 LOG_INF("Received a frame on channel %d, "
			 "id: %d, data: %d, timestamp: %d",
			 channel_id, frame->serial.id, frame->serial.data, frame->timestamp);
	 } else {
		 LOG_INF("Error received on channel %d", channel_id);
	 }
 }

 void rx_fast_frame_cb(const struct device *dev, uint8_t channel_id, struct saej2716_frame *frame,
		       enum saej2716_status status, void *user_data)
 {
	 if (status == SAEJ2716_RX_FAST_FRAME) {
		 LOG_INF("Received a frame on channel %d, "
			 "data: %d, timestamp: %d",
			 channel_id, frame->serial.data, frame->timestamp);
	 } else {
		 LOG_INF("Error received on channel %d", channel_id);
	 }
 }

 int main(void)
 {
	 const struct device *const dev = DEVICE_DT_GET(SAEJ2716_NODE);

	saej2716_add_rx_callback(dev, SAEJ2716_CHANNEL, rx_serial_frame_cb, rx_fast_frame_cb, NULL);

	saej2716_start_rx(dev, SAEJ2716_CHANNEL);

	 while (true) {
		 /* To receive data */
	 }

	 return 0;
 }
