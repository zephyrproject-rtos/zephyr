/*
 * Copyright (c) 2026 Kristina Kisiuk, Aliensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

/* RGB color values (0-255 per channel) */
static const uint8_t red[]   = { 0xff, 0x00, 0x00 };
static const uint8_t green[] = { 0x00, 0xff, 0x00 };
static const uint8_t blue[]  = { 0x00, 0x00, 0xff };
static const uint8_t white[] = { 0xff, 0xff, 0xff };
static const uint8_t off[]   = { 0x00, 0x00, 0x00 };

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(ti_lp5815);
	int ret;

	if (!dev) {
		LOG_ERR("No device with compatible ti,lp5815 found");
		return 0;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("LED device %s not ready", dev->name);
		return 0;
	}

	LOG_INF("Found LED device %s", dev->name);

	/* Cycle through colors using set_color (for RGB LED at index 0) */
	LOG_INF("Setting red");
	ret = led_set_color(dev, 0, 3, red);
	if (ret < 0) {
		LOG_ERR("Failed to set color: %d", ret);
		return 0;
	}
	k_sleep(K_SECONDS(2));

	LOG_INF("Setting green");
	led_set_color(dev, 0, 3, green);
	k_sleep(K_SECONDS(2));

	LOG_INF("Setting blue");
	led_set_color(dev, 0, 3, blue);
	k_sleep(K_SECONDS(2));

	LOG_INF("Setting white");
	led_set_color(dev, 0, 3, white);
	k_sleep(K_SECONDS(2));

	/* Blink the LED using the autonomous animation engine */
	LOG_INF("Blinking (500ms on, 500ms off)");
	ret = led_blink(dev, 0, 500, 500);
	if (ret < 0) {
		LOG_ERR("Failed to blink: %d", ret);
		return 0;
	}
	k_sleep(K_SECONDS(5));

	/* Stop blinking by setting color (returns to manual mode) */
	LOG_INF("Stopping blink");
	led_set_color(dev, 0, 3, off);

	return 0;
}
