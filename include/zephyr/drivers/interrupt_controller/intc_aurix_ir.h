/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the Infineon AURIX Interrupt Router (IR).
 *
 * The AURIX interrupt subsystem is built from two cooperating blocks:
 *
 * - The Interrupt Router (IR), which arbitrates pending Service Requests
 *   among the CPUs and the DMA, and drives the per-source priority comparator.
 * - The Service Request Control (SRC) registers, one per interrupt source,
 *   which carry the per-source enable, priority, routing and pending bits.
 *
 * This header exposes the small low-level helpers used by the Zephyr
 * TriCore IRQ glue to configure, enable, query and raise individual
 * interrupt sources.
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_AURIX_IR_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_AURIX_IR_H_

/** Base address of the AURIX Interrupt Router (IR) MMIO block. */
#define IR_BASE  DT_REG_ADDR_BY_IDX(DT_INST(0, infineon_aurix_ir), 0)

/** Base address of the AURIX Service Request Control (SRC) MMIO block. */
#define SRC_BASE DT_REG_ADDR_BY_IDX(DT_INST(0, infineon_aurix_ir), 1)

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/device.h>

/**
 * @brief Configure an AURIX interrupt source.
 *
 * Programs the per-source SRC register with the given priority and any
 * flags supported by the SoC (e.g. type-of-service / routing).
 *
 * @param irq      Zephyr-encoded IRQ number.
 * @param priority Interrupt priority value to write into SRPN.
 * @param flags    Implementation-defined flags (typically the TOS field).
 */
void intc_aurix_ir_irq_config(unsigned int irq, unsigned int priority, unsigned int flags);

/**
 * @brief Enable an AURIX interrupt source.
 *
 * Sets the SRE bit in the matching SRC register so the source can
 * request service.
 *
 * @param irq Zephyr-encoded IRQ number.
 */
void intc_aurix_ir_irq_enable(unsigned int irq);

/**
 * @brief Disable an AURIX interrupt source.
 *
 * Clears the SRE bit in the matching SRC register. Pending requests in
 * the source are left untouched.
 *
 * @param irq Zephyr-encoded IRQ number.
 */
void intc_aurix_ir_irq_disable(unsigned int irq);

/**
 * @brief Query whether an AURIX interrupt source is enabled.
 *
 * @param irq Zephyr-encoded IRQ number.
 *
 * @return true if the source's SRE bit is set, false otherwise.
 */
bool intc_aurix_ir_irq_is_enabled(unsigned int irq);

/**
 * @brief Query whether an AURIX interrupt source has a pending request.
 *
 * @param irq Zephyr-encoded IRQ number.
 *
 * @return true if the source's SRR bit is set, false otherwise.
 */
bool intc_aurix_ir_irq_is_pending(unsigned int irq);

/**
 * @brief Clear a pending request on an AURIX interrupt source.
 *
 * Writes the CLRR bit in the matching SRC register, which clears SRR
 * without affecting other fields.
 *
 * @param irq Zephyr-encoded IRQ number.
 */
void intc_aurix_ir_irq_clear_pending(unsigned int irq);

/**
 * @brief Raise an AURIX interrupt source by IRQ number.
 *
 * Sets the SETR bit in the matching SRC register, requesting service
 * from software. Useful for self-test and software-triggered IPIs.
 *
 * @param irq Zephyr-encoded IRQ number.
 */
void intc_aurix_ir_irq_raise(unsigned int irq);

/**
 * @brief Return the currently active IRQ number.
 *
 * Reads the CPU's identifier of the interrupt currently being serviced,
 * as latched at interrupt entry.
 *
 * @return The currently active Zephyr-encoded IRQ number.
 */
unsigned int intc_aurix_ir_get_active(void);

/**
 * @brief Raise an AURIX interrupt source by SRC register address.
 *
 * Variant of @ref intc_aurix_ir_irq_raise that targets a source by its
 * absolute SRC register address rather than by IRQ number. Intended for
 * call sites that already hold the SRC pointer (e.g. driver-internal
 * software events).
 *
 * @param src Absolute address of the source's SRC register.
 */
void intc_aurix_ir_src_raise(uintptr_t src);

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_AURIX_IR_H_ */
