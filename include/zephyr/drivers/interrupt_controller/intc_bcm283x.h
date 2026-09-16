/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief BCM283x interrupt-controller driver API.
 *
 * Two physical controllers back a single flat Zephyr IRQ space on
 * BCM2710 (Pi 2 / Pi 3 / Pi Zero 2 W):
 *
 *   - IRQs 0..9    -- BCM2836 ARM-local intc (per-core timers,
 *                     mailboxes, the GPU cascade, and the PMU).
 *                     Driver: drivers/interrupt_controller/intc_bcm2836_l1.c.
 *   - IRQs 32..127 -- BCM2835 ARMC peripheral aggregator (basic
 *                     bank, GPU bank 1, GPU bank 2), cascaded under
 *                     L1 IRQ 8.
 *                     Driver: drivers/interrupt_controller/intc_bcm2835_armctrl.c.
 *
 * The L1 driver is the root interrupt controller: it handles its own
 * IRQ range directly and the peripheral range through the ARMC driver
 * functions below. The drivers own their own MMIO via @c device_map().
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_BCM283X_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_BCM283X_H_

#include <stdint.h>

/**
 * @defgroup intc_bcm283x BCM283x interrupt controllers
 * @ingroup io_interfaces
 * @{
 */

/** First Zephyr IRQ number routed to the BCM2835 ARMC peripheral aggregator. */
#define BCM283X_ARMC_IRQ_BASE   32U

/** One past the last ARMC IRQ in the Zephyr IRQ space. */
#define BCM283X_ARMC_IRQ_LIMIT  (BCM283X_ARMC_IRQ_BASE + 96U)

/**
 * @brief True when @p irq falls in the BCM2836 ARM-local intc range (0..9).
 */
#define BCM283X_IRQ_IS_L1(irq)    ((irq) < BCM283X_ARMC_IRQ_BASE)

/**
 * @brief True when @p irq falls in the BCM2835 ARMC peripheral range
 *        (32..127).
 */
#define BCM283X_IRQ_IS_ARMC(irq)  ((irq) >= BCM283X_ARMC_IRQ_BASE && \
				   (irq) < BCM283X_ARMC_IRQ_LIMIT)

/**
 * @brief L1 IRQ_SOURCE bit position for the GPU cascade.
 *
 * Also used as the Zephyr IRQ number for the cascade entry. ISRs are
 * never registered at this index; when the L1 intc reports it,
 * the L1 driver transparently walks down into the ARMC controller to
 * find the actual peripheral IRQ.
 */
#define BCM2836_L1_IRQ_GPU_BIT   8U

/**
 * @brief L1 IRQ_SOURCE bit position for the per-core PMU IRQ.
 *
 * Also the Zephyr IRQ number for the PMU.
 */
#define BCM2836_L1_IRQ_PMU_BIT   9U

/* ARMC peripheral aggregator, driven by the L1 root interrupt controller */
void intc_bcm2835_armctrl_irq_enable(unsigned int irq);
void intc_bcm2835_armctrl_irq_disable(unsigned int irq);
int intc_bcm2835_armctrl_irq_is_enabled(unsigned int irq);
unsigned int intc_bcm2835_armctrl_irq_get_active(void);

/** @} */

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_BCM283X_H_ */
