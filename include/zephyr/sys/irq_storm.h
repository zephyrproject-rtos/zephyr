/*
 * Copyright (c) 2026 Picoheart Inc.
 * Copyright (c) 2026 Zhan Gao <gaozhan.9426@picoheart.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt storm detection
 */

#ifndef ZEPHYR_INCLUDE_SYS_IRQ_STORM_H_
#define ZEPHYR_INCLUDE_SYS_IRQ_STORM_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IRQ_STORM_DETECTION

/**
 * @brief Translate a software ISR table slot back to its encoded zephyr IRQ.
 *
 * Inverse of @c z_get_sw_isr_table_idx().  Returns the IRQ number that
 * @c irq_enable() / @c irq_disable() would accept, including any
 * MULTI_LEVEL_INTERRUPTS aggregator-local bits.  Mainly used by tooling
 * (shell commands, diagnostics) that iterates over @c _sw_isr_table.
 *
 * @param idx Slot index in the software ISR table (0 .. IRQ_TABLE_SIZE-1).
 *
 * @return Encoded zephyr IRQ for the slot, or 0 if @p idx is out of range.
 */
unsigned int z_irq_storm_idx_to_irq(unsigned int idx);

/**
 * @brief Application hook invoked after a stormed line has been masked.
 *
 * The default implementation is a no-op.  Enable
 * @kconfig{CONFIG_IRQ_STORM_CUSTOM_ON_DISABLE_HOOK} and provide a
 * strong definition to plug in custom policy instead:
 *
 *   - schedule a delayed work that calls @c irq_enable() to re-arm the
 *     line after a cooldown;
 *   - escalate to a fault handler / system reset;
 *   - log to a separate sink (e.g. a flight recorder);
 *   - notify userspace.
 *
 * Without an override, a stormed line remains masked until something
 * explicitly re-enables it.
 *
 * Called once per @c irq_disable() decision, after the line is masked,
 * outside any storm spinlock.
 *
 * @param irq Encoded zephyr IRQ that was just masked.
 */
void z_irq_storm_on_disable(unsigned int irq);

#endif /* CONFIG_IRQ_STORM_DETECTION */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_IRQ_STORM_H_ */
