/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_ICARUS_LOG_LEVEL);

#define SIM_SELECT_PIN 8

static void select_sim(void)
{
	struct device *port = device_get_binding(DT_GPIO_P0_DEV_NAME);

	if (!port) {
		LOG_ERR("Could not get GPIO Device Binding");

		return;
	}

	gpio_pin_configure(port, SIM_SELECT_PIN, GPIO_DIR_OUT);
	#ifdef CONFIG_BOARD_SELECT_SIM_EXTERNAL
		gpio_pin_write(port, SIM_SELECT_PIN, 0);
		LOG_INF("External SIM is selected");
	#else
		gpio_pin_write(port, SIM_SELECT_PIN, 1);
		LOG_INF("eSIM is selected");
	#endif
}

static int board_actinius_icarus_init(struct device *dev)
{
	ARG_UNUSED(dev);

	select_sim();

	return 0;
}

SYS_INIT(board_actinius_icarus_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
