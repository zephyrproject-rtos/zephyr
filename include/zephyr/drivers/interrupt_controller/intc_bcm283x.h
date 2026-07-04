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
 * The SoC layer (soc/brcm/bcm2710/soc_irq.c) installs the
 * @c z_soc_irq_* arch hooks under
 * @kconfig{CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER} and dispatches to
 * the two driver function families below based on which IRQ range
 * the caller is asking about. The drivers own their own MMIO via
 * @c device_map().
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_BCM283X_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_BCM283X_H_

#include <stdint.h>

/**
 * @defgroup intc_bcm283x BCM283x interrupt controllers
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
 * the SoC dispatcher transparently walks down into the ARMC
 * controller to find the actual peripheral IRQ.
 */
#define BCM2836_L1_IRQ_GPU_BIT   8U

/**
 * @brief L1 IRQ_SOURCE bit position for the per-core PMU IRQ.
 *
 * Also the Zephyr IRQ number for the PMU.
 */
#define BCM2836_L1_IRQ_PMU_BIT   9U

/**
 * @brief L1 IRQ_SOURCE bit position for the per-core mailbox 0.
 *
 * Also the Zephyr IRQ number for mailbox 0. SMP builds use mailbox 0
 * as the IPI vehicle; bits within the mailbox word distinguish IPI
 * types.
 */
#define BCM2836_L1_IRQ_MBOX0_BIT 4U

/**
 * @brief Initialise the BCM2836 ARM-local interrupt controller.
 *
 * Maps the MMIO bank via @c device_map() and masks all locally-routed
 * sources for core 0. Invoked by the SoC's @c z_soc_irq_init() during
 * arch boot, before any @c SYS_INIT priority.
 */
void bcm2836_l1_intc_init(void);

/**
 * @brief Enable an L1 IRQ source.
 *
 * @param irq Zephyr IRQ number in the L1 range (0..9). IRQ 8 (GPU
 *            cascade) is implicit and has no per-IRQ enable bit.
 */
void bcm2836_l1_intc_irq_enable(unsigned int irq);

/**
 * @brief Disable an L1 IRQ source.
 *
 * @param irq Zephyr IRQ number in the L1 range (0..9).
 */
void bcm2836_l1_intc_irq_disable(unsigned int irq);

/**
 * @brief Query whether an L1 IRQ source is currently enabled.
 *
 * @param irq Zephyr IRQ number in the L1 range.
 *
 * @retval 1 if enabled.
 * @retval 0 if disabled, or for PMU (no readback path).
 */
int  bcm2836_l1_intc_irq_is_enabled(unsigned int irq);

/**
 * @brief Decode the firing L1 IRQ.
 *
 * @return The Zephyr IRQ number of the firing source, or
 *         @kconfig{CONFIG_NUM_IRQS} if no L1 source is asserted.
 *         When the GPU cascade bit fires the caller is expected to
 *         walk into bcm2835_armctrl_ic_irq_get_active() next.
 */
unsigned int bcm2836_l1_intc_irq_get_active(void);

/**
 * @brief Raise mailbox-0 bits on a core (SMP IPI send).
 *
 * @param core Physical core id (MPIDR Aff0) of the target.
 * @param bits Mailbox bits to set; each bit is one IPI type.
 */
void bcm2836_l1_intc_mbox0_raise(unsigned int core, uint32_t bits);

/**
 * @brief Read and acknowledge this core's pending mailbox-0 bits.
 *
 * Acks only the bits observed, so a raise landing concurrently is
 * preserved and refires after the SoC's eoi hook re-enables the
 * source.
 *
 * @return The mailbox-0 bits that were pending and are now acked.
 */
uint32_t bcm2836_l1_intc_mbox0_read_ack(void);

/**
 * @brief Initialise the BCM2835 ARMC peripheral interrupt controller.
 *
 * Maps the MMIO bank via @c device_map() and disables every bank-0,
 * PEND1 and PEND2 source. Invoked by the SoC's @c z_soc_irq_init()
 * during arch boot, before any @c SYS_INIT priority.
 */
void bcm2835_armctrl_ic_init(void);

/**
 * @brief Enable an ARMC peripheral IRQ source.
 *
 * @param irq Zephyr IRQ number in the ARMC range
 *            (@ref BCM283X_ARMC_IRQ_BASE .. @ref BCM283X_ARMC_IRQ_LIMIT - 1).
 *            Calls outside the range are silently ignored.
 */
void bcm2835_armctrl_ic_irq_enable(unsigned int irq);

/**
 * @brief Disable an ARMC peripheral IRQ source.
 *
 * @param irq Zephyr IRQ number in the ARMC range. Out-of-range
 *            callers are silently ignored.
 */
void bcm2835_armctrl_ic_irq_disable(unsigned int irq);

/**
 * @brief Query whether an ARMC peripheral IRQ source is enabled.
 *
 * @param irq Zephyr IRQ number in the ARMC range.
 *
 * @retval 1 if enabled.
 * @retval 0 if disabled, or @p irq is outside the ARMC range.
 */
int  bcm2835_armctrl_ic_irq_is_enabled(unsigned int irq);

/**
 * @brief Decode the firing ARMC peripheral IRQ.
 *
 * Encapsulates the bank-0 shortcut-bit decode (Quirk 1 in the
 * BCM2835 ARM Peripherals datasheet ch. 7) and falls through to
 * the PEND1 / PEND2 walk only when no shortcut bit is set.
 *
 * @return The Zephyr IRQ number of the firing peripheral, or
 *         @kconfig{CONFIG_NUM_IRQS} if no ARMC source is asserted.
 */
unsigned int bcm2835_armctrl_ic_irq_get_active(void);

/** @} */

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_BCM283X_H_ */
