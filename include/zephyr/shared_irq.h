/* shared_irq - Shared interrupt driver */

/*
 * Copyright (c) 2015 - 2023 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHARED_IRQ_H_
#define ZEPHYR_INCLUDE_SHARED_IRQ_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*isr_t)(const struct device *dev, unsigned int irq_number);

/* driver API definition */
typedef int (*shared_irq_register_t)(const struct device *dev,
				     isr_t isr_func,
				     const struct device *isr_dev);
typedef int (*shared_irq_enable_t)(const struct device *dev,
				   const struct device *isr_dev);
typedef int (*shared_irq_disable_t)(const struct device *dev,
				    const struct device *isr_dev);

__subsystem struct shared_irq_driver_api {
	shared_irq_register_t isr_register;
	shared_irq_enable_t enable;
	shared_irq_disable_t disable;
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
