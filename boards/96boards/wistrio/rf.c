/*
 * Copyright (c) 2019 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

/* Runs as the board late init hook, after all POST_KERNEL device init,
 * so the GPIO driver is available.
 */
void board_late_init_hook(void)
{
	const struct gpio_dt_spec rf1 =
		GPIO_DT_SPEC_GET(DT_NODELABEL(rf_switch), rf1_gpios);
	const struct gpio_dt_spec rf2 =
		GPIO_DT_SPEC_GET(DT_NODELABEL(rf_switch), rf2_gpios);
	const struct gpio_dt_spec rf3 =
		GPIO_DT_SPEC_GET(DT_NODELABEL(rf_switch), rf3_gpios);


	/* configure RFSW8001 GPIOs (110: RF1/RF2 coexistence mode) */
	if (!gpio_is_ready_dt(&rf1) ||
	    !gpio_is_ready_dt(&rf2) ||
	    !gpio_is_ready_dt(&rf3)) {
		return;
	}

	(void)gpio_pin_configure_dt(&rf1, GPIO_OUTPUT_HIGH);
	(void)gpio_pin_configure_dt(&rf2, GPIO_OUTPUT_HIGH);
	(void)gpio_pin_configure_dt(&rf3, GPIO_OUTPUT_LOW);
}
