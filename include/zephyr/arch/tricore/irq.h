/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_IRQ_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <zephyr/sys/util.h>

#define IRQ_ZERO_LACTENCY BIT(0)
#define IRQ_USE_TOS		  BIT(1)
#define IRQ_TOS			  GENMASK(31, 28)

#ifndef _ASMLANGUAGE
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>


extern void arch_irq_enable(unsigned int irq);
extern void arch_irq_disable(unsigned int irq);
extern int arch_irq_is_enabled(unsigned int irq);
unsigned int z_soc_irq_get_active(void);

extern void z_tricore_irq_config(unsigned int irq, unsigned int prio, unsigned int flags);

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	{                                                                                          \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);                                       \
		z_tricore_irq_config(irq_p, priority_p, flags_p);                                  \
	}

#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)                                 \
	{                                                                                          \
		Z_ISR_DECLARE_DIRECT(irq_p, ISR_FLAG_DIRECT, isr_p);                               \
		z_tricore_irq_config(irq_p, priority_p, flags_p);                                  \
	}

#define ARCH_ISR_DIRECT_HEADER()     arch_isr_direct_header()
#define ARCH_ISR_DIRECT_FOOTER(swap) arch_isr_direct_footer(swap)

#ifdef CONFIG_TRACING_ISR
extern void sys_trace_isr_enter(void);
extern void sys_trace_isr_exit(void);
#endif

static inline void arch_isr_direct_header(void)
{
#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_enter();
#endif
	/* We need to increment this so that arch_is_in_isr() keeps working */
	++(arch_curr_cpu()->nested);
}

static inline void arch_isr_direct_footer(int swap)
{
	ARG_UNUSED(swap);

	/* We are not in the ISR anymore */
	--(arch_curr_cpu()->nested);

#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_exit();
#endif
}

/*
 * TODO: Add support for rescheduling, add support for vector
 */
#define ARCH_ISR_DIRECT_DECLARE(name)                                                              \
	static inline int name##_body(void);                                                       \
	__attribute__((interrupt)) void name(void)                                                 \
	{                                                                                          \
		int swap;                                                                          \
		ISR_DIRECT_HEADER();                                                               \
		swap = name##_body();                                                              \
		ISR_DIRECT_FOOTER(swap);                                                           \
	}                                                                                          \
	static inline int name##_body(void)
#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_IRQ_H_ */
