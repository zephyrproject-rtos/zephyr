/*
 * Copyright (c) 2023 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define LED_RGB_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(rgb_leds)

const uint8_t led_idx[] = {DT_FOREACH_CHILD_SEP(LED_RGB_NODE_ID, DT_NODE_CHILD_IDX, (,))};

static int num_leds = ARRAY_SIZE(led_idx);

static const uint8_t colors[][3] = {
	{0xFF, 0x00, 0x00}, /* Red    */
	{0x00, 0xFF, 0x00}, /* Green  */
	{0x00, 0x00, 0xFF}, /* Blue   */
	{0xFF, 0xFF, 0x00}, /* Yellow */
	{0xFF, 0x00, 0xFF}, /* Purple */
	{0x00, 0xFF, 0xFF}, /* Cyan   */
	{0xFF, 0xFF, 0xFF}, /* White  */
};

/**
 * @brief Demonstrates the use of an RGB LED using the LED API.
 *
 * @param dev LED device.
 * @param led Number of the LED to demo.
 */
static void led_demo(const struct device *dev, uint8_t led)
{
	LOG_INF("Demoing LED %d", led);

	led_on(dev, led);
	k_sleep(K_MSEC(1000));

	led_off(dev, led);
	k_sleep(K_MSEC(1000));

	/* Set a color and fade in with setting the brightness.
	 * PWM led's will fade, while GPIO led's will be fully on or off.
	 */
	for (uint8_t color = 0; color <= ARRAY_SIZE(colors); color++) {
		led_set_color(dev, led, 3, colors[color]);
		for (uint8_t level = 0; level <= 100; level++) {
			led_set_brightness(dev, led, level);
			k_sleep(K_MSEC(20));
		}
	}

	led_on(dev, led);
	led_blink(dev, led, 500, 500);
	k_sleep(K_MSEC(5000));
	led_off(dev, led);
}

int main(void)
{
	const struct device *dev;
	uint8_t led;

	dev = DEVICE_DT_GET(LED_RGB_NODE_ID);
	if (!device_is_ready(dev)) {
		LOG_ERR("Device %s is not ready", dev->name);
		return 0;
	}

	if (!num_leds) {
		LOG_ERR("No LEDs found for %s", dev->name);
		return 0;
	}

	for (led = 0; led < num_leds; led++) {
		led_demo(dev, led);
	}

	return 0;
}
