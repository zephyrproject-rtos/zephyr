/*
 * Copyright 2023 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3733.h>
#include <string.h>

#define HW_ROW_COUNT 12
#define HW_COL_COUNT 16

/* LED matrix is addressed using a row major format */
#define LED_MATRIX_COORD(x, y) ((x) * HW_COL_COUNT) + (y)

static uint8_t led_state[HW_COL_COUNT * HW_ROW_COUNT];

static int led_channel_write(const struct device *led)
{
	int ret;
	uint32_t led_idx;

	/* Set all LEDs to full brightness */
	printk("Set all LEDs to full brightness\n");
	memset(led_state, 0, sizeof(led_state));
	for (uint8_t row = 0; row < CONFIG_LED_ROW_COUNT; row++) {
		for (uint8_t col = 0; col < CONFIG_LED_COLUMN_COUNT; col++) {
			led_idx = LED_MATRIX_COORD(row, col);
			led_state[led_idx] = 0xFF;
		}
	}
	ret = led_write_channels(led, 0, sizeof(led_state), led_state);
	if (ret) {
		printk("Error: could not write LED channels (%d)\n", ret);
		return ret;
	}
	k_msleep(1000);
	/* Disable quadrant of LED display */
	printk("Disable LED quadrant\n");
	for (uint8_t row = 0; row < CONFIG_LED_ROW_COUNT / 2; row++) {
		for (uint8_t col = 0; col < CONFIG_LED_COLUMN_COUNT / 2; col++) {
			led_idx = LED_MATRIX_COORD(row, col);
			led_state[led_idx] = 0x00;
		}
	}
	ret = led_write_channels(led, 0,
		((CONFIG_LED_ROW_COUNT / 2) * HW_COL_COUNT), led_state);
	if (ret) {
		printk("Error: could not write LED channels (%d)\n", ret);
		return ret;
	}
	k_msleep(1000);
	return 0;
}

static int led_brightness(const struct device *led)
{
	int ret;
	uint8_t row, col;

	/* Set LED brightness to low value sequentially */
	printk("Set LEDs to half brightness sequentially\n");
	for (row = 0; row < CONFIG_LED_ROW_COUNT; row++) {
		for (col = 0; col < CONFIG_LED_COLUMN_COUNT; col++) {
			ret = led_set_brightness(led, LED_MATRIX_COORD(row, col),
						50);
			if (ret < 0) {
				printk("Error: could not enable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
			k_msleep(100);
		}
	}
	return 0;
}

static int led_on_off(const struct device *led)
{
	int ret;
	uint8_t row, col;

	printk("Toggle each led\n");
	/* Turn on each led for a short duration */
	for (row = 0; row < CONFIG_LED_ROW_COUNT; row++) {
		for (col = 0; col < CONFIG_LED_COLUMN_COUNT; col++) {
			ret = led_off(led, LED_MATRIX_COORD(row, col));
			if (ret < 0) {
				printk("Error: could not disable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
			k_msleep(100);
			ret = led_on(led, LED_MATRIX_COORD(row, col));
			if (ret < 0) {
				printk("Error: could not enable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
		}
	}
	k_msleep(500);
	return 0;
}

const struct device *led_dev = DEVICE_DT_GET_ONE(issi_is31fl3733);

int main(void)
{
	int ret;
	int current_limit = 0xFF;

	if (!device_is_ready(led_dev)) {
		printk("Error- LED device is not ready\n");
		return 0;
	}

	while (1) {
		ret = led_channel_write(led_dev);
		if (ret < 0) {
			return 0;
		}
		ret = led_brightness(led_dev);
		if (ret < 0) {
			return 0;
		}
		ret = led_on_off(led_dev);
		if (ret < 0) {
			return 0;
		}
		if (current_limit == 0xFF) {
			/* Select lower current limt */
			printk("Restarting sample with lower current limit\n");
			current_limit = 0x3F;
			ret = is31fl3733_current_limit(led_dev, current_limit);
			if (ret) {
				printk("Could not set LED current limit (%d)\n", ret);
				return 0;
			}
		} else {
			/* Select higher current limt */
			printk("Restarting sample with higher current limit\n");
			current_limit = 0xFF;
			ret = is31fl3733_current_limit(led_dev, current_limit);
			if (ret) {
				printk("Could not set LED current limit (%d)\n", ret);
				return 0;
			}
		}
	}
}
