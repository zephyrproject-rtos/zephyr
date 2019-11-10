/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A public interrupt handling
 *
 * ARM64-specific kernel interrupt handling interface.
 * Included by arm/aarch64/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_IRQ_H_

#include <irq.h>
#include <sw_isr_table.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(arch_irq_enable)
GTEXT(arch_irq_disable)
GTEXT(arch_irq_is_enabled)
#else
extern void arch_irq_enable(unsigned int irq);
extern void arch_irq_disable(unsigned int irq);
extern int arch_irq_is_enabled(unsigned int irq);

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void z_arm64_irq_priority_set(unsigned int irq, unsigned int prio,
				     u32_t flags);

/* All arguments must be computable by the compiler at build time.
 *
 * Z_ISR_DECLARE will populate the .intList section with the interrupt's
 * parameters, which will then be used by gen_irq_tables.py to create
 * the vector table and the software ISR table. This is all done at
 * build-time.
 *
 * We additionally set the priority in the interrupt controller at
 * runtime.
 */
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	z_arm64_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})

#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
({ \
	Z_ISR_DECLARE(irq_p, ISR_FLAG_DIRECT, isr_p, NULL); \
	z_arm64_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})

/* Spurious interrupt handler. Throws an error if called */
extern void z_irq_spurious(void *unused);

#ifdef CONFIG_GEN_SW_ISR_TABLE
/* Architecture-specific common entry point for interrupts from the vector
 * table. Most likely implemented in assembly. Looks up the correct handler
 * and parameter from the _sw_isr_table and executes it.
 */
extern void _isr_wrapper(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_IRQ_H_ */
