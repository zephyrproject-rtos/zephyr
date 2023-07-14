/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/lp50xx.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define MAX_BRIGHTNESS 100

#define SLEEP_DELAY_MS 1000
#define FADE_DELAY_MS  10

#define FADE_DELAY  K_MSEC(FADE_DELAY_MS)
#define SLEEP_DELAY K_MSEC(SLEEP_DELAY_MS)

/*
 * The following colors are shown in the given order.
 */
static uint8_t colors[][3] = {
	{0xFF, 0x00, 0x00}, /* Red    */
	{0x00, 0xFF, 0x00}, /* Green  */
	{0x00, 0x00, 0xFF}, /* Blue   */
	{0xFF, 0xFF, 0xFF}, /* White  */
	{0xFF, 0xFF, 0x00}, /* Yellow */
	{0xFF, 0x00, 0xFF}, /* Purple */
	{0x00, 0xFF, 0xFF}, /* Cyan   */
	{0xF4, 0x79, 0x20}, /* Orange */
};

/**
 * @brief Run tests on a single LED using the LED-based API syscalls.
 *
 * @param lp50xx_dev LP50XX LED controller device.
 * @param led Number of the LED to test.
 */
static int run_led_test(const struct device *lp50xx_dev, uint8_t led)
{
	uint8_t idx;
	int err;

	LOG_INF("Testing LED %d (LED API)", led);

	for (idx = 0; idx < ARRAY_SIZE(colors); idx++) {
		uint16_t level;

		/* Update LED color. */
		err = led_set_color(lp50xx_dev, led, 3, colors[idx]);
		if (err < 0) {
			LOG_ERR("Failed to set LED %d color to "
				"%02x:%02x:%02x, err=%d",
				led, colors[idx][0], colors[idx][1], colors[idx][2], err);
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
					", err=%d\n",
					led, level, err);
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
static int run_channel_test(const struct device *lp50xx_dev)
{
	uint8_t idx;
	enum lp50xx_chips chip = lp50xx_get_chip(lp50xx_dev);
	uint8_t buffer[LP50XX_COLORS_PER_LED * LP50XX_MAX_LEDS(chip)];
	int err;

	LOG_INF("Testing all LEDs (channel API)");

	for (idx = 0; idx < ARRAY_SIZE(colors); idx++) {
		uint8_t led;
		uint16_t level;

		/* Update LEDs colors. */
		for (led = 0; led < LP50XX_MAX_LEDS(chip); led++) {
			uint8_t *col = &buffer[led * 3];

			col[0] = colors[idx][0];
			col[1] = colors[idx][1];
			col[2] = colors[idx][2];
		}
		err = led_write_channels(lp50xx_dev, LP50XX_LED_COL1_CHAN(chip, 0),
					 LP50XX_COLORS_PER_LED * LP50XX_MAX_LEDS(chip), buffer);
		if (err < 0) {
			LOG_ERR("Failed to write channels, start=%d num=%d"
				" err=%d\n",
				LP50XX_LED_COL1_CHAN(chip, 0),
				LP50XX_COLORS_PER_LED * LP50XX_MAX_LEDS(chip), err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs on. */
		for (led = 0; led < LP50XX_MAX_LEDS(chip); led++) {
			buffer[led] = MAX_BRIGHTNESS;
		}
		err = led_write_channels(lp50xx_dev, LP50XX_LED_BRIGHT_CHAN(chip, 0),
					 LP50XX_MAX_LEDS(chip), buffer);
		if (err < 0) {
			LOG_ERR("Failed to write channels, start=%d num=%d"
				" err=%d\n",
				LP50XX_LED_BRIGHT_CHAN(chip, 0), LP50XX_MAX_LEDS(chip), err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs off. */
		for (led = 0; led < LP50XX_MAX_LEDS(chip); led++) {
			buffer[led] = 0;
		}
		err = led_write_channels(lp50xx_dev, LP50XX_LED_BRIGHT_CHAN(chip, 0),
					 LP50XX_MAX_LEDS(chip), buffer);
		if (err < 0) {
			LOG_ERR("Failed to write channels, start=%d num=%d"
				" err=%d\n",
				LP50XX_LED_BRIGHT_CHAN(chip, 0), LP50XX_MAX_LEDS(chip), err);
			return err;
		}
		k_sleep(SLEEP_DELAY);

		/* Set LEDs brightnesses gradually to the maximum level. */
		for (level = 0; level <= MAX_BRIGHTNESS; level++) {
			for (led = 0; led < LP50XX_MAX_LEDS(chip); led++) {
				buffer[led] = level;
			}
			err = led_write_channels(lp50xx_dev, LP50XX_LED_BRIGHT_CHAN(chip, 0),
						 LP50XX_MAX_LEDS(chip), buffer);
			if (err < 0) {
				LOG_ERR("Failed to write channels, start=%d"
					" num=%d err=%d\n",
					LP50XX_LED_BRIGHT_CHAN(chip, 0), LP50XX_MAX_LEDS(chip),
					err);
				return err;
			}
			k_sleep(FADE_DELAY);
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs off. */
		for (led = 0; led < LP50XX_MAX_LEDS(chip); led++) {
			buffer[led] = 0;
		}
		err = led_write_channels(lp50xx_dev, LP50XX_LED_BRIGHT_CHAN(chip, 0),
					 LP50XX_MAX_LEDS(chip), buffer);
		if (err < 0) {
			LOG_ERR("Failed to write channels, start=%d "
				"num=%d err=%d\n",
				LP50XX_LED_BRIGHT_CHAN(chip, 0), LP50XX_MAX_LEDS(chip), err);
			return err;
		}
		k_sleep(SLEEP_DELAY);
	}

	return 0;
}

/**
 * @brief Run tests on all the LEDs using the bank-based calls.
 *
 * @param lp50xx_dev LP50XX LED controller device.
 */
static int run_bank_test(const struct device *lp50xx_dev)
{
	enum lp50xx_chips chip = lp50xx_get_chip(lp50xx_dev);
	int err;

	LOG_INF("Testing all LEDs (bank calls)");

	/* enable bank mode for all LEDs */
	lp50xx_set_bank_mode(lp50xx_dev, (1 << LP50XX_MAX_LEDS(chip)) - 1);

	for (uint8_t idx = 0; idx < ARRAY_SIZE(colors); idx++) {
		/* Update LEDs color. */
		err = lp50xx_set_bank_color(lp50xx_dev, colors[idx]);
		if (err < 0) {
			LOG_ERR("Failed to set color err=%d\n", err);
			goto exit;
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs on. */
		err = lp50xx_set_bank_brightness(lp50xx_dev, MAX_BRIGHTNESS);
		if (err < 0) {
			LOG_ERR("Failed to set brightness err=%d\n", err);
			goto exit;
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs off. */
		err = lp50xx_set_bank_brightness(lp50xx_dev, 0);
		if (err < 0) {
			LOG_ERR("Failed to set brightness err=%d\n", err);
			goto exit;
		}
		k_sleep(SLEEP_DELAY);

		/* Set LEDs brightnesses gradually to the maximum level. */
		for (uint8_t level = 0; level <= MAX_BRIGHTNESS; level++) {
			err = lp50xx_set_bank_brightness(lp50xx_dev, level);
			if (err < 0) {
				LOG_ERR("Failed to set brightness err=%d\n", err);
				goto exit;
			}
			k_sleep(FADE_DELAY);
		}
		k_sleep(SLEEP_DELAY);

		/* Turn LEDs off. */
		err = lp50xx_set_bank_brightness(lp50xx_dev, 0);
		if (err < 0) {
			LOG_ERR("Failed to set brightness err=%d\n", err);
			goto exit;
		}
		k_sleep(SLEEP_DELAY);
	}

exit:
	/* disable bank mode for all LEDs */
	lp50xx_set_bank_mode(lp50xx_dev, 0);

	return err;
}

int main(void)
{
	const struct device *const lp50xx_dev = DEVICE_DT_GET_ANY(ti_lp50xx);

	int err;
	uint8_t led;
	uint8_t num_leds = 0;

	if (!lp50xx_dev) {
		LOG_ERR("No device with compatible ti,lp50xx found");
		return 0;
	} else if (!device_is_ready(lp50xx_dev)) {
		LOG_ERR("LED controller %s is not ready", lp50xx_dev->name);
		return 0;
	}
	LOG_INF("Found LED controller %s", lp50xx_dev->name);

	for (led = 0; led < LP50XX_MAX_LEDS(lp50xx_get_chip(lp50xx_dev)); led++) {
		int col;
		const struct led_info *info;

		err = led_get_info(lp50xx_dev, led, &info);
		if (err < 0) {
			LOG_DBG("Failed to get information for LED %d (err=%d)", led, err);
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
		err = run_bank_test(lp50xx_dev);
		if (err) {
			return 0;
		}
		err = run_channel_test(lp50xx_dev);
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
