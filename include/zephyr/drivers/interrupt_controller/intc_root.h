/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Root interrupt controller API
 *
 * The platform's root interrupt controller is a singleton, like the system
 * timer: exactly one driver implements it. That driver selects
 * CONFIG_INTC_ROOT and provides the functions below under these names; the
 * architecture maps its interrupt control functions onto them directly, with
 * no glue and no indirection. The signatures match the architecture functions
 * they stand in for.
 *
 * An architecture only references the functions its interrupt path needs: a
 * driver provides the pending-state operations when it selects
 * CONFIG_ARCH_HAS_IRQ_PENDING_OPS, and the acknowledge pair on architectures
 * that acknowledge interrupts in software.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_ROOT_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_ROOT_H_

#ifndef _ASMLANGUAGE

#include <stdbool.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup intc_root_interface Root interrupt controller API
 * @ingroup io_interfaces
 * @since 4.5
 * @version 0.1.0
 * @{
 */

/**
 * @brief Enable an interrupt line
 *
 * @param irq IRQ number, as passed to arch_irq_enable()
 */
void intc_root_enable(unsigned int irq);

/**
 * @brief Disable an interrupt line
 *
 * @param irq IRQ number
 */
void intc_root_disable(unsigned int irq);

/**
 * @brief Get the enable state of an interrupt line
 *
 * @param irq IRQ number
 *
 * @return 1 if the line is enabled, 0 otherwise
 */
int intc_root_is_enabled(unsigned int irq);

/**
 * @brief Configure the priority of an interrupt line
 *
 * @param irq IRQ number
 * @param prio Interrupt priority
 * @param flags Architecture-specific interrupt flags, as passed to IRQ_CONNECT()
 */
void intc_root_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);

#if defined(CONFIG_ARCH_HAS_IRQ_PENDING_OPS) || defined(__DOXYGEN__)
/**
 * @brief Set an interrupt line pending
 *
 * @kconfig_dep{CONFIG_ARCH_HAS_IRQ_PENDING_OPS}
 *
 * @param irq IRQ number
 */
void intc_root_set_pending(unsigned int irq);

/**
 * @brief Clear the pending state of an interrupt line
 *
 * @kconfig_dep{CONFIG_ARCH_HAS_IRQ_PENDING_OPS}
 *
 * @param irq IRQ number
 */
void intc_root_clear_pending(unsigned int irq);

/**
 * @brief Get the pending state of an interrupt line
 *
 * @kconfig_dep{CONFIG_ARCH_HAS_IRQ_PENDING_OPS}
 *
 * @param irq IRQ number
 *
 * @return true if the line is pending, false otherwise
 */
bool intc_root_is_pending(unsigned int irq);
#endif /* CONFIG_ARCH_HAS_IRQ_PENDING_OPS */

/**
 * @brief Acknowledge the highest priority pending interrupt
 *
 * Called from the interrupt entry path on architectures that acknowledge
 * interrupts in software.
 *
 * @return IRQ number of the interrupt being acknowledged
 */
unsigned int intc_root_get_active(void);

/**
 * @brief Signal end of interrupt
 *
 * Called from the interrupt exit path on architectures that acknowledge
 * interrupts in software.
 *
 * @param irq IRQ number returned by intc_root_get_active()
 */
void intc_root_eoi(unsigned int irq);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_ROOT_H_ */
