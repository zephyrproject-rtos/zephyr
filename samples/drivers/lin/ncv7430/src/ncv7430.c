/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/lin.h>

#include "ncv7430.h"

static struct lin_msg ncv7430_msg;
static uint8_t tx_data[LIN_MAX_DLEN];
static uint8_t rx_data[LIN_MAX_DLEN];
static volatile bool tx_complete;
static volatile bool rx_complete;
static volatile uint32_t ncv7430_event_flags;

void ncv7430_event_handler(const struct device *dev, const struct lin_event *event, void *user_data)
{
	switch (event->type) {
	case LIN_EVT_TX_DATA:
		tx_complete = true;
		ncv7430_event_flags = event->status;
		break;
	case LIN_EVT_RX_DATA:
		rx_complete = true;
		ncv7430_event_flags = event->status;
		break;
	case LIN_EVT_ERR:
		printk("NCV7430 LIN error event: 0x%08X\n", event->error_flags);
		ncv7430_event_flags = -EIO;
		break;
	default:
		break;
	}
}

static int ncv7430_send_command(const struct device *dev, const uint8_t cmd_id, const uint8_t *data,
				const uint8_t data_len)
{
	int ret;

	tx_complete = false;
	ncv7430_event_flags = 0;

	ncv7430_msg.id = cmd_id;
	ncv7430_msg.data_len = data_len;
	ncv7430_msg.checksum_type = LIN_CHECKSUM_CLASSIC;
	ncv7430_msg.flags = 0;
	ncv7430_msg.data = (void *)data;

	ret = lin_send(dev, &ncv7430_msg, K_FOREVER);
	if (ret) {
		printk("Failed to send NCV7430 command 0x%02X: %d\n", cmd_id, ret);
		return ret;
	}

	if (!WAIT_FOR(tx_complete, 500000, k_sleep(K_MSEC(10)))) {
		printk("Timeout waiting for NCV7430 command 0x%02X TX complete\n", cmd_id);
		return -EAGAIN;
	}

	return ncv7430_event_flags;
}

static int ncv7430_request_response(const struct device *dev, const uint8_t cmd_id, uint8_t *data,
				    const uint8_t data_len)
{
	int ret;

	rx_complete = false;
	ncv7430_event_flags = 0;

	/* Clear response data buffer */
	memset(data, NCV7430_FILL, data_len);

	ncv7430_msg.id = cmd_id;
	ncv7430_msg.data_len = data_len;
	ncv7430_msg.checksum_type = LIN_CHECKSUM_CLASSIC;
	ncv7430_msg.flags = 0;
	ncv7430_msg.data = (void *)data;

	ret = lin_receive(dev, &ncv7430_msg, K_FOREVER);
	if (ret) {
		printk("Failed to receive NCV7430 response for command 0x%02X: %d\n", cmd_id, ret);
		return ret;
	}

	if (!WAIT_FOR(rx_complete, 500000, k_sleep(K_MSEC(10)))) {
		printk("Timeout waiting for NCV7430 response for command 0x%02X\n", cmd_id);
		return -EAGAIN;
	}

	return ncv7430_event_flags;
}

int ncv7430_init(const struct device *dev, uint8_t addr)
{
	uint8_t leds_en = (NCV7430_LED_R_EN_MSK | NCV7430_LED_G_EN_MSK | NCV7430_LED_B_EN_MSK);
	int ret;

	ret = lin_set_callback(dev, ncv7430_event_handler, NULL);
	if (ret) {
		printk("Failed to set NCV7430 event handler: %d\n", ret);
		return ret;
	}

	/*
	 * NCV7430 Set_LED_Control WRITING FRAME
	 * Frame ID: 0x23
	 * Data Length: 4 bytes
	 * Data:
	 *      Byte 0:
	 *              [7:7] Broad: Individual Addressing (1) or Group Addressing (0)
	 *              [6:6] Fixed as 1
	 *              [5:0] AD: NCV7430 Node Address - unused in case of Group Addressing
	 *              Other bits are filled as 0
	 *      Byte 1: 0xFF (FILL)
	 *      Byte 2: 0xFF (FILL)
	 *      Byte 3:
	 *              [0:0] Fixed as 1
	 *              [3:1] LEDs Enable: 0x07 (All LEDs On), 0x00 (All LEDs Off)
	 *              [5:5] Global LEDs Enable: 1 (Enabled), 0 (Disabled)
	 *              [6:6] LED Modulation Frequency: 0 (122 Hz), 1 (244 Hz)
	 *              Other bits are filled as 0
	 */
	tx_data[0] = (addr & 0x3F) | BIT(6) | BIT(7);
	tx_data[1] = NCV7430_FILL;
	tx_data[2] = NCV7430_FILL;
	tx_data[3] = BIT(0) | (leds_en << 1) | BIT(5) | NCV7430_LED_MOD_FREQ_122_HZ << 6;

	ret = ncv7430_send_command(dev, NCV7430_FRAME_ID_SET_LED_CONTROL_WRITING, tx_data,
				   NCV7430_4_BYTES);
	if (ret) {
		return ret;
	}

	return 0;
}

