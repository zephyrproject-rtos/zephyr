/*
 * Copyright (c) 2021 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_ICARUS_SOM_LOG_LEVEL);

#define SIM_SELECT_PIN 12

static void select_sim(void)
{
	const struct device *port = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));

	if (!port) {
		LOG_ERR("Could not get GPIO Device Binding");

		return;
	}

	#ifdef CONFIG_BOARD_SELECT_SIM_EXTERNAL
		gpio_pin_configure(port, SIM_SELECT_PIN, GPIO_OUTPUT_LOW);
		LOG_INF("External SIM is selected");
	#else
		gpio_pin_configure(port, SIM_SELECT_PIN, GPIO_OUTPUT_HIGH);
		LOG_INF("eSIM is selected");
	#endif
}

static int board_actinius_icarus_som_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	select_sim();

	return 0;
}

/* Needs to happen after GPIO driver init */
SYS_INIT(board_actinius_icarus_som_init, POST_KERNEL, 99);
