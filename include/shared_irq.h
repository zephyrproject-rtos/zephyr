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

typedef int (*isr_t)(const struct device *dev);

/* driver API definition */
typedef int (*shared_irq_register_t)(const struct device *dev,
				     isr_t isr_func,
				     const struct device *isr_dev);
typedef int (*shared_irq_enable_t)(const struct device *dev,
				   const struct device *isr_dev);
typedef int (*shared_irq_disable_t)(const struct device *dev,
				    const struct device *isr_dev);

struct shared_irq_driver_api {
	shared_irq_register_t isr_register;
	shared_irq_enable_t enable;
	shared_irq_disable_t disable;
};

extern int shared_irq_initialize(const struct device *port);

typedef void (*shared_irq_config_irq_t)(void);

struct shared_irq_config {
	uint32_t irq_num;
	shared_irq_config_irq_t config;
	uint32_t client_count;
};

struct shared_irq_client {
	const struct device *isr_dev;
	isr_t isr_func;
	uint32_t enabled;
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
static inline int shared_irq_isr_register(const struct device *dev,
					  isr_t isr_func,
					  const struct device *isr_dev)
{
	const struct shared_irq_driver_api *api =
		(const struct shared_irq_driver_api *)dev->api;

	return api->isr_register(dev, isr_func, isr_dev);
}

/**
 *  @brief Enable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_enable(const struct device *dev,
				    const struct device *isr_dev)
{
	const struct shared_irq_driver_api *api =
		(const struct shared_irq_driver_api *)dev->api;

	return api->enable(dev, isr_dev);
}

/**
 *  @brief Disable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_disable(const struct device *dev,
				     const struct device *isr_dev)
{
	const struct shared_irq_driver_api *api =
		(const struct shared_irq_driver_api *)dev->api;

	return api->disable(dev, isr_dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHARED_IRQ_H_ */
