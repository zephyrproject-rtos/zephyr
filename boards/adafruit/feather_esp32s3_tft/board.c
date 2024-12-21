/*
 * Copyright (c) 2024 Leon Rinkel <leon@rinkel.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Automatically turns on backlight if display is configured, i.e. display DT
 * node has status okay.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

#define DISPLAY_NODE DT_CHOSEN(zephyr_display)
#if DT_NODE_HAS_STATUS(DISPLAY_NODE, okay)

static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET(DT_ALIAS(backlight), gpios);

static int board_backlight_init(void)
{
	if (!gpio_is_ready_dt(&backlight)) {
		return 0;
	}

	return gpio_pin_configure_dt(&backlight, GPIO_OUTPUT_ACTIVE);
}

SYS_INIT(board_backlight_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif
