/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lin.h>
#include <zephyr/sys/byteorder.h>

#include "ncv7430.h"

#define LIN_BUS_BAUDRATE            19200U
#define LIN_BUS_BREAK_LEN           13U
#define LIN_BUS_BREAK_DELIMITER_LEN 1U

#define LED_CHANGE_INTERVAL_MS 1000

/* RGB LED test sequence (RGB888 format, separate channels) */
static const struct rgb_led_color rgb_led_test_colors[8] = {
	{255, 0, 0},   {0, 255, 0},   {0, 0, 255},     {255, 255, 0},
	{0, 255, 255}, {255, 0, 255}, {255, 255, 255}, {0, 0, 0},
};

/* RGB LED color names in string */
static const unsigned char rgb_test_color_name[][8] = {
	"Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "White", "Black",
};

static const struct lin_config dut_config = {
	.mode = LIN_MODE_COMMANDER,
	.baudrate = LIN_BUS_BAUDRATE,
	.break_len = LIN_BUS_BREAK_LEN,
	.break_delimiter_len = LIN_BUS_BREAK_DELIMITER_LEN,
	.flags = 0,
};

static unsigned int current_led_idx;

#if DT_HAS_ALIAS(lin0)
static const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(lin0));
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_lin_sci_b)
static const struct device *const dev = DEVICE_DT_GET(DT_INST(0, renesas_ra_lin_sci_b));
#else
#error "No LIN device found. Please enable at least one LIN device."
#endif

int main(void)
{
	int ret;

	ret = device_is_ready(dev);
	if (!ret) {
		printk("NCV7430 device not found\n");
		return 0;
	}

	ret = lin_configure(dev, &dut_config);
	if (ret) {
		printk("LIN configure failed: %d\n", ret);
		return 0;
	}

	ret = lin_start(dev);
	if (ret) {
		printk("LIN start failed: %d\n", ret);
		return 0;
	}

	ret = ncv7430_init(dev, NCV7430_NODE_ADDRESS);
	if (ret) {
		printk("NCV7430 initialization failed: %d\n", ret);
		return 0;
	}

	printk("NCV7430 sample started\n");
	current_led_idx = 0;

	while (true) {
		struct rgb_led_color color = rgb_led_test_colors[current_led_idx];
		struct rgb_led_color read_color;
		uint32_t color_code;

		ret = ncv7430_set_led_color(dev, NCV7430_NODE_ADDRESS, &color);
		if (ret != 0) {
			printk("Failed to set LED color: %d\n", ret);
			return 0;
		}

		ret = ncv7430_get_led_color(dev, NCV7430_NODE_ADDRESS, &read_color);
		if (ret != 0) {
			printk("Failed to get LED color: %d\n", ret);
			return 0;
		}

		color_code = sys_get_be24((uint8_t *)&read_color);
		printk("Set LED color: %s - read back actual color code: 0x%06X\n",
		       rgb_test_color_name[current_led_idx], color_code);

		if ((read_color.red != color.red) || (read_color.green != color.green) ||
		    (read_color.blue != color.blue)) {
			printk("Warning: Mismatch in set and read back color "
			       "values!\n");
		}

		current_led_idx++;
		if (current_led_idx == ARRAY_SIZE(rgb_led_test_colors)) {
			current_led_idx = 0;
		}

		k_msleep(LED_CHANGE_INTERVAL_MS);
	}

	return 0;
}
