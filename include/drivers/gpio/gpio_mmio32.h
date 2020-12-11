/*
 * Copyright (c) 2016 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_MMIO32_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_MMIO32_H_

#include <device.h>
#include <drivers/gpio.h>
#include <zephyr/types.h>

extern const struct gpio_driver_api gpio_mmio32_api;

struct gpio_mmio32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	volatile uint32_t *reg;
	uint32_t mask;
};

struct gpio_mmio32_context {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	const struct gpio_mmio32_config *config;
};

int gpio_mmio32_init(const struct device *dev);

#ifdef CONFIG_GPIO_MMIO32

/**
 * Create a device object for accessing a simple 32-bit i/o register using the
 * same APIs as GPIO drivers.
 *
 * @param _dev_name Device name.
 * @param _drv_name The name this instance of the driver exposes to the system.
 * @param _address  The address of the 32-bit i/o register the device will
 *		    provide access to.
 * @param _mask     Mask of bits in the register that it is valid to access.
 *		    E.g. 0xffffffffu to allow access to all of them.
 *
 * @see DEVICE_INIT
 */
#define GPIO_MMIO32_INIT(_dev_name, _drv_name, _address, _mask)		\
									\
static struct gpio_mmio32_context _dev_name##_dev_data;			\
									\
static const struct gpio_mmio32_config _dev_name##_dev_cfg = {		\
	.common = {							\
		 .port_pin_mask = _mask,				\
	},								\
	.reg	= (volatile uint32_t *)_address,				\
	.mask	= _mask,						\
};									\
									\
DEVICE_AND_API_INIT(_dev_name, _drv_name,				\
		    &gpio_mmio32_init,					\
		    &_dev_name##_dev_data,				\
		    &_dev_name##_dev_cfg,				\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &gpio_mmio32_api)

#else /* CONFIG_GPIO_MMIO32 */

/* Null definition for when support not configured into kernel */
#define GPIO_MMIO32_INIT(_dev_name, _drv_name, _address, _mask)

#endif


#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_MMIO32_H_ */
