/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RH850 specific IRQ macros and helpers
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_IRQ_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

extern void arch_irq_enable(unsigned int irq);
extern void arch_irq_disable(unsigned int irq);
extern int arch_irq_is_enabled(unsigned int irq);

extern void z_rh850_irq_config(unsigned int irq, unsigned int prio, uint32_t flags);

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	{                                                                                          \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);                                       \
		z_rh850_irq_config(irq_p, priority_p, flags_p);                                    \
	}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_IRQ_H_ */