int ncv7430_set_led_color(const struct device *dev, uint8_t addr, struct rgb_led_color *color)
{
	/**
	 * NCV7430 Set_Color WRITING FRAME
	 * Frame ID: 0x24
	 * Data Length: 8 bytes
	 * Data:
	 *      Byte 0:
	 *              [7:7] Broad: Individual Addressing (1) or Group Addressing (0)
	 *              [6:6] Fixed as 1
	 *              [5:0] AD: NCV7430 Node Address - unused in case of Group Addressing
	 *              Other bits are filled as 0
	 *      Byte 1: 0xFF (FILL)
	 *      Byte 2: 0xFF (FILL)
	 *      Byte 3: Fading time - set as 0 for immediate change
	 *      Byte 4:
	 *              [3:0] Intensity: 0x00 (Min) to 0xF (Max)
	 *              [4:4] Fixed as 1
	 *              [5:5] LEDs on/off: 1 (On), 0 (Off)
	 *              Other bits are filled as 0
	 *      Byte 5: Red Color Value: 0x00 (Min) to 0xFF (Max)
	 *      Byte 6: Green Color Value: 0x00 (Min) to 0xFF (Max)
	 *      Byte 7: Blue Color Value: 0x00 (Min) to 0xFF (Max)
	 */
	tx_data[0] = (addr & 0x3F) | BIT(6) | BIT(7);
	tx_data[1] = NCV7430_FILL;
	tx_data[2] = NCV7430_FILL;
	tx_data[3] = 0x00; /* Fading time == 0 */
	tx_data[4] = 0x01 | BIT(4) | BIT(5);
	tx_data[5] = color->green;
	tx_data[6] = color->red;
	tx_data[7] = color->blue;

	return ncv7430_send_command(dev, NCV7430_FRAME_ID_SET_LED_COLOR_WRITING, tx_data,
				    NCV7430_8_BYTES);
}

int ncv7430_get_led_color(const struct device *dev, uint8_t addr, struct rgb_led_color *color)
{
	int ret;

	/**
	 * NCV7430 Get_Actual_Param1 WRITING FRAME
	 * First send the command to read the full set of the actual parameters of the NCV7430
	 * Frame ID: 0x3c
	 * Data Length: 8 bytes
	 * Data:
	 *      Byte 0:
	 *              [7:7] Broad: Individual Addressing (1) or Group
	 *              [6:6] Fixed as 1
	 *              [5:0] AD: NCV7430 Node Address - unused in case of Group Addressing
	 *              Other bits are filled as 0
	 *      Byte 1: 0xFF (FILL)
	 */
	tx_data[0] = NCV7430_APPCMD;
	tx_data[1] = NCV7430_CMD_GET_ACTUAL_PARAM_1 | BIT(7);
	tx_data[2] = (addr & 0x3F) | BIT(6) | BIT(7);
	tx_data[3] = NCV7430_FILL;
	tx_data[4] = NCV7430_FILL;
	tx_data[5] = NCV7430_FILL;
	tx_data[6] = NCV7430_FILL;
	tx_data[7] = NCV7430_FILL;

	ret = ncv7430_send_command(dev, NCV7430_COMMANDER_COMMAND, tx_data, NCV7430_8_BYTES);
	if (ret) {
		return ret;
	}

	/**
	 * NCV7430 Get_Actual_Param In frame response 1
	 * Frame ID: 0x3D
	 * Data Length: 8 bytes
	 * Data:
	 *      Byte 0:
	 *              [7:7] Broad: Individual Addressing (1) or Group Addressing (0)
	 *              [6:6] Fixed as 1
	 *              [5:0] AD: NCV7430 Node Address - unused in case of Group Addressing
	 *      Byte 1: Green Color Value: 0x00 (Min) to 0xFF (Max)
	 *      Byte 2: Red Color Value: 0x00 (Min) to 0xFF (Max)
	 *      Byte 3: Blue Color Value: 0x00 (Min) to 0xFF (Max)
	 *      Byte 4: 0xFF (FILL)
	 *      Byte 5: 0xFF (FILL)
	 *      Byte 6: 0xFF (FILL)
	 *      Byte 7: 0xFF (FILL)
	 */
	ret = ncv7430_request_response(dev, NCV7430_RESPONDER_RESPONSE, rx_data, NCV7430_8_BYTES);
	if (ret) {
		return ret;
	}

	color->red = rx_data[2];
	color->green = rx_data[1];
	color->blue = rx_data[3];

	return 0;
}
