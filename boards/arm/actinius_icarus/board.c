/*
 * Copyright (c) 2019-2022 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_ICARUS_LOG_LEVEL);

#define CHARGER_ENABLE_NODE DT_NODELABEL(charger_enable)

static int set_sim_select_pin(void)
{
	const struct gpio_dt_spec sim =
		GPIO_DT_SPEC_GET(DT_NODELABEL(sim_select), sim_gpios);

	if (!device_is_ready(sim.port)) {
		LOG_ERR("The SIM Select Pin port is not ready");
		return -ENODEV;
	}

	if (DT_ENUM_IDX(DT_NODELABEL(sim_select), sim) == 0) {
		(void)gpio_pin_configure_dt(&sim, GPIO_OUTPUT_HIGH);
		LOG_INF("eSIM is selected");
	} else {
		(void)gpio_pin_configure_dt(&sim, GPIO_OUTPUT_LOW);
		LOG_INF("External SIM is selected");
	}

	return 0;
}

#if DT_NODE_EXISTS(CHARGER_ENABLE_NODE)

static int set_charger_enable_pin(void)
{
	const struct gpio_dt_spec charger_en =
		GPIO_DT_SPEC_GET(CHARGER_ENABLE_NODE, gpios);

	if (!device_is_ready(charger_en.port)) {
		LOG_ERR("The Charger Enable Pin port is not ready");
		return -ENODEV;
	}

	if (DT_ENUM_IDX(CHARGER_ENABLE_NODE, charger) == 0) {
		(void)gpio_pin_configure_dt(&charger_en, GPIO_OUTPUT_LOW);
		LOG_INF("Charger is set to auto");
	} else {
		(void)gpio_pin_configure_dt(&charger_en, GPIO_OUTPUT_HIGH);
		LOG_INF("Charger is disabled");
	}

	return 0;
}

#endif /* DT_NODE_EXISTS(CHARGER_ENABLE_NODE) */

static int board_actinius_icarus_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int result;

	result = set_sim_select_pin();
	if (result < 0) {
		LOG_ERR("Failed to set the SIM Select Pin (error: %d)", result);
		/* do not return so that the rest of the init process is attempted */
	}

#if DT_NODE_EXISTS(CHARGER_ENABLE_NODE)
	result = set_charger_enable_pin();
	if (result < 0) {
		LOG_ERR("Failed to set the Charger Enable Pin (error: %d)", result);
		/* do not return so that the rest of the init process is attempted */
	}
#endif /* DT_NODE_EXISTS(CHARGER_ENABLE_NODE) */

	return result;
}

/* Needs to happen after GPIO driver init */
SYS_INIT(board_actinius_icarus_init, POST_KERNEL, 99);
