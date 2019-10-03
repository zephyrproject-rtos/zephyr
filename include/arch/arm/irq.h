/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-M public interrupt handling
 *
 * ARM-specific kernel interrupt handling interface. Included by arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_IRQ_H_

#include <irq.h>
#include <sw_isr_table.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(z_arm_int_exit);
GTEXT(z_arch_irq_enable)
GTEXT(z_arch_irq_disable)
GTEXT(z_arch_irq_is_enabled)
#else
extern void z_arch_irq_enable(unsigned int irq);
extern void z_arch_irq_disable(unsigned int irq);
extern int z_arch_irq_is_enabled(unsigned int irq);

extern void z_arm_int_exit(void);

#if defined(CONFIG_ARMV7_R)
static ALWAYS_INLINE void z_arm_int_lib_init(void)
{
}
#else
extern void z_arm_int_lib_init(void);
#endif

/* macros convert value of it's argument to a string */
#define DO_TOSTR(s) #s
#define TOSTR(s) DO_TOSTR(s)

/* concatenate the values of the arguments into one */
#define DO_CONCAT(x, y) x ## y
#define CONCAT(x, y) DO_CONCAT(x, y)

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void z_arm_irq_priority_set(unsigned int irq, unsigned int prio,
				   u32_t flags);


/* Flags for use with IRQ_CONNECT() */
#ifdef CONFIG_ZERO_LATENCY_IRQS
/**
 * Set this interrupt up as a zero-latency IRQ. It has a fixed hardware
 * priority level (discarding what was supplied in the interrupt's priority
 * argument), and will run even if irq_lock() is active. Be careful!
 */
#define IRQ_ZERO_LATENCY	BIT(0)
#endif


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
#define Z_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	z_arm_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})

#define Z_ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
({ \
	Z_ISR_DECLARE(irq_p, ISR_FLAG_DIRECT, isr_p, NULL); \
	z_arm_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})

/* FIXME prefer these inline, but see GH-3056 */
#ifdef CONFIG_SYS_POWER_MANAGEMENT
extern void _arch_isr_direct_pm(void);
#define Z_ARCH_ISR_DIRECT_PM() _arch_isr_direct_pm()
#else
#define Z_ARCH_ISR_DIRECT_PM() do { } while (false)
#endif

#define Z_ARCH_ISR_DIRECT_HEADER() z_arch_isr_direct_header()
extern void z_arch_isr_direct_header(void);

#define Z_ARCH_ISR_DIRECT_FOOTER(swap) z_arch_isr_direct_footer(swap)

/* arch/arm/core/exc_exit.S */
extern void z_arm_int_exit(void);

#ifdef CONFIG_TRACING
extern void sys_trace_isr_exit(void);
#endif

static inline void z_arch_isr_direct_footer(int maybe_swap)
{

#ifdef CONFIG_TRACING
	sys_trace_isr_exit();
#endif
	if (maybe_swap) {
		z_arm_int_exit();
	}
}

#define Z_ARCH_ISR_DIRECT_DECLARE(name) \
	static inline int name##_body(void); \
	__attribute__ ((interrupt ("IRQ"))) void name(void) \
	{ \
		int check_reschedule; \
		ISR_DIRECT_HEADER(); \
		check_reschedule = name##_body(); \
		ISR_DIRECT_FOOTER(check_reschedule); \
	} \
	static inline int name##_body(void)

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

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_IRQ_H_ */
