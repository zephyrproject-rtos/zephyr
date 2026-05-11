/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_button, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>

#include "button.h"
#include "bmc_init.h"
#include "config.h"

#define USER_BUTTON_NODE	DT_ALIAS(reset_button)
#if DT_NODE_HAS_STATUS_OKAY(USER_BUTTON_NODE)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(USER_BUTTON_NODE, gpios,
							      {0});

static void reset_work_fn(struct k_work *work)
{
	int val = gpio_pin_get_dt(&button);
	int rc;

	if (val != 1)
		return;

	LOG_INF("Button held, rebooting.");

	rc = config_clear();
	if (rc) {
		LOG_ERR("Error: could not clear config");
		return;
	}

	bmc_reboot();
}

static K_WORK_DELAYABLE_DEFINE(reset_work, reset_work_fn);

#define RESET_HOLD_TIME_MS 1000

static void button_work_fn(struct k_work *work)
{
	int val = gpio_pin_get_dt(&button);
	if (val == 1) {
		/* Pressed */
		k_work_schedule(&reset_work, K_MSEC(RESET_HOLD_TIME_MS));
	} else if (val == 0) {
		/* Released */
		k_work_cancel_delayable(&reset_work);
		LOG_INF("Hold button for 1s to clear config and reset");
	}
}

static K_WORK_DELAYABLE_DEFINE(button_work, button_work_fn);

#define DEBOUNCE_TIME_MS 1

static struct gpio_callback button_cb_data;

static void button_pressed(const struct device *dev,
			   struct gpio_callback *cb, uint32_t pins)
{
	k_work_schedule(&button_work, K_MSEC(DEBOUNCE_TIME_MS));
}

int button_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Error: button device %s is not ready",
			button.port->name);
		return -ENOSYS;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error: button device %s failed to configure pin %d (err=%d)",
			button.port->name, button.pin, ret);
		return ret;
	}

	/* Interrupt on press and release */
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Error: button device %s failed to configure interrupt on pin %d (err=%d)",
			button.port->name, button.pin, ret);
		return ret;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	LOG_INF("Button device initialised");

	return 0;
}
#else
int button_init(void)
{
	LOG_INF("Button device not defined for this board");
	return 0;
}
#endif
