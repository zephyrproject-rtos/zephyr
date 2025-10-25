/*
 * Copyright (c) 2025 LiteX Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_LITEX_CLIC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_LITEX_CLIC_H_

#include <zephyr/types.h>

/**
 * @brief LiteX CLIC register definitions
 *
 * Register layout based on LiteX CLIC implementation in litex/soc/cores/clic.py
 * Each interrupt has 4 CSR registers (32-bit each for LiteX CSRStorage):
 * - CLICINTIP: Interrupt pending
 * - CLICINTIE: Interrupt enable
 * - CLICINTATTR: Interrupt attributes (trigger type, shv, etc.)
 * - CLICIPRIO: Interrupt priority
 */

/* Base offsets for CSR registers arrays */
#define LITEX_CLIC_CLICINTIP_BASE    0x0000  /* Interrupt pending array */
#define LITEX_CLIC_CLICINTIE_BASE    0x0040  /* Interrupt enable array */
#define LITEX_CLIC_CLICINTATTR_BASE  0x0080  /* Interrupt attributes array */
#define LITEX_CLIC_CLICIPRIO_BASE    0x00C0  /* Interrupt priority array */

/* Debug registers for hardware bridge verification */
#define LITEX_CLIC_DEBUG_IP_HW       0x0100  /* Hardware pending bits */
#define LITEX_CLIC_DEBUG_IE_HW       0x0140  /* Hardware enable bits */
#define LITEX_CLIC_DEBUG_ATTR_HW     0x0180  /* Hardware attributes */
#define LITEX_CLIC_DEBUG_PRIO_HW     0x01C0  /* Hardware priority */
#define LITEX_CLIC_DEBUG_ACTIVE      0x0200  /* Active interrupts */

/* Per-interrupt register calculation macros */
#define LITEX_CLIC_INTIP(irq)        (LITEX_CLIC_CLICINTIP_BASE + ((irq) * 4))
#define LITEX_CLIC_INTIE(irq)        (LITEX_CLIC_CLICINTIE_BASE + ((irq) * 4))
#define LITEX_CLIC_INTATTR(irq)      (LITEX_CLIC_CLICINTATTR_BASE + ((irq) * 4))
#define LITEX_CLIC_INTPRIO(irq)      (LITEX_CLIC_CLICIPRIO_BASE + ((irq) * 4))

/* Debug register calculation macros */
#define LITEX_CLIC_DEBUG_IP(irq)     (LITEX_CLIC_DEBUG_IP_HW + ((irq) * 4))
#define LITEX_CLIC_DEBUG_IE(irq)     (LITEX_CLIC_DEBUG_IE_HW + ((irq) * 4))
#define LITEX_CLIC_DEBUG_ATTR(irq)   (LITEX_CLIC_DEBUG_ATTR_HW + ((irq) * 4))
#define LITEX_CLIC_DEBUG_PRIO(irq)   (LITEX_CLIC_DEBUG_PRIO_HW + ((irq) * 4))
#define LITEX_CLIC_DEBUG_ACT(irq)    (LITEX_CLIC_DEBUG_ACTIVE + ((irq) * 4))

/* CLIC interrupt attributes bits (CLICINTATTR register) */
#define LITEX_CLIC_ATTR_TRIG_POS     0  /* Trigger type bit position */
#define LITEX_CLIC_ATTR_TRIG_MASK    0x03  /* Trigger type mask */
#define LITEX_CLIC_ATTR_TRIG_LEVEL   0x00  /* Level-triggered */
#define LITEX_CLIC_ATTR_TRIG_EDGE_POS 0x01  /* Positive edge-triggered */
#define LITEX_CLIC_ATTR_TRIG_EDGE_NEG 0x03  /* Negative edge-triggered */
#define LITEX_CLIC_ATTR_SHV_POS      2  /* Selective hardware vectoring bit position */
#define LITEX_CLIC_ATTR_SHV          BIT(2)  /* Selective hardware vectoring enable */

/* LiteX CLIC specific constants */
#define LITEX_CLIC_MAX_INTERRUPTS    64   /* Maximum supported interrupts */
#define LITEX_CLIC_CSR_INTERRUPTS    16   /* First 16 interrupts have CSR control */
#define LITEX_CLIC_MAX_PRIORITY      255  /* 8-bit priority field */
#define LITEX_CLIC_DEFAULT_PRIORITY  128  /* Default interrupt priority */

/* LiteX CSRStorage register size */
#define LITEX_CLIC_CSR_SIZE          4    /* 32-bit CSRStorage registers */

/* Helper macros for interrupt management */
#define LITEX_CLIC_IS_CSR_CONTROLLED(irq) ((irq) < LITEX_CLIC_CSR_INTERRUPTS)
#define LITEX_CLIC_IS_HW_CONTROLLED(irq)  ((irq) >= LITEX_CLIC_CSR_INTERRUPTS)

/* Function declarations for internal use */
#ifdef CONFIG_LITEX_CLIC_DIRECT_API
void litex_clic_irq_enable(const struct device *dev, uint32_t irq);
void litex_clic_irq_disable(const struct device *dev, uint32_t irq);
int litex_clic_irq_is_enabled(const struct device *dev, uint32_t irq);
void litex_clic_set_priority(const struct device *dev, uint32_t irq, uint32_t priority);
uint32_t litex_clic_get_priority(const struct device *dev, uint32_t irq);
void litex_clic_set_trigger(const struct device *dev, uint32_t irq, uint32_t trigger);
void litex_clic_set_pending(const struct device *dev, uint32_t irq);
void litex_clic_clear_pending(const struct device *dev, uint32_t irq);
bool litex_clic_is_pending(const struct device *dev, uint32_t irq);
#endif /* CONFIG_LITEX_CLIC_DIRECT_API */

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_LITEX_CLIC_H_ */