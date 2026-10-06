/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TI K3 SCI-managed interrupt router public API
 *
 * With ti,sci-intr-out children and MULTI_LEVEL_INTERRUPTS, peripherals use
 * ordinary IRQ_CONNECT / irq_enable; the SoC layer calls ti_sci_intr_irq_* to
 * program TISCI and demux through VIM.
 *
 * ti_sci_intr_connect() remains available for tests and dynamic allocation.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate an interrupt-router output, program the TISCI route, and
 *        connect @p isr (directly to VIM, or via the L2 table when MLI is on).
 *
 * @param dev      Interrupt-router device
 * @param input    Router input hardware number from the device tree
 * @param isr      Interrupt service routine
 * @param arg      ISR argument
 * @param priority Parent IRQ priority
 * @param flags    Parent IRQ flags (e.g. IRQ_TYPE_EDGE / IRQ_TYPE_LEVEL).
 *                 If neither edge nor level is set, the driver's
 *                 ti,intr-trigger-type DT property is applied when present.
 *
 * @return 0 on success, or a negative errno on failure
 */
int ti_sci_intr_connect(const struct device *dev, uint16_t input,
			void (*isr)(const void *arg), const void *arg, uint32_t priority,
			uint32_t flags);

/**
 * @brief Disconnect a previously connected router input and release the route.
 *
 * @param dev   Interrupt-router device
 * @param input Router input hardware number
 *
 * @return 0 on success, or a negative errno on failure
 */
int ti_sci_intr_disconnect(const struct device *dev, uint16_t input);

/**
 * @brief Return the parent IRQ currently mapped for @p input.
 *
 * @param dev      Interrupt-router device
 * @param input    Router input hardware number
 * @param parent_irq Output storage for the parent IRQ number
 *
 * @return 0 on success, -ENOENT if no route is active, or another negative errno
 */
int ti_sci_intr_get_parent_irq(const struct device *dev, uint16_t input, unsigned int *parent_irq);

#if (defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)) ||     \
	defined(__DOXYGEN__)

/**
 * @brief Print currently programmed IR input / output / VIM routes.
 *
 * Useful after the console is up; early irq_enable() printks may be lost.
 */
void ti_sci_intr_log_routes(void);

/**
 * @brief Enable a multi-level IRQ whose parent is a ti,sci-intr-out.
 *
 * Programs TISCI to route the L2 IR input onto the output wired to the L1 VIM
 * IRQ encoded in @p irq, then enables that VIM line.
 *
 * @param irq Zephyr multi-level IRQ (IRQ_TO_L2(input) | vim_irq)
 */
void ti_sci_intr_irq_enable(unsigned int irq);

/**
 * @brief Disable a multi-level IRQ previously enabled with ti_sci_intr_irq_enable().
 *
 * @param irq Zephyr multi-level IRQ
 */
void ti_sci_intr_irq_disable(unsigned int irq);

/**
 * @brief Query whether a multi-level ti,sci-intr IRQ is enabled.
 *
 * @param irq Zephyr multi-level IRQ
 *
 * @return non-zero if enabled
 */
int ti_sci_intr_irq_is_enabled(unsigned int irq);

/**
 * @brief Set priority/flags on the parent VIM line of a multi-level IRQ.
 *
 * @param irq   Zephyr multi-level IRQ
 * @param prio  Priority
 * @param flags VIM trigger flags
 */
void ti_sci_intr_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS && ti_sci_intr_out */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_ */
