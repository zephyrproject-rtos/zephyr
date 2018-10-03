/*
 * Copyright (c) 2017 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public interface for configuring interrupts
 */
#ifndef ZEPHYR_INCLUDE_IRQ_NEXTLEVEL_H_
#define ZEPHYR_INCLUDE_IRQ_NEXTLEVEL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
typedef void (*irq_next_level_func_t)(struct device *dev, unsigned int irq);
typedef unsigned int (*irq_next_level_get_state_t)(struct device *dev);

struct irq_next_level_api {
	irq_next_level_func_t intr_enable;
	irq_next_level_func_t intr_disable;
	irq_next_level_get_state_t intr_get_state;
};
/**
 * @endcond
 */

/**
 * @brief Enable an IRQ in the next level.
 *
 * This routine enables interrupts present in the interrupt controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param irq IRQ to be enabled.
 *
 * @return N/A
 */
static inline void irq_enable_next_level(struct device *dev, u32_t irq)
{
	const struct irq_next_level_api *api = dev->driver_api;

	api->intr_enable(dev, irq);
}

/**
 * @brief Disable an IRQ in the next level.
 *
 * This routine disables interrupts present in the interrupt controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param irq IRQ to be disabled.
 *
 * @return N/A
 */
static inline void irq_disable_next_level(struct device *dev, u32_t irq)
{
	const struct irq_next_level_api *api = dev->driver_api;

	api->intr_disable(dev, irq);
}

/**
 * @brief Get IRQ enable state.
 *
 * This routine indicates if any interrupts are enabled in the interrupt
 * controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return interrupt enable state, true or false
 */
static inline unsigned int irq_is_enabled_next_level(struct device *dev)
{
	const struct irq_next_level_api *api = dev->driver_api;

	return api->intr_get_state(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IRQ_NEXTLEVEL_H_ */
