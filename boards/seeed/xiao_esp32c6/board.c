/*
 * Copyright (c) 2025 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

void board_late_init_hook(void)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rf_switch))
	const struct gpio_dt_spec rf_switch_en =
		GPIO_DT_SPEC_GET(DT_NODELABEL(rf_switch), enable_gpios);

	if (gpio_is_ready_dt(&rf_switch_en)) {
		gpio_pin_configure_dt(&rf_switch_en, GPIO_OUTPUT_ACTIVE);
	}

	const struct gpio_dt_spec ant_sel = GPIO_DT_SPEC_GET(DT_NODELABEL(rf_switch), select_gpios);

	if (gpio_is_ready_dt(&ant_sel)) {
#ifdef CONFIG_XIAO_ESP32C6_EXT_ANTENNA
		gpio_pin_configure_dt(&ant_sel, GPIO_OUTPUT_ACTIVE);
#else
		gpio_pin_configure_dt(&ant_sel, GPIO_OUTPUT_INACTIVE);
#endif /* XIAO_ESP32C6_EXT_ANTENNA */
	}

#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rf_switch)) */
}
