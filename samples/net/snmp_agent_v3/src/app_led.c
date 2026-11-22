/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_led.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_led, LOG_LEVEL_INF);

#if DT_NODE_EXISTS(DT_ALIAS(led0))
static const struct gpio_dt_spec s_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static bool s_hardware_available;
#endif

static int s_led_state;

int app_led_init(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(led0))
	if (!gpio_is_ready_dt(&s_led)) {
		LOG_WRN("User LED device %s not ready", s_led.port->name);
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&s_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {

		LOG_ERR("Failed to configure User LED pin: %d", ret);
		return ret;
	}

	s_hardware_available = true;
	s_led_state = 0;
	LOG_INF("User LED initialized on %s pin %d", s_led.port->name, s_led.pin);
	return 0;
#else
	LOG_WRN("No 'led0' alias found in DeviceTree; simulating LED in software");
	s_led_state = 0;
	return 0;
#endif
}

int app_led_get_state(void)
{
	return s_led_state;
}

int app_led_set_state(int state)
{
	if (state != 0 && state != 1) {
		LOG_WRN("Invalid LED state requested: %d (must be 0 or 1)", state);
		return -EINVAL;
	}

	s_led_state = state;

#if DT_NODE_EXISTS(DT_ALIAS(led0))
	if (s_hardware_available) {
		int ret = gpio_pin_set_dt(&s_led, state);
		if (ret < 0) {

			LOG_ERR("Failed to set physical LED pin: %d", ret);
			return ret;
		}
	}
#endif

	LOG_INF("Physical User LED changed to: %s (%d)", state ? "ON" : "OFF", state);
	return 0;
}
