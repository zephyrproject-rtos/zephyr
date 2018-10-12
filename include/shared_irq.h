/* shared_irq - Shared interrupt driver */

/*
 * Copyright (c) 2015 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHARED_IRQ_H_
#define ZEPHYR_INCLUDE_SHARED_IRQ_H_

#include <autoconf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*isr_t)(struct device *dev);

/* driver API definition */
typedef int (*shared_irq_register_t)(struct device *dev,
				isr_t isr_func,
				struct device *isr_dev);
typedef int (*shared_irq_enable_t)(struct device *dev, struct device *isr_dev);
typedef int (*shared_irq_disable_t)(struct device *dev, struct device *isr_dev);

struct shared_irq_driver_api {
	shared_irq_register_t isr_register;
	shared_irq_enable_t enable;
	shared_irq_disable_t disable;
};

extern int shared_irq_initialize(struct device *port);

typedef void (*shared_irq_config_irq_t)(void);

struct shared_irq_config {
	u32_t irq_num;
	shared_irq_config_irq_t config;
	u32_t client_count;
};

struct shared_irq_client {
	struct device *isr_dev;
	isr_t isr_func;
	u32_t enabled;
};

struct shared_irq_runtime {
	struct shared_irq_client client[CONFIG_SHARED_IRQ_NUM_CLIENTS];
};

/**
 *  @brief Register a device ISR
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_func Pointer to the ISR function for the device.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_isr_register(struct device *dev, isr_t isr_func,
				 struct device *isr_dev)
{
	const struct shared_irq_driver_api *api = dev->driver_api;

	return api->isr_register(dev, isr_func, isr_dev);
}

/**
 *  @brief Enable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_enable(struct device *dev, struct device *isr_dev)
{
	const struct shared_irq_driver_api *api = dev->driver_api;

	return api->enable(dev, isr_dev);
}

/**
 *  @brief Disable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_disable(struct device *dev, struct device *isr_dev)
{
	const struct shared_irq_driver_api *api = dev->driver_api;

	return api->disable(dev, isr_dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHARED_IRQ_H_ */
