/*
 * Copyright (c) 2021-2022 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_ICARUS_BEE_LOG_LEVEL);

static int board_actinius_icarus_bee_init(const struct device *dev)
{
	const struct gpio_dt_spec sim =
		GPIO_DT_SPEC_GET(DT_NODELABEL(sim_select), sim_gpios);

	ARG_UNUSED(dev);

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

/* Needs to happen after GPIO driver init */
SYS_INIT(board_actinius_icarus_bee_init, POST_KERNEL, 99);
