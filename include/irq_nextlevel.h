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
typedef void (*irq_next_level_priority_t)(struct device *dev,
		unsigned int irq, unsigned int prio, uint32_t flags);
typedef int (*irq_next_level_get_line_state_t)(struct device *dev,
					       unsigned int irq);

struct irq_next_level_api {
	irq_next_level_func_t intr_enable;
	irq_next_level_func_t intr_disable;
	irq_next_level_get_state_t intr_get_state;
	irq_next_level_priority_t intr_set_priority;
	irq_next_level_get_line_state_t intr_get_line_state;
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
static inline void irq_enable_next_level(struct device *dev, uint32_t irq)
{
	const struct irq_next_level_api *api =
		(const struct irq_next_level_api *)dev->api;

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
static inline void irq_disable_next_level(struct device *dev, uint32_t irq)
{
	const struct irq_next_level_api *api =
		(const struct irq_next_level_api *)dev->api;

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
	const struct irq_next_level_api *api =
		(const struct irq_next_level_api *)dev->api;

	return api->intr_get_state(dev);
}

/**
 * @brief Set IRQ priority.
 *
 * This routine indicates if any interrupts are enabled in the interrupt
 * controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param irq IRQ to be disabled.
 * @param prio priority for irq in the interrupt controller.
 * @param flags controller specific flags.
 *
 * @return N/A
 */
static inline void irq_set_priority_next_level(struct device *dev, uint32_t irq,
		uint32_t prio, uint32_t flags)
{
	const struct irq_next_level_api *api =
		(const struct irq_next_level_api *)dev->api;

	if (api->intr_set_priority)
		api->intr_set_priority(dev, irq, prio, flags);
}

/**
 * @brief Get IRQ line enable state.
 *
 * Query if a particular IRQ line is enabled.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param irq IRQ line to be queried.
 *
 * @return interrupt enable state, true or false
 */
static inline unsigned int irq_line_is_enabled_next_level(struct device *dev,
							  unsigned int irq)
{
	const struct irq_next_level_api *api =
		(const struct irq_next_level_api *)dev->api;

	return api->intr_get_line_state(dev, irq);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IRQ_NEXTLEVEL_H_ */
