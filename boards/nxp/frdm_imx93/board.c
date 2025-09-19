/**
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_FRDM_IMX93_LOG_LEVEL);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_exp1)) && \
	IS_ENABLED(CONFIG_BOARD_FRDM_IMX93_GPIO_EXP1_INIT)

static int board_init_gpio_exp1(void)
{
	int ret = 0;
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio_exp1));

	/** Deferred initialization of PCAL6408 */
	ret = device_init(dev);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to initialize device %s",
			   ret, dev->name);
	}

	return ret;
}

SYS_INIT(board_init_gpio_exp1, POST_KERNEL, CONFIG_BOARD_FRDM_IMX93_GPIO_EXP1_INIT_PRIO);

#endif
/*
 * DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_exp1)) && \
 * IS_ENABLED(CONFIG_BOARD_FRDM_IMX93_GPIO_EXP1_INIT)
 */
