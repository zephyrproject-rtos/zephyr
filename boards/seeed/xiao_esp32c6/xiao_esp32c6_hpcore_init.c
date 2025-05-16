/*
 * Copyright (c) 2025 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

static int board_init(void)
{
	const struct gpio_dt_spec rf_switch_en =
		GPIO_DT_SPEC_GET(DT_NODELABEL(rf_switch_en), gpios);
	const struct gpio_dt_spec ext_ant_sel = GPIO_DT_SPEC_GET(DT_NODELABEL(ext_ant), gpios);

	if (!device_is_ready(rf_switch_en.port)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&rf_switch_en, GPIO_OUTPUT);

	if (!device_is_ready(ext_ant_sel.port)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&ext_ant_sel, GPIO_OUTPUT);

	gpio_pin_set_dt(&rf_switch_en, 1);

#ifdef CONFIG_XIAO_ESP32C6_EXT_ANTENNA
	gpio_pin_set_dt(&ext_ant_sel, 1);
#else
	gpio_pin_set_dt(&ext_ant_sel, 0);
#endif

	return 0;
}

SYS_INIT(board_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
