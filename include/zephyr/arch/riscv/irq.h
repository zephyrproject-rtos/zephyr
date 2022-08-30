/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V public interrupt handling
 *
 * RISC-V-specific kernel interrupt handling interface.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_H_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <stdbool.h>
#include <soc.h>

extern void arch_irq_enable(unsigned int irq);
extern void arch_irq_disable(unsigned int irq);
extern int arch_irq_is_enabled(unsigned int irq);

#if defined(CONFIG_RISCV_HAS_PLIC) || defined(CONFIG_RISCV_HAS_CLIC)
extern void z_riscv_irq_priority_set(unsigned int irq,
				     unsigned int prio,
				     uint32_t flags);
#else
#define z_riscv_irq_priority_set(i, p, f) /* Nothing */
#endif /* CONFIG_RISCV_HAS_PLIC || CONFIG_RISCV_HAS_CLIC */

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	z_riscv_irq_priority_set(irq_p, priority_p, flags_p); \
}

#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, ISR_FLAG_DIRECT, isr_p, NULL); \
}

#define ARCH_ISR_DIRECT_HEADER() arch_isr_direct_header()
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

extern void __soc_handle_irq(ulong_t mcause);

static inline void arch_isr_direct_footer(int swap)
{
	ulong_t mcause;

	/* Get the IRQ number */
	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));
	mcause &= SOC_MCAUSE_EXP_MASK;

	/* Clear the pending IRQ */
	__soc_handle_irq(mcause);

	/* We are not in the ISR anymore */
	--(arch_curr_cpu()->nested);

#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_exit();
#endif
}

/*
 * TODO: Add support for rescheduling
 */
#define ARCH_ISR_DIRECT_DECLARE(name) \
	static inline int name##_body(void); \
	__attribute__ ((interrupt)) void name(void) \
	{ \
		ISR_DIRECT_HEADER(); \
		name##_body(); \
		ISR_DIRECT_FOOTER(0); \
	} \
	static inline int name##_body(void)


#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_H_ */
