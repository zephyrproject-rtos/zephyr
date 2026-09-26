/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(reterminal_sticky, CONFIG_LOG_DEFAULT_LEVEL);

#define POWER_LONGPRESS_NODE DT_NODELABEL(power_longpress)
#define PWR_HOLD_NODE        DT_NODELABEL(pwr_hold)
#define PWR_LOCK_NODE        DT_NODELABEL(pwr_lock)

/* Well above the ns range setup and hold times of the 74AHC1G79 latch */
#define PWR_LATCH_DELAY_US 10

#define GPIO_HOG_DT_SPEC(node_id)                                                                  \
	{                                                                                          \
		.port = DEVICE_DT_GET(DT_PARENT(node_id)),                                         \
		.pin = DT_GPIO_HOG_PIN_BY_IDX(node_id, 0),                                         \
		.dt_flags = DT_GPIO_HOG_FLAGS_BY_IDX(node_id, 0),                                  \
	}

static const struct gpio_dt_spec pwr_hold = GPIO_HOG_DT_SPEC(PWR_HOLD_NODE);
static const struct gpio_dt_spec pwr_lock = GPIO_HOG_DT_SPEC(PWR_LOCK_NODE);

static void power_longpress_cb(struct input_event *evt, void *user_data)
{
	int err;

	ARG_UNUSED(user_data);

	if ((evt->type != INPUT_EV_KEY) ||
	    (evt->code != DT_PROP_BY_IDX(POWER_LONGPRESS_NODE, long_codes, 0)) ||
	    (evt->value == 0)) {
		return;
	}

	/*
	 * Clock a low PWR_HOLD level into the power latch. When running from
	 * the battery, the system switches off once the power button is
	 * released. USB power keeps the system on regardless of the latch.
	 */
	err = gpio_pin_configure_dt(&pwr_hold, GPIO_OUTPUT_INACTIVE);
	if (err == 0) {
		err = gpio_pin_configure_dt(&pwr_lock, GPIO_OUTPUT_INACTIVE);
	}

	if (err != 0) {
		LOG_ERR("Failed to configure power latch GPIOs (err %d)", err);
		return;
	}

	LOG_INF("Power button held, releasing power latch");

	k_busy_wait(PWR_LATCH_DELAY_US);
	(void)gpio_pin_set_dt(&pwr_lock, 1);
	k_busy_wait(PWR_LATCH_DELAY_US);
	(void)gpio_pin_set_dt(&pwr_lock, 0);
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(POWER_LONGPRESS_NODE), power_longpress_cb, NULL);
