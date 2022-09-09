/*
 * Copyright (c) 2018 Workaround GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define NUM_LEDS 4
#define BLINK_DELAY_ON 500
#define BLINK_DELAY_OFF 500
#define DELAY_TIME 2000
#define COLORS_TO_SHOW 8
#define VALUES_PER_COLOR 3

/*
 * We assume that the colors are connected the way it is described in the driver
 * datasheet.
 */
#define LED_R 2
#define LED_G 1
#define LED_B 0

/*
 * The following colors are shown in the given order.
 */
static uint8_t colors[COLORS_TO_SHOW][VALUES_PER_COLOR] = {
	{ 0xFF, 0x00, 0x00 }, /*< Red    */
	{ 0x00, 0xFF, 0x00 }, /*< Green  */
	{ 0x00, 0x00, 0xFF }, /*< Blue   */
	{ 0xFF, 0xFF, 0xFF }, /*< White  */
	{ 0xFF, 0xFF, 0x00 }, /*< Yellow */
	{ 0xFF, 0x00, 0xFF }, /*< Purple */
	{ 0x00, 0xFF, 0xFF }, /*< Cyan   */
	{ 0xF4, 0x79, 0x20 }, /*< Orange */
};

/*
 * @brief scale hex values [0x00..0xFF] to percent [0..100].
 *
 * @param hex Hex value.
 *
 * @return Hex value scaled to percent.
 */
static inline uint8_t scale_color_to_percent(uint8_t hex)
{
	return (hex * 100U) / 0xFF;
}

/*
 * @brief Set a color using the three RGB LEDs.
 *
 * @param dev LED device.
 * @param r Red value in hex [0x00..0xFF].
 * @param g Green value in hex [0x00..0xFF].
 * @param b Blue value in hex [0x00..0xFF].
 *
 * @return 0 if successful, -ERRNO otherwise.
 */
static int set_static_color(const struct device *dev, uint8_t r, uint8_t g,
			    uint8_t b)
{
	int ret;

	r = scale_color_to_percent(r);
	g = scale_color_to_percent(g);
	b = scale_color_to_percent(b);

	ret = led_set_brightness(dev, LED_R, r);
	if (ret) {
		LOG_ERR("Failed to set color.");
		return ret;
	}

	ret = led_set_brightness(dev, LED_G, g);
	if (ret) {
		LOG_ERR("Failed to set color.");
		return ret;
	}

	ret = led_set_brightness(dev, LED_B, b);
	if (ret) {
		LOG_ERR("Failed to set color.");
		return ret;
	}

	return 0;
}

/*
 * @brief Blink the LED in the given color.
 *
 * Currently it is only possible to create colors by turning the three LEDs
 * completely on or off. Proper mixing is not supported by the underlying API.
 *
 * @param dev LED device.
 * @param r Blink the red LED.
 * @param g Blink the green LED.
 * @param b Blink the blue LED.
 * @param delay_on Time in ms the LEDs stay on.
 * @param delay_off Time in ms the LEDs stay off.
 *
 * @return 0 if successful, -ERRNO otherwise.
 */
static int blink_color(const struct device *dev, bool r, bool g, bool b,
		       uint32_t delay_on, uint32_t delay_off)
{
	int ret;

	if (r) {
		ret = led_blink(dev, LED_R, delay_on, delay_off);
		if (ret) {
			LOG_ERR("Failed to set color.");
			return ret;
		}
	}

	if (g) {
		ret = led_blink(dev, LED_G, delay_on, delay_off);
		if (ret) {
			LOG_ERR("Failed to set color.");
			return ret;
		}
	}

	if (b) {
		ret = led_blink(dev, LED_B, delay_on, delay_off);
		if (ret) {
			LOG_ERR("Failed to set color.");
			return ret;
		}
	}

	return 0;
}

/*
 * @brief Helper function to turn off all LEDs connected to the driver.
 *
 * @param dev LED driver device.
 *
 * @return 0 if successful, -ERRNO otherwise.
 */
static int turn_off_all_leds(const struct device *dev)
{
	for (int i = 0; i < NUM_LEDS; i++) {
		int ret = led_off(dev, i);

		if (ret) {
			return ret;
		}
	}

	return 0;
}

void main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(ti_lp5562);
	int i, ret;

	if (!dev) {
		LOG_ERR("No \"ti,lp5562\" device found");
		return;
	} else if (!device_is_ready(dev)) {
		LOG_ERR("LED device %s is not ready", dev->name);
		return;
	} else {
		LOG_INF("Found LED device %s", dev->name);
	}

	LOG_INF("Testing leds");

	while (1) {

		for (i = 0; i < COLORS_TO_SHOW; i++) {
			ret = set_static_color(dev,
					colors[i][0],
					colors[i][1],
					colors[i][2]);
			if (ret) {
				return;
			}

			k_msleep(DELAY_TIME);
		}

		ret = turn_off_all_leds(dev);
		if (ret < 0) {
			return;
		}

		/* Blink white. */
		ret = blink_color(dev, true, true, true, BLINK_DELAY_ON,
				BLINK_DELAY_OFF);
		if (ret) {
			return;
		}

		/* Wait a few blinking before turning off the LEDs */
		k_msleep(DELAY_TIME * 2);

		/* Change the color of the LEDs while keeping blinking. */
		for (i = 0; i < COLORS_TO_SHOW; i++) {
			ret = set_static_color(dev,
					colors[i][0],
					colors[i][1],
					colors[i][2]);
			if (ret) {
				return;
			}

			k_msleep(DELAY_TIME * 2);
		}

		ret = turn_off_all_leds(dev);
		if (ret < 0) {
			return;
		}

		k_msleep(DELAY_TIME);
	}
}
