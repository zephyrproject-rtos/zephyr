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
 * Included by arm64/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_IRQ_H_

#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(arch_irq_enable)
GTEXT(arch_irq_disable)
GTEXT(arch_irq_is_enabled)
GTEXT(arch_irq_get_active)
GTEXT(arch_irq_eoi)
#if defined(CONFIG_PLATFORM_HAS_CUSTOM_INTERRUPT_CONTROLLER)
GTEXT(platform_irq_get_active)
GTEXT(platform_irq_eoi)
#endif /* CONFIG_PLATFORM_HAS_CUSTOM_INTERRUPT_CONTROLLER */
#else

#if !defined(CONFIG_PLATFORM_HAS_CUSTOM_INTERRUPT_CONTROLLER)

extern void arch_irq_enable(unsigned int irq);
extern void arch_irq_disable(unsigned int irq);
extern int arch_irq_is_enabled(unsigned int irq);
extern void arch_irq_priority_set(unsigned int irq, unsigned int prio,
				     uint32_t flags);
unsigned int arch_irq_get_active(void);
void arch_irq_eoi(unsigned int intid);
#else

/*
 * When a custom interrupt controller is specified, map the architecture
 * interrupt control functions to the SoC layer interrupt control functions.
 */

void platform_irq_init(void);
void platform_irq_enable(unsigned int irq);
void platform_irq_disable(unsigned int irq);
int platform_irq_is_enabled(unsigned int irq);

void platform_irq_priority_set(
	unsigned int irq, unsigned int prio, unsigned int flags);

unsigned int platform_irq_get_active(void);
void platform_irq_eoi(unsigned int irq);

#define arch_irq_enable(irq)		platform_irq_enable(irq)
#define arch_irq_disable(irq)		platform_irq_disable(irq)
#define arch_irq_is_enabled(irq)	platform_irq_is_enabled(irq)

#define arch_irq_priority_set(irq, prio, flags)	\
	platform_irq_priority_set(irq, prio, flags)

#endif /* !CONFIG_PLATFORM_HAS_CUSTOM_INTERRUPT_CONTROLLER */

extern void z_arm64_interrupt_init(void);

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
{ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	arch_irq_priority_set(irq_p, priority_p, flags_p); \
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_IRQ_H_ */
