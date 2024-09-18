/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IRQ_MGMT_H_
#define ZEPHYR_INCLUDE_IRQ_MGMT_H_

#include <zephyr/device.h>
#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
#include <zephyr/irq_multilevel.h>
#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */

#ifdef __cplusplus
extern "C" {
#endif

/* driver API definition */
typedef int (*irq_mgr_alloc_t)(const struct device *dev, uint32_t *irq_base, uint32_t nr_irqs);
typedef int (*irq_mgr_free_t)(const struct device *dev, uint32_t irq_base, uint32_t nr_irqs);

__subsystem struct irq_mgr_driver_api {
	irq_mgr_alloc_t alloc;
	irq_mgr_free_t free;
};

/**
 * @brief Allocate a range of IRQs.
 *
 * @param[in] dev Pointer to device structure for IRQ_MGMT driver instance
 * @param[out] irq_base Start of allocated IRQ if successful
 * @param[in] nr_irqs Number of IRQs to allocate
 *
 * @retval 0 Allocated @a nr_irqs IRQs, starting from @a irq_base
 * @retval -EINVAL Invalid argument (e.g. allocating more IRQs than the manager has, trying to
 *                 allocate 0 IRQs, etc.)
 * @retval -ENOSPC No contiguous IRQs big enough to accommodate the allocation
 */
static inline int irq_mgr_alloc(const struct device *dev, uint32_t *irq_base, uint32_t nr_irqs)
{
	const struct irq_mgr_driver_api *api = (const struct irq_mgr_driver_api *)dev->api;

	return api->alloc(dev, irq_base, nr_irqs);
}

/**
 * @brief Free a range of IRQs.
 *
 * @note The IRQs must be disabled before calling this routine.
 *
 * @param[in] dev Pointer to device structure for IRQ_MGMT driver instance
 * @param[in] irq_base Start of the previously allocated IRQ
 * @param[in] nr_irqs Number of IRQs to free
 *
 * @retval 0 Freed @a nr_irqs IRQs, starting from @a irq_base
 * @retval -EINVAL Invalid argument (e.g. try to free more IRQs than the manager has, trying
 *                 to free 0 IRQ, etc.)
 * @retval -EFAULT The IRQs in the indicated range are not all allocated.
 */
static inline int irq_mgr_free(const struct device *dev, uint32_t irq_base, uint32_t nr_irqs)
{
	const struct irq_mgr_driver_api *api = (const struct irq_mgr_driver_api *)dev->api;

	return api->free(dev, irq_base, nr_irqs);
}

static inline uint32_t irq_mgr_irq_inc(uint32_t irq, uint32_t inc)
{
#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	unsigned int level = irq_get_level(irq);
	uint32_t local_irq = irq_from_level(irq, level);
	uint32_t parent_irq = irq_get_intc_irq(irq);

	return irq_to_level(local_irq + inc, level) | parent_irq;
#else
	return irq + inc;
#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IRQ_MGMT_H_ */
