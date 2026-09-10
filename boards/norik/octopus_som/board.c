/*
 * Copyright (c) 2024 Norik Systems
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(board_control, CONFIG_OCTOPUS_SOM_CONTROL_LOG_LEVEL);

#define SIM_SELECT_NODE DT_PATH(sim_select)

void board_late_init_hook(void)
{
	const struct gpio_dt_spec simctrl = GPIO_DT_SPEC_GET(DT_PATH(sim_select), sim_gpios);

	if (!gpio_is_ready_dt(&simctrl)) {
		LOG_ERR("SIM select GPIO not available");
		return;
	}

	if (DT_ENUM_IDX(SIM_SELECT_NODE, sim) == 0) {
		(void)gpio_pin_configure_dt(&simctrl, GPIO_OUTPUT_LOW);
		LOG_INF("On-board SIM selected");
	} else {
		(void)gpio_pin_configure_dt(&simctrl, GPIO_OUTPUT_HIGH);
		LOG_INF("External SIM selected");
	}
}
