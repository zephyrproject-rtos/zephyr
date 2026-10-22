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
#endif /* _ASMLANGUAGE */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_IRQ_H_ */
