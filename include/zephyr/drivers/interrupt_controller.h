/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTORLLER_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTORLLER_H_

/**
 * @brief Interrupt Controller Interface
 * @defgroup interrupt_controller Interrupt Controller Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Enable IRQ */
typedef int (*intc_irq_enable_t)(const struct device *dev, uint32_t irq);

/* Disable IRQ */
typedef int (*intc_irq_disable_t)(const struct device *dev, uint32_t irq);

/* Configure IRQ */
typedef int (*intc_irq_configure_t)(const struct device *dev, uint32_t irq, unsigned int priority,
				    uint32_t flags);

/* Check if an IRQ is enabled */
typedef bool (*intc_irq_is_enabled_t)(const struct device *dev, uint32_t irq);

/* Check if an IRQ is pending */
typedef bool (*intc_irq_is_pending_t)(const struct device *dev, uint32_t irq);

/* Set the interrupt service routine for an IRQ, similar to dynamic_connect */
typedef int (*intc_irq_set_isr_t)(const struct device *dev, uint32_t irq, uint32_t priority,
				  void *routine, void *data, uint32_t flags);

/* Remove the interrupt service routine for an IRQ */
typedef int (*intc_irq_unset_isr_t)(const struct device *dev, uint32_t irq);

/* Enter the interrupt service routine of an IRQ */
typedef void (*intc_irq_enter_isr_t)(const struct device *dev, uint32_t irq);

/* Signal end-of-interrupt */
typedef void (*intc_irq_eoi_t)(const struct device *dev, uint32_t irq);

/* Set IRQ affinity for SMP */
typedef int (*intc_irq_set_affinity_t)(const struct device *dev, uint32_t irq, uint32_t cpumask);

/* Get the IRQ that's currently active */
typedef uint64_t (*intc_get_active_irq_t)(const struct device *dev);

/* Get the controller of the IRQ that's currently active */
typedef const struct device *(*intc_get_active_dev_t)(const struct device *dev);

__subsystem struct interrupt_controller_driver_api {
	intc_irq_enable_t intc_irq_enable;
	intc_irq_disable_t intc_irq_disable;
	intc_irq_configure_t intc_irq_configure;
	intc_irq_is_enabled_t intc_irq_is_enabled;
	intc_irq_is_pending_t intc_irq_is_pending;
	intc_irq_set_isr_t intc_irq_set_isr;
	intc_irq_unset_isr_t intc_irq_unset_isr;
	intc_irq_enter_isr_t intc_irq_enter_isr;
	intc_irq_eoi_t intc_irq_eoi;
#if CONFIG_SMP
	intc_irq_set_affinity_t intc_irq_set_affinity;
#endif /* CONFIG_SMP */
	intc_get_active_irq_t intc_get_active_irq;
	intc_get_active_dev_t intc_get_active_dev;
};

/**
 * @brief Enable an IRQ
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 *
 * @retval 0 if success, -errno otherwise
 */
__syscall int intc_irq_enable(const struct device *dev, uint32_t irq);

static inline int z_impl_intc_irq_enable(const struct device *dev, uint32_t irq)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_enable == NULL) {
		return -ENOSYS;
	}

	return api->intc_irq_enable(dev, irq);
}

/**
 * @brief Disable an IRQ
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 *
 * @retval 0 if success, -errno otherwise
 */
__syscall int intc_irq_disable(const struct device *dev, uint32_t irq);

static inline int z_impl_intc_irq_disable(const struct device *dev, uint32_t irq)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_disable == NULL) {
		return -ENOSYS;
	}

	return api->intc_irq_disable(dev, irq);
}

/**
 * @brief Configure an IRQ with priority and flags
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 * @param priority Priority for the IRQ
 * @param flags Architecture-dependent configuration flags
 *
 * @retval 0 if success, -errno otherwise
 */
__syscall int intc_irq_configure(const struct device *dev, uint32_t irq, unsigned int priority,
				 uint32_t flags);

static inline int z_impl_intc_irq_configure(const struct device *dev, uint32_t irq,
					    unsigned int priority, uint32_t flags)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_configure == NULL) {
		return -ENOSYS;
	}

	return api->intc_irq_configure(dev, irq, priority, flags);
}

/**
 * @brief Check if an IRQ is enabled
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 *
 * @retval true if the IRQ is enabled, false otherwise
 */
