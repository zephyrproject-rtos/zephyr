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
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)) ||     \
	defined(__DOXYGEN__)

/**
 * @brief Return the parent IRQ currently mapped for @p input.
 *
 * @param dev        Interrupt-router device
 * @param input      Router input hardware number
 * @param parent_irq Output storage for the parent IRQ number
 *
 * @return 0 on success, -ENOENT if no route is active, or another negative errno
 */
int ti_sci_intr_get_parent_irq(const struct device *dev, uint16_t input, unsigned int *parent_irq);

/**
 * @brief Print currently programmed IR input / output / VIM routes.
 *
 * Useful after the console is up; early irq_enable() printks may be lost.
 */
void ti_sci_intr_log_routes(void);

/**
 * @brief Enable an IRQ, programming TISCI when @p irq is multi-level.
 *
 * Level-1 IRQs are forwarded to VIM. Level-2 IRQs whose parent is a
 * ti,sci-intr-out program the IR route and then enable the parent VIM line.
 *
 * @param irq Zephyr IRQ (level-1 VIM number, or IRQ_TO_L2(input) | vim_irq)
 */
void ti_sci_intr_irq_enable(unsigned int irq);

/**
 * @brief Disable an IRQ previously enabled with ti_sci_intr_irq_enable().
 *
 * @param irq Zephyr IRQ
 */
void ti_sci_intr_irq_disable(unsigned int irq);

/**
 * @brief Query whether an IRQ is enabled.
 *
 * @param irq Zephyr IRQ
 *
 * @return non-zero if enabled
 */
int ti_sci_intr_irq_is_enabled(unsigned int irq);

/**
 * @brief Set priority/flags on the VIM line for @p irq.
 *
 * @param irq   Zephyr IRQ
 * @param prio  Priority
 * @param flags VIM trigger flags
 */
void ti_sci_intr_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS && ti_sci_intr_out */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_ */
