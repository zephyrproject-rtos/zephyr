/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/reset.h>

#include "gpio_dw_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gpio_config_irq_t)(const struct device *port);

struct gpio_dw_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	DEVICE_MMIO_NAMED_ROM(gpio_mmio);
	uint32_t ngpios;
	uint32_t irq_num; /* set to 0 if GPIO port cannot interrupt */
	gpio_config_irq_t config_func;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
	const struct reset_dt_spec reset;
#endif
};

struct gpio_dw_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	DEVICE_MMIO_NAMED_RAM(gpio_mmio);
	sys_slist_t callbacks;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_DW_H_ */
