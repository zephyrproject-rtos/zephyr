/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TI K3 SCI-managed interrupt router public API
 *
 * Runtime allocation of interrupt-router outputs via TISCI, with translation
 * to the parent interrupt controller (typically VIM on Cortex-R5).
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_

#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate an interrupt-router output, program the TISCI route, and
 *        dynamically connect @p isr to the translated parent (VIM) IRQ.
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_SCI_INTR_H_ */
