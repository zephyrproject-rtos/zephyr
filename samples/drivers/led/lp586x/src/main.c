/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Draeger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define SLEEP_DELAY_MS 500
#define FADE_DELAY_MS  2

#define SLEEP_DELAY K_MSEC(SLEEP_DELAY_MS)
#define FADE_DELAY  K_MSEC(FADE_DELAY_MS)

#define MAX_CHANNELS_LP5860 (11 * 18)
#define MAX_CHANNELS_LP5861 (1 * 18)
#define MAX_CHANNELS_LP5862 (1 * 18)
#define MAX_CHANNELS_LP5864 (4 * 18)
#define MAX_CHANNELS_LP5866 (6 * 18)
#define MAX_CHANNELS_LP5868 (8 * 18)

/*
 * Fade all LED dots simultaneously using write_channels.
 *
 * The channel values written via led_write_channels map directly to the 8-bit
 * PWM brightness registers.
 */
static int run_channel_test(const struct device *dev, uint8_t num_leds)
{
	/* Static buffer sized for the largest LP586x variant (LP5860, 198 dots) */
	static uint8_t buf[MAX_CHANNELS_LP5860];
	int err;

	LOG_INF("Channel test: fading %d LED dots together", num_leds);

	for (int level = 0; level <= 0xFF; level++) {
		memset(buf, (uint8_t)level, num_leds);
		err = led_write_channels(dev, 0, num_leds, buf);
		if (err < 0) {
			LOG_ERR("Failed to write channels, err=%d", err);
			return err;
		}
		k_sleep(FADE_DELAY);
	}
	k_sleep(SLEEP_DELAY);

	memset(buf, 0, num_leds);
	err = led_write_channels(dev, 0, num_leds, buf);
	if (err < 0) {
		LOG_ERR("Failed to clear channels, err=%d", err);
		return err;
	}
	k_sleep(SLEEP_DELAY);

	return 0;
}

/*
 * Fade a single configured LED node using the LED API (set_brightness).
 */
static int run_set_brightness_test(const struct device *dev, uint8_t led)
{
	const struct led_info *info;
	int err;

	led_get_info(dev, led, &info);
	if (info->num_colors > 1) {
		LOG_INF("LED test: not fading multi-color LED %d via set_brightness", led);
		return 0;
	}

	LOG_INF("LED test: fading LED %d", led);

	for (int level = 0; level <= LED_BRIGHTNESS_MAX; level++) {
		err = led_set_brightness(dev, led, (uint8_t)level);
		if (err < 0) {
			LOG_ERR("Failed to set brightness %d on LED %d, err=%d", level, led, err);
			return err;
		}
		k_sleep(FADE_DELAY);
	}
	k_sleep(SLEEP_DELAY);

	err = led_off(dev, led);
	if (err < 0) {
		LOG_ERR("Failed to turn LED %d off, err=%d", led, err);
		return err;
	}
	k_sleep(SLEEP_DELAY);

	return 0;
}

/*
 * Fade a single configured multi-color LED node using the LED API (set_color).
 */
static int run_set_color_test(const struct device *dev, uint8_t led)
{
	const struct led_info *info;
	int err;

	led_get_info(dev, led, &info);

	if (info->num_colors == 1) {
		LOG_INF("LED test: not fading monochromatic LED %d via set_color", led);
		return 0;
	}

	uint8_t colors[info->num_colors];

	memset(colors, 0, info->num_colors);

	for (uint8_t color = 0; color < info->num_colors; color++) {
		LOG_INF("LED test: fading multi-color LED %d color %d", led, color);
		for (int level = 0; level <= 0xFF; level++) {
			colors[color] = (uint8_t)level;
			err = led_set_color(dev, led, info->num_colors, colors);
			if (err < 0) {
				LOG_ERR("Failed to set color %d to %d on LED %d, err=%d", color,
					level, led, err);
				return err;
			}
			k_sleep(FADE_DELAY);
		}
		memset(colors, 0, info->num_colors);
		k_sleep(SLEEP_DELAY);
	}

	memset(colors, 0, info->num_colors);
	err = led_set_color(dev, led, info->num_colors, colors);
	if (err < 0) {
		LOG_ERR("Failed to turn multi-color LED %d off, err=%d", led, err);
		return err;
	}

	k_sleep(SLEEP_DELAY);

	return 0;
}

static int run_test(const struct device *dev, uint8_t max_channels)
{
	uint8_t led;
	uint8_t num_leds = 0;
	int err;

	/* Print information for each configured LED node */
	for (led = 0; led < max_channels; led++) {
		const struct led_info *info;
		int col;

		err = led_get_info(dev, led, &info);
		if (err < 0) {
			break;
		}

		printk("Found LED %d", led);
		if (info->label) {
			printk(" - %s", info->label);
		}
		printk(" - index:%d - %d color(s)", info->index, info->num_colors);
		if (info->color_mapping) {
			printk(" - %d", info->color_mapping[0]);
			for (col = 1; col < info->num_colors; col++) {
				printk(":%d", info->color_mapping[col]);
			}
		}
		printk("\n");
		num_leds++;
	}

	if (!num_leds) {
		LOG_ERR("No LED nodes found");
		return 0;
	}

	do {
		err = run_channel_test(dev, max_channels);
		if (err) {
			return 0;
		}
		for (led = 0; led < num_leds; led++) {
			err = run_set_brightness_test(dev, led);
			if (err) {
				return 0;
			}
		}
		for (led = 0; led < num_leds; led++) {
			err = run_set_color_test(dev, led);
			if (err) {
				return 0;
			}
		}
	} while (true);

	return 0;
}

int main(void)
{
	const struct device *lp586x_dev;
	int max_channels = MAX_CHANNELS_LP5860;

	k_sleep(K_MSEC(3000));

	lp586x_dev = DEVICE_DT_GET_ANY(ti_lp5860);
	if (!lp586x_dev) {
		lp586x_dev = DEVICE_DT_GET_ANY(ti_lp5861);
		max_channels = MAX_CHANNELS_LP5861;
	}
	if (!lp586x_dev) {
		lp586x_dev = DEVICE_DT_GET_ANY(ti_lp5862);
		max_channels = MAX_CHANNELS_LP5862;
	}
	if (!lp586x_dev) {
		lp586x_dev = DEVICE_DT_GET_ANY(ti_lp5864);
		max_channels = MAX_CHANNELS_LP5864;
	}
	if (!lp586x_dev) {
		lp586x_dev = DEVICE_DT_GET_ANY(ti_lp5866);
		max_channels = MAX_CHANNELS_LP5866;
	}
	if (!lp586x_dev) {
		lp586x_dev = DEVICE_DT_GET_ANY(ti_lp5868);
		max_channels = MAX_CHANNELS_LP5868;
	}
	if (!lp586x_dev) {
		LOG_ERR("No LP586x LED matrix controller found");
		return -ENODEV;
	}

	LOG_INF("Found %s", lp586x_dev->name);
	if (device_is_ready(lp586x_dev)) {
		run_test(lp586x_dev, max_channels);
	} else {
		LOG_ERR_DEVICE_NOT_READY(lp586x_dev);
		return -ENODEV;
	}

	return 0;
}
