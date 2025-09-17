/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include "gpio_dw_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gpio_config_irq_t)(const struct device *port);

struct gpio_dw_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint32_t base_addr;
	uint32_t ngpios;
	uint32_t irq_num; /* set to 0 if GPIO port cannot interrupt */
	gpio_config_irq_t config_func;
};

struct gpio_dw_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_ */
