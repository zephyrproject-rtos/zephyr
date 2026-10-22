/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/lp50xx.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define MAX_BRIGHTNESS	100

#define SLEEP_DELAY_MS	1000
#define FADE_DELAY_MS	10

#define FADE_DELAY	K_MSEC(FADE_DELAY_MS)
#define SLEEP_DELAY	K_MSEC(SLEEP_DELAY_MS)

/*
 * The following colors are shown in the given order. Each row index is sorted
 * in RGB order.
 */
static uint8_t colors[][3] = {
	{ 0xFF, 0x00, 0x00 }, /* Red    */
	{ 0x00, 0xFF, 0x00 }, /* Green  */
	{ 0x00, 0x00, 0xFF }, /* Blue   */
	{ 0xFF, 0xFF, 0xFF }, /* White  */
	{ 0xFF, 0xFF, 0x00 }, /* Yellow */
	{ 0xFF, 0x00, 0xFF }, /* Purple */
	{ 0x00, 0xFF, 0xFF }, /* Cyan   */
	{ 0xF4, 0x79, 0x20 }, /* Orange */
};

/*
 * Prepare a color buffer for a single LED using its color mapping and the
 * desired color index.
 */
static int prepare_color_buffer(const struct led_info *info, uint8_t *buf,
				uint8_t color_idx)
{
	uint8_t color;

	for (color = 0; color < info->num_colors; color++) {
		switch (info->color_mapping[color]) {
		case LED_COLOR_ID_RED:
			buf[color] = colors[color_idx][0];
			continue;
		case LED_COLOR_ID_GREEN:
			buf[color] = colors[color_idx][1];
			continue;
		case LED_COLOR_ID_BLUE:
			buf[color] = colors[color_idx][2];
			continue;
		default:
			LOG_ERR("Invalid color: %d",
				info->color_mapping[color]);
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * @brief Run tests on a single LED using the LED-based API syscalls.
 *
 * @param lp50xx_dev LP50XX LED controller device.
 * @param led Number of the LED to test.
 */
static int run_led_test(const struct device *lp50xx_dev, uint8_t led)
{
	const struct led_info *info;
	uint8_t idx;
	int err;

	LOG_INF("Testing LED %d (LED API)", led);

	err = led_get_info(lp50xx_dev, led, &info);
	if (err < 0) {
		LOG_ERR("Failed to get LED %d info", led);
		return err;
	}

	for (idx = 0; idx < ARRAY_SIZE(colors); idx++) {
		uint16_t level;
		uint8_t buf[3];

		err = prepare_color_buffer(info, buf, idx);
		if (err < 0) {
			LOG_ERR("Failed to set color buffer, err=%d", err);
			return err;
		}

		/* Update LED color. */
		err = led_set_color(lp50xx_dev, led, 3, buf);
		if (err < 0) {
			LOG_ERR("Failed to set LED %d color to "
				"%02x:%02x:%02x, err=%d", led,
				buf[0], buf[1], buf[2], err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LED on. */
		err = led_on(lp50xx_dev, led);
		if (err < 0) {
			LOG_ERR("Failed to turn LED %d on, err=%d", led, err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LED off. */
		err = led_off(lp50xx_dev, led);
		if (err < 0) {
			LOG_ERR("Failed to turn LED %d off, err=%d", led, err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Set LED brightness gradually to the maximum level. */
		for (level = 0; level <= MAX_BRIGHTNESS; level++) {
			err = led_set_brightness(lp50xx_dev, led, level);
			if (err < 0) {
				LOG_ERR("Failed to set LED %d brightness to %d"
					", err=%d\n", led, level, err);
				return err;
			}
			k_sleep(FADE_DELAY);
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LED off. */
		err = led_off(lp50xx_dev, led);
		if (err < 0) {
			LOG_ERR("Failed to turn LED %d off, err=%d", led, err);
			return err;
		}
		k_sleep(SLEEP_DELAY);
	}

	return 0;
}

/**
 * @brief Run tests on all the LEDs using the channel-based API syscalls.
 *
 * @param lp50xx_dev LP50XX LED controller device.
 */
static int run_channel_test(const struct device *lp50xx_dev,
			    const uint8_t max_leds,
			    const uint8_t bright_chan,
			    const uint8_t color_chan)
{
	uint8_t idx;
	uint8_t buffer[LP50XX_COLORS_PER_LED * max_leds];
	int err;

	LOG_INF("Testing all LEDs (channel API)");

	for (idx = 0; idx < ARRAY_SIZE(colors); idx++) {
		uint8_t led;
		uint16_t level;

		/* Update LEDs colors. */
		memset(buffer, 0, sizeof(buffer));
		for (led = 0; led < max_leds; led++) {
			const struct led_info *info;
			uint8_t *col = &buffer[led * 3];

			err = led_get_info(lp50xx_dev, led, &info);
			if (err < 0) {
				continue;
			}

			col = &buffer[info->index * 3];
			err = prepare_color_buffer(info, col, idx);
			if (err < 0) {
				LOG_ERR("Failed to set color buffer, err=%d",
					err);
				return err;
			}
		}

		err = led_write_channels(lp50xx_dev, color_chan,
					 LP50XX_COLORS_PER_LED *
					 max_leds,
					 buffer);
		if (err < 0) {
			LOG_ERR("Failed to write channels, start=%d num=%d"
				" err=%d\n", color_chan,
				LP50XX_COLORS_PER_LED * max_leds, err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs on. */
		memset(buffer, 0, sizeof(buffer));
		for (led = 0; led < max_leds; led++) {
			buffer[led] = MAX_BRIGHTNESS;
		}
		err = led_write_channels(lp50xx_dev,
					 bright_chan,
					 max_leds, buffer);
		if (err < 0) {
			LOG_ERR("Failed to write channels, start=%d num=%d"
				" err=%d\n", bright_chan,
				max_leds, err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs off. */
		for (led = 0; led < max_leds; led++) {
			buffer[led] = 0;
		}
		err = led_write_channels(lp50xx_dev,
					 bright_chan,
					 max_leds, buffer);
		if (err < 0) {
			LOG_ERR("Failed to write channels, start=%d num=%d"
				" err=%d\n", bright_chan,
				max_leds, err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Set LEDs brightnesses gradually to the maximum level. */
		for (level = 0; level <= MAX_BRIGHTNESS; level++) {
			for (led = 0; led < max_leds; led++) {
				buffer[led] = level;
			}
			err = led_write_channels(lp50xx_dev,
					bright_chan,
					max_leds, buffer);
			if (err < 0) {
				LOG_ERR("Failed to write channels, start=%d"
					" num=%d err=%d\n",
					bright_chan,
					max_leds, err);
				return err;
			}
			k_sleep(FADE_DELAY);
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs off. */
		for (led = 0; led < max_leds; led++) {
			buffer[led] = 0;
		}
		err = led_write_channels(lp50xx_dev,
					 bright_chan,
					 max_leds, buffer);
		if (err < 0) {
			LOG_ERR("Failed to write channels, start=%d "
				"num=%d err=%d\n", bright_chan,
				max_leds, err);
			return err;
		}
		k_sleep(SLEEP_DELAY);
	}

	return 0;
}

static int run_test(const struct device *const lp50xx_dev,
		    const uint8_t max_leds,
		    const uint8_t bright_chan,
		    const uint8_t color_chan)
{
	int err;
	uint8_t led;
	uint8_t num_leds = 0;

	for (led = 0; led < max_leds; led++) {
		int col;
		const struct led_info *info;

		err = led_get_info(lp50xx_dev, led, &info);
		if (err < 0) {
			LOG_DBG("Failed to get information for LED %d (err=%d)",
				led, err);
			break;
		}

		/* Display LED information. */
		printk("Found LED %d", led);
		if (info->label) {
			printk(" - %s", info->label);
		}
		printk(" - index:%d", info->index);
		printk(" - %d colors", info->num_colors);
		if (!info->color_mapping) {
			continue;
		}
		printk(" - %d", info->color_mapping[0]);
		for (col = 1; col < info->num_colors; col++) {
			printk(":%d", info->color_mapping[col]);
		}
		printk("\n");
	}

	num_leds = led;
	if (!num_leds) {
		LOG_ERR("No LEDs found");
		return 0;
	}

	do {
		err = run_channel_test(lp50xx_dev, max_leds, bright_chan,
					color_chan);
		if (err) {
			return 0;
		}
		for (led = 0; led < num_leds; led++) {
			err = run_led_test(lp50xx_dev, led);
			if (err) {
				return 0;
			}
		}
	} while (true);
	return 0;
}

int main(void)
{
	const struct device *lp50xx_dev;
	bool found = false;

	lp50xx_dev = DEVICE_DT_GET_ANY(ti_lp5009);
	if (lp50xx_dev) {
		LOG_INF("Found LED controller %s", lp50xx_dev->name);
		found = true;

		if (device_is_ready(lp50xx_dev)) {
			run_test(lp50xx_dev,
				 LP5009_MAX_LEDS,
				 LP50XX_LED_BRIGHT_CHAN_BASE,
				 LP5012_LED_COL_CHAN_BASE);
		} else {
			LOG_ERR("LED controller %s is not ready",
				lp50xx_dev->name);
		}
	}

	lp50xx_dev = DEVICE_DT_GET_ANY(ti_lp5012);
	if (lp50xx_dev) {
		LOG_INF("Found LED controller %s", lp50xx_dev->name);
		found = true;

		if (device_is_ready(lp50xx_dev)) {
			run_test(lp50xx_dev,
				 LP5012_MAX_LEDS,
				 LP50XX_LED_BRIGHT_CHAN_BASE,
				 LP5012_LED_COL_CHAN_BASE);
		} else {
			LOG_ERR("LED controller %s is not ready",
				lp50xx_dev->name);
		}
	}

	lp50xx_dev = DEVICE_DT_GET_ANY(ti_lp5018);
	if (lp50xx_dev) {
		LOG_INF("Found LED controller %s", lp50xx_dev->name);
		found = true;

		if (device_is_ready(lp50xx_dev)) {
			run_test(lp50xx_dev,
				 LP5018_MAX_LEDS,
				 LP50XX_LED_BRIGHT_CHAN_BASE,
				 LP5024_LED_COL_CHAN_BASE);
		} else {
			LOG_ERR("LED controller %s is not ready",
				lp50xx_dev->name);
		}
	}

	lp50xx_dev = DEVICE_DT_GET_ANY(ti_lp5024);
	if (lp50xx_dev) {
		LOG_INF("Found LED controller %s", lp50xx_dev->name);
		found = true;

		if (device_is_ready(lp50xx_dev)) {
			run_test(lp50xx_dev,
				 LP5024_MAX_LEDS,
				 LP50XX_LED_BRIGHT_CHAN_BASE,
				 LP5024_LED_COL_CHAN_BASE);
		} else {
			LOG_ERR("LED controller %s is not ready",
				lp50xx_dev->name);
		}
	}

	lp50xx_dev = DEVICE_DT_GET_ANY(ti_lp5030);
	if (lp50xx_dev) {
		LOG_INF("Found LED controller %s", lp50xx_dev->name);
		found = true;

		if (device_is_ready(lp50xx_dev)) {
			run_test(lp50xx_dev,
				 LP5030_MAX_LEDS,
				 LP50XX_LED_BRIGHT_CHAN_BASE,
				 LP5036_LED_COL_CHAN_BASE);
		} else {
			LOG_ERR("LED controller %s is not ready",
				lp50xx_dev->name);
		}
	}

	lp50xx_dev = DEVICE_DT_GET_ANY(ti_lp5036);
	if (lp50xx_dev) {
		LOG_INF("Found LED controller %s", lp50xx_dev->name);
		found = true;

		if (device_is_ready(lp50xx_dev)) {
			run_test(lp50xx_dev,
				 LP5036_MAX_LEDS,
				 LP50XX_LED_BRIGHT_CHAN_BASE,
				 LP5036_LED_COL_CHAN_BASE);
		} else {
			LOG_ERR("LED controller %s is not ready",
				lp50xx_dev->name);
		}
	}

	if (!found) {
		LOG_ERR("No LP50XX LED controller found");
	}
	return 0;
}
