/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_

#include <zephyr/types.h>
#include <drivers/gpio.h>
#include "gpio_dw_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gpio_config_irq_t)(const struct device *port);

struct gpio_dw_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint32_t ngpios;
	uint32_t irq_num; /* set to 0 if GPIO port cannot interrupt */
	gpio_config_irq_t config_func;

#ifdef CONFIG_GPIO_DW_SHARED_IRQ
	char *shared_irq_dev_name;
#endif /* CONFIG_GPIO_DW_SHARED_IRQ */

#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	void *clock_data;
#endif
};

struct gpio_dw_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	uint32_t base_addr;
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	const struct device *clock;
#endif
	sys_slist_t callbacks;
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state device_power_state;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_ */