__syscall bool intc_irq_is_enabled(const struct device *dev, uint32_t irq);

static inline bool z_impl_intc_is_enabled(const struct device *dev, uint32_t irq)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_is_enabled == NULL) {
		return -ENOSYS;
	}

	return api->intc_irq_is_enabled(dev, irq);
}

/**
 * @brief Check if an IRQ is pending
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 *
 * @retval true if the IRQ is pending, false otherwise
 */
__syscall bool intc_irq_is_pending(const struct device *dev, uint32_t irq);

static inline bool z_impl_intc_is_pending(const struct device *dev, uint32_t irq)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_is_pending == NULL) {
		return -ENOSYS;
	}

	return api->intc_irq_is_pending(dev, irq);
}

/**
 * @brief Set isr for the IRQ
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 * @param priority Interrupt priority
 * @param routine Service routine for the IRQ
 * @param data Data to be passed into the interrupt service routine
 * @param flags Architecture-dependent configuration flags
 *
 * @retval 0 if success, -errno otherwise
 */
__syscall int intc_irq_set_isr(const struct device *dev, uint32_t irq, uint32_t priority,
			       void *routine, void *data, uint32_t flags);

static inline int z_impl_intc_irq_set_isr(const struct device *dev, uint32_t irq, uint32_t priority,
					  void *routine, void *data, uint32_t flags)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_set_isr == NULL) {
		return -ENOSYS;
	}

	return api->intc_irq_set_isr(dev, irq, priority, routine, data, flags);
}

/**
 * @brief Unset isr for the IRQ
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 *
 * @retval 0 if success, -errno otherwise
 */
__syscall int intc_irq_unset_isr(const struct device *dev, uint32_t irq);

static inline int z_impl_intc_irq_unset_isr(const struct device *dev, uint32_t irq)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_unset_isr == NULL) {
		return -ENOSYS;
	}

	return api->intc_irq_unset_isr(dev, irq);
}

/**
 * @brief Invoke an IRQ's interrupt service routine
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 */
__syscall void intc_irq_enter_isr(const struct device *dev, uint32_t irq);

static inline void z_impl_intc_irq_enter_isr(const struct device *dev, uint32_t irq)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_enter_isr != NULL) {
		api->intc_irq_enter_isr(dev, irq);
	}
}

/**
 * @brief Signal end-of-interrupt
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 */
__syscall void intc_irq_eoi(const struct device *dev, uint32_t irq);

static inline void z_impl_intc_irq_eoi(const struct device *dev, uint32_t irq)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_eoi != NULL) {
		api->intc_irq_eoi(dev, irq);
	}
}

/**
 * @brief Set CPU affinity mask for an IRQ
 *
 * Avaialble when CONFIG_SMP is enabled
 *
 * @param dev Interrupt controller device for the IRQ
 * @param irq Zephyr-encoded IRQ number
 * @param cpu_mask Bitmask to specific which cores can handle IRQ
 */
__syscall void intc_irq_set_affinity(const struct device *dev, uint32_t irq, uint32_t cpu_mask);
#if CONFIG_SMP
static inline void z_impl_irq_set_affinity(const struct device *dev, uint32_t irq,
					   uint32_t cpu_mask)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_irq_set_affinity != NULL) {
		api->intc_irq_set_affinity(dev, irq);
	}
}
#endif /* CONFIG_SMP */

/**
 * @brief Get active IRQ of an interrupt controller
 *
 * @param dev Interrupt controller device
 *
 * @retval Active IRQ number
 */
__syscall uint64_t intc_get_active_irq(const struct device *dev);

static inline uint64_t z_impl_intc_get_active_irq(const struct device *dev)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_get_active_irq == NULL) {
		return 0;
	}

	return api->intc_get_active_irq(dev);
}

/**
 * @brief Get the interrupt controller that has an active interrupt
 *
 * @retval Interrupt controller that has an active interrupt
 */
__syscall const struct device *intc_get_active_dev(const struct device *dev);

static const struct device *z_impl_intc_get_active_dev(const struct device *dev)
{
	const struct interrupt_controller_driver_api *api =
		(const struct interrupt_controller_driver_api *)dev->api;

	if (api->intc_get_active_dev == NULL) {
		return NULL;
	}

	return api->intc_get_active_dev(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/interrupt_controller.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTORLLER_H_ */
