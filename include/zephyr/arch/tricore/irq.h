/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore specific IRQ macros and helpers
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_IRQ_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <zephyr/sys/util.h>

/** @cond INTERNAL_HIDDEN */

#define IRQ_USE_TOS BIT(1)
#define IRQ_TOS     GENMASK(31, 28)

#ifndef _ASMLANGUAGE
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>

extern void arch_irq_enable(unsigned int irq);
extern void arch_irq_disable(unsigned int irq);
extern int arch_irq_is_enabled(unsigned int irq);

extern void z_tricore_irq_config(unsigned int irq, unsigned int prio, unsigned int flags);

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	{                                                                                          \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);                                       \
		z_tricore_irq_config(irq_p, priority_p, flags_p);                                  \
	}

/* Vectored by priority: one direct ISR per priority, given as a literal */
#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)                                 \
	{                                                                                          \
		__asm__(".globl z_tricore_prio_isr_" STRINGIFY(priority_p) "\n\t"                  \
			".set z_tricore_prio_isr_" STRINGIFY(priority_p)                           \
			", " STRINGIFY(isr_p));                                                    \
		z_tricore_irq_config(irq_p, priority_p, flags_p);                                  \
	}

/* The common interrupt entry and exit do the nesting count and tracing */
#define ARCH_ISR_DIRECT_HEADER()     do { } while (false)
#define ARCH_ISR_DIRECT_FOOTER(swap) ARG_UNUSED(swap)
#define ARCH_ISR_DIRECT_PM()         do { } while (false)

/* A non-zero return requests a reschedule check */
#define ARCH_ISR_DIRECT_DECLARE(name)                                                              \
	static inline int name##_body(void);                                                       \
	static int __used name##_handler(void)                                                     \
	{                                                                                          \
		return name##_body();                                                              \
	}                                                                                          \
	__asm__(".section .text." #name ", \"ax\"\n\t"                                             \
		".globl " #name "\n\t"                                                             \
		".type " #name ", @function\n\t"                                                   \
		".p2align 1\n"                                                                     \
		#name ":\n\t"                                                                      \
		"movh.a %a12, hi:" #name "_handler\n\t"                                            \
		"lea %a12, [%a12], lo:" #name "_handler\n\t"                                       \
		"j z_tricore_isr_direct\n\t"                                                       \
		".size " #name ", . - " #name "\n\t"                                               \
		".previous");                                                                      \
	void name(void);                                                                           \
	static inline int name##_body(void)
#endif /* _ASMLANGUAGE */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_IRQ_H_ */
