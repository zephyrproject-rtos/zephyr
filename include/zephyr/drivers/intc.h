/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_H_

/**
 * @brief Interrupt controller interface
 * @defgroup intc_interface Interrupt controller interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** Interrupt controller API structure */
__subsystem struct intc_driver_api {
	int (*configure_irq)(const struct device *dev, uint16_t irq, uint32_t flags);
	int (*enable_irq)(const struct device *dev, uint16_t irq);
	int (*disable_irq)(const struct device *dev, uint16_t irq);
	int (*trigger_irq)(const struct device *dev, uint16_t irq);
	int (*clear_irq)(const struct device *dev, uint16_t irq);
};

/** @endcond */

/**
 * @brief Configure interrupt line
 *
 * @param dev Interrupt controller instance
 * @param irq Interrupt line of interrupt controller
 * @param flags Encoded interrupt line configuration
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
static inline int intc_configure_irq(const struct device *dev, uint16_t irq, uint32_t flags)
{
	const struct intc_driver_api *api = (const struct intc_driver_api *)dev->api;

	return api->configure_irq(dev, irq, flags);
}

/**
 * @brief Enable interrupt request
 *
 * @param dev Interrupt controller instance
 * @param irq Interrupt line of interrupt controller
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
static inline int intc_enable_irq(const struct device *dev, uint16_t irq)
{
	const struct intc_driver_api *api = (const struct intc_driver_api *)dev->api;

	return api->enable_irq(dev, irq);
}

/**
 * @brief Disable interrupt request
 *
 * @param dev Interrupt controller instance
 * @param irq Interrupt line of interrupt controller
 *
 * @retval 1 if interrupt request was previously enabled
 * @retval 0 if interrupt request was already disabled
 * @retval -errno code if failure
 */
static inline int intc_disable_irq(const struct device *dev, uint16_t irq)
{
	const struct intc_driver_api *api = (const struct intc_driver_api *)dev->api;

	return api->disable_irq(dev, irq);
}

/**
 * @brief Trigger interrupt request
 *
 * @param dev Interrupt controller instance
 * @param irq Interrupt line of interrupt controller
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
static inline int intc_trigger_irq(const struct device *dev, uint16_t irq)
{
	const struct intc_driver_api *api = (const struct intc_driver_api *)dev->api;

	return api->trigger_irq(dev, irq);
}

/**
 * @brief Clear triggered interrupt request
 *
 * @param dev Interrupt controller instance
 * @param irq Interrupt line of interrupt controller
 *
 * @retval 1 if interrupt was previously triggered
 * @retval 0 if interrupt was already cleared
 * @retval -errno code if failure
 */
static inline int intc_clear_irq(const struct device *dev, uint16_t irq)
{
	const struct intc_driver_api *api = (const struct intc_driver_api *)dev->api;

	return api->clear_irq(dev, irq);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_H_ */
