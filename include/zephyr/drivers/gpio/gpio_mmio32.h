/*
 * Copyright (c) 2016 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_MMIO32_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_MMIO32_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @param node_id   The devicetree node identifier.
 * @param _address  The address of the 32-bit i/o register the device will
 *		    provide access to.
 * @param _mask     Mask of bits in the register that it is valid to access.
 *		    E.g. 0xffffffffu to allow access to all of them.
 *
 */
#define GPIO_MMIO32_INIT(node_id, _address, _mask)					\
static struct gpio_mmio32_context _CONCAT(Z_DEVICE_DT_DEV_ID(node_id), _ctx);		\
											\
static const struct gpio_mmio32_config _CONCAT(Z_DEVICE_DT_DEV_ID(node_id), _cfg) = {	\
	.common = {									\
		 .port_pin_mask = _mask,						\
	},										\
	.reg	= (volatile uint32_t *)_address,					\
	.mask	= _mask,								\
};											\
											\
DEVICE_DT_DEFINE(node_id,								\
		    &gpio_mmio32_init,							\
		    NULL,								\
		    &_CONCAT(Z_DEVICE_DT_DEV_ID(node_id), _ctx),			\
		    &_CONCAT(Z_DEVICE_DT_DEV_ID(node_id), _cfg),			\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		    &gpio_mmio32_api)


#else /* CONFIG_GPIO_MMIO32 */

/* Null definition for when support not configured into kernel */
#define GPIO_MMIO32_INIT(node_id, _address, _mask)

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_MMIO32_H_ */
