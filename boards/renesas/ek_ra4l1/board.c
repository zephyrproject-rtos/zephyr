/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(board_control, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i3c0)) && defined(CONFIG_I3C)
static int i3c_init(void)
{
	const struct gpio_dt_spec i3c_pullup_gpios[2] = {
		GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), i3c_pullup_gpios, 0),
		GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), i3c_pullup_gpios, 1),
	};

	if (!gpio_is_ready_dt(&i3c_pullup_gpios[0]) || !gpio_is_ready_dt(&i3c_pullup_gpios[1])) {
		LOG_ERR("I3C pull-up control is not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&i3c_pullup_gpios[0], GPIO_INPUT) ||
	    gpio_pin_configure_dt(&i3c_pullup_gpios[1], GPIO_INPUT)) {
		LOG_ERR("Failed to configure pull-up control");
		return -EIO;
	}

	return 0;
}

SYS_INIT(i3c_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
