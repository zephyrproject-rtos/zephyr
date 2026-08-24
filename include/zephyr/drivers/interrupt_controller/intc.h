/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt controller driver API
 *
 * A platform's root interrupt controller is an interrupt controller device
 * implementing this API. The architecture interrupt control functions
 * (arch_irq_enable() and friends) are implemented generically on top of it,
 * so a platform needs no interrupt control glue of its own.
 *
 * Interrupt acknowledge and end-of-interrupt run on the interrupt entry path
 * and are deliberately not part of the device API: on architectures that
 * acknowledge interrupts in software the root controller driver provides
 * intc_root_get_active() and intc_root_eoi() as direct-call functions.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_H_

#ifndef _ASMLANGUAGE

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interrupt controller driver API
 *
 * The pending-state operations are optional. A driver that provides them
 * selects CONFIG_ARCH_HAS_IRQ_PENDING_OPS when it is the root controller.
 */
__subsystem struct intc_driver_api {
	void (*enable)(const struct device *dev, unsigned int irq);
	void (*disable)(const struct device *dev, unsigned int irq);
	int (*is_enabled)(const struct device *dev, unsigned int irq);
	void (*priority_set)(const struct device *dev, unsigned int irq, unsigned int prio,
			     uint32_t flags);
	void (*set_pending)(const struct device *dev, unsigned int irq);
	void (*clear_pending)(const struct device *dev, unsigned int irq);
	bool (*is_pending)(const struct device *dev, unsigned int irq);
};

/**
 * @brief The platform's root interrupt controller device
 *
 * Provided by the root interrupt controller driver with
 * INTC_ROOT_DEVICE_DEFINE().
 */
extern const struct device *const intc_root_device;

/**
 * @brief Define a device as the platform's root interrupt controller
 *
 * @param node_id Devicetree node of the interrupt controller
 */
#define INTC_ROOT_DEVICE_DEFINE(node_id)                                                           \
	const struct device *const intc_root_device = DEVICE_DT_GET(node_id)

/**
 * @brief Enable an interrupt line
 *
 * @param dev Interrupt controller device
 * @param irq Interrupt line
 */
static ALWAYS_INLINE void intc_irq_enable(const struct device *dev, unsigned int irq)
{
	((const struct intc_driver_api *)dev->api)->enable(dev, irq);
}

/**
 * @brief Disable an interrupt line
 *
 * @param dev Interrupt controller device
 * @param irq Interrupt line
 */
static ALWAYS_INLINE void intc_irq_disable(const struct device *dev, unsigned int irq)
{
	((const struct intc_driver_api *)dev->api)->disable(dev, irq);
}

/**
 * @brief Get the enable state of an interrupt line
 *
 * @param dev Interrupt controller device
 * @param irq Interrupt line
 *
 * @return 1 if the line is enabled, 0 otherwise
 */
static ALWAYS_INLINE int intc_irq_is_enabled(const struct device *dev, unsigned int irq)
{
	return ((const struct intc_driver_api *)dev->api)->is_enabled(dev, irq);
}

/**
 * @brief Set the priority of an interrupt line
 *
 * @param dev Interrupt controller device
 * @param irq Interrupt line
 * @param prio Interrupt priority
 * @param flags Architecture-specific interrupt flags
 */
static ALWAYS_INLINE void intc_irq_priority_set(const struct device *dev, unsigned int irq,
						unsigned int prio, uint32_t flags)
{
	((const struct intc_driver_api *)dev->api)->priority_set(dev, irq, prio, flags);
}

/**
 * @brief Set an interrupt line pending
 *
 * @param dev Interrupt controller device
 * @param irq Interrupt line
 */
static ALWAYS_INLINE void intc_irq_set_pending(const struct device *dev, unsigned int irq)
{
	((const struct intc_driver_api *)dev->api)->set_pending(dev, irq);
}

/**
 * @brief Clear the pending state of an interrupt line
 *
 * @param dev Interrupt controller device
 * @param irq Interrupt line
 */
static ALWAYS_INLINE void intc_irq_clear_pending(const struct device *dev, unsigned int irq)
{
	((const struct intc_driver_api *)dev->api)->clear_pending(dev, irq);
}

/**
 * @brief Get the pending state of an interrupt line
 *
 * @param dev Interrupt controller device
 * @param irq Interrupt line
 *
 * @return true if the line is pending, false otherwise
 */
static ALWAYS_INLINE bool intc_irq_is_pending(const struct device *dev, unsigned int irq)
{
	return ((const struct intc_driver_api *)dev->api)->is_pending(dev, irq);
}

/**
 * @brief Set the priority of an interrupt line on the root controller
 *
 * @param irq Interrupt line
 * @param prio Interrupt priority
 * @param flags Architecture-specific interrupt flags
 */
void intc_root_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);

/**
 * @brief Acknowledge the highest priority pending interrupt
 *
 * Provided by the root interrupt controller driver on architectures that
 * acknowledge interrupts in software; called from the interrupt entry path.
 *
 * @return The interrupt line being acknowledged
 */
unsigned int intc_root_get_active(void);

/**
 * @brief Signal end of interrupt on the root controller
 *
 * Provided by the root interrupt controller driver on architectures that
 * acknowledge interrupts in software; called from the interrupt exit path.
 *
 * @param irq Interrupt line returned by intc_root_get_active()
 */
void intc_root_eoi(unsigned int irq);

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_H_ */
