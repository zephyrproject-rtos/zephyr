/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/lin.h>

#include "ncv7430_mock.h"

static uint8_t node_address;
static struct rgb_led_color current_color;
static uint8_t leds_enable_mask;

static struct lin_msg work_msg;
static struct lin_msg response_msg;

static void ncv7430_mock_handle_rx_header(const struct device *dev, uint8_t pid)
{
	uint8_t id = lin_get_frame_id(pid);

	if (!lin_verify_pid(pid)) {
		printk("NCV7430 mock: received invalid PID 0x%02X\n", pid);
		return;
	}

	switch (id) {
	case NCV7430_FRAME_ID_SET_LED_CONTROL_WRITING:
		work_msg.id = id;
		work_msg.data_len = NCV7430_4_BYTES;
		work_msg.checksum_type = LIN_CHECKSUM_CLASSIC;

		if (lin_read(dev, &work_msg, K_FOREVER)) {
			printk("NCV7430 mock: failed to read Set_LED_Control payload\n");
		}
		break;

	case NCV7430_FRAME_ID_SET_LED_COLOR_WRITING:
		work_msg.id = id;
		work_msg.data_len = NCV7430_8_BYTES;
		work_msg.checksum_type = LIN_CHECKSUM_CLASSIC;

		if (lin_read(dev, &work_msg, K_FOREVER)) {
			printk("NCV7430 mock: failed to read Set_Color payload\n");
		}
		break;

	case NCV7430_COMMANDER_COMMAND:
		work_msg.id = id;
		work_msg.data_len = NCV7430_8_BYTES;
		work_msg.checksum_type = LIN_CHECKSUM_CLASSIC;

		if (lin_read(dev, &work_msg, K_FOREVER)) {
			printk("NCV7430 mock: failed to read Commander_Command payload\n");
		}
		break;

	case NCV7430_RESPONDER_RESPONSE:
		response_msg.id = id;
		response_msg.data_len = NCV7430_8_BYTES;
		response_msg.checksum_type = LIN_CHECKSUM_CLASSIC;

		response_msg.data[0] = (node_address & 0x3F) | BIT(6) | BIT(7);
		response_msg.data[1] = current_color.green;
		response_msg.data[2] = current_color.red;
		response_msg.data[3] = current_color.blue;
		response_msg.data[4] = NCV7430_FILL;
		response_msg.data[5] = NCV7430_FILL;
		response_msg.data[6] = NCV7430_FILL;
		response_msg.data[7] = NCV7430_FILL;

		if (lin_response(dev, &response_msg, K_FOREVER)) {
			printk("NCV7430 mock: failed to send Responder_Response\n");
		}
		break;

	default:
		break;
	}
}

static void ncv7430_mock_handle_rx_data(uint8_t pid)
{
	uint8_t id = lin_get_frame_id(pid);

	switch (id) {
	case NCV7430_FRAME_ID_SET_LED_CONTROL_WRITING:
		leds_enable_mask = (work_msg.data[3] >> 1) &
				   (NCV7430_LED_R_EN_MSK | NCV7430_LED_G_EN_MSK |
				    NCV7430_LED_B_EN_MSK);
		printk("NCV7430 mock: LED enable mask updated to 0x%02X\n", leds_enable_mask);
		break;

	case NCV7430_FRAME_ID_SET_LED_COLOR_WRITING:
		current_color.green = work_msg.data[5];
		current_color.red = work_msg.data[6];
		current_color.blue = work_msg.data[7];
		printk("NCV7430 mock: LED color updated to R=0x%02X G=0x%02X B=0x%02X\n",
		       current_color.red, current_color.green, current_color.blue);
		break;

	case NCV7430_COMMANDER_COMMAND:
		/* Sub-command byte is ignored: the mock always answers the same
		 * stored LED color regardless of which Get_* command was requested.
		 */
		break;

	default:
		break;
	}
}

static void ncv7430_mock_event_handler(const struct device *dev, const struct lin_event *event,
					void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event->type) {
	case LIN_EVT_RX_HEADER:
		ncv7430_mock_handle_rx_header(dev, event->header.pid);
		break;

	case LIN_EVT_RX_DATA:
		if (event->status != 0) {
			printk("NCV7430 mock: RX data error: %d\n", event->status);
			break;
		}

		ncv7430_mock_handle_rx_data(event->data.pid);
		break;

	case LIN_EVT_TX_DATA:
		if (event->status != 0) {
			printk("NCV7430 mock: TX data error: %d\n", event->status);
		}
		break;

	case LIN_EVT_ERR:
		printk("NCV7430 mock: LIN error event: 0x%08X\n", event->error_flags);
		break;

	default:
		break;
	}
}

int ncv7430_mock_init(const struct device *dev, uint8_t addr)
{
	node_address = addr;
	memset(&current_color, 0, sizeof(current_color));
	leds_enable_mask = NCV7430_LED_R_EN_MSK | NCV7430_LED_G_EN_MSK | NCV7430_LED_B_EN_MSK;

	return lin_set_callback(dev, ncv7430_mock_event_handler, NULL);
}
