/** @file
 *  @brief RGB LED Service sample
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 STMicroelectronics
 */

#include "led_svc.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/led_strip.h>

LOG_MODULE_REGISTER(led_svc);

#define STRIP_NODE		DT_ALIAS(led_strip)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct led_rgb blue = {
	.r = 0x00,
	.g = 0x00,
	.b = 0x10,
};

static struct led_rgb led;

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

static bool led_state; /* Tracking state here supports GPIO expander-based LEDs. */
static bool led_ok;

void led_update(void)
{
	int rc;

	if (!led_ok) {
		return;
	}

	led_state = !led_state;
	LOG_INF("Turn %s LED", led_state ? "on" : "off");

	memset(&led, 0x00, sizeof(struct led_rgb));
	if (led_state) {
		memcpy(&led, &blue, sizeof(struct led_rgb));
	}

	rc = led_strip_update_rgb(strip, &led, 1);
	if (rc) {
		LOG_ERR("couldn't update strip: %d", rc);
	}

}

int led_init(void)
{

	led_ok = device_is_ready(strip);
	if (led_ok) {
		LOG_INF("Found LED strip device %s", strip->name);
	} else {
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return 1;
	}

	return 0;
}
