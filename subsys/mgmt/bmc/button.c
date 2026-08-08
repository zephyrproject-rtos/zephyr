/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Holding the reset button erases the stored configuration and reboots, which
 * is the recovery path when the BMC is no longer reachable over the network.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/config.h>

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

#define RESET_BUTTON_NODE DT_ALIAS(bmc_reset_button)

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(RESET_BUTTON_NODE),
	     "CONFIG_BMC_RESET_BUTTON needs an enabled bmc-reset-button devicetree alias");

#define DEBOUNCE_TIME_MS 1

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(RESET_BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

static void reset_work_fn(struct k_work *work)
{
	int ret;

	ARG_UNUSED(work);

	if (gpio_pin_get_dt(&button) != 1) {
		return;
	}

	LOG_INF("Reset button held, clearing the configuration and rebooting");

	ret = bmc_config_clear();
	if (ret < 0) {
		LOG_ERR("Could not clear the configuration (err=%d)", ret);
		return;
	}

	bmc_reboot();
}

static K_WORK_DELAYABLE_DEFINE(reset_work, reset_work_fn);

static void button_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	if (gpio_pin_get_dt(&button) == 1) {
		k_work_schedule(&reset_work, K_MSEC(CONFIG_BMC_RESET_BUTTON_HOLD_MS));
		LOG_INF("Hold the reset button for %d ms to clear the configuration and reboot",
			CONFIG_BMC_RESET_BUTTON_HOLD_MS);
	} else {
		k_work_cancel_delayable(&reset_work);
	}
}

static K_WORK_DELAYABLE_DEFINE(button_work, button_work_fn);

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	k_work_schedule(&button_work, K_MSEC(DEBOUNCE_TIME_MS));
}

static int button_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Reset button device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure the reset button pin (err=%d)", ret);
		return ret;
	}

	/* Interrupt on both press and release so that the hold can be timed. */
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Could not configure the reset button interrupt (err=%d)", ret);
		return ret;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));

	ret = gpio_add_callback(button.port, &button_cb_data);
	if (ret < 0) {
		LOG_ERR("Could not add the reset button callback (err=%d)", ret);
		return ret;
	}

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_reset_button, BMC_INIT_PHASE_PLATFORM, button_init, true);
