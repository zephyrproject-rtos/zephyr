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

#ifndef _ARCH_ARM_CORTEXM_IRQ_H_
#define _ARCH_ARM_CORTEXM_IRQ_H_

#include <irq.h>
#include <sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(_IntExit);
GTEXT(_arch_irq_enable)
GTEXT(_arch_irq_disable)
GTEXT(_arch_irq_is_enabled)
#else
extern void _arch_irq_enable(unsigned int irq);
extern void _arch_irq_disable(unsigned int irq);
extern int _arch_irq_is_enabled(unsigned int irq);

extern void _IntExit(void);

/* macros convert value of it's argument to a string */
#define DO_TOSTR(s) #s
#define TOSTR(s) DO_TOSTR(s)

/* concatenate the values of the arguments into one */
#define DO_CONCAT(x, y) x ## y
#define CONCAT(x, y) DO_CONCAT(x, y)

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void _irq_priority_set(unsigned int irq, unsigned int prio,
			      uint32_t flags);


/* Flags for use with IRQ_CONNECT() */
#if CONFIG_ZERO_LATENCY_IRQS
/**
 * Set this interrupt up as a zero-latency IRQ. It has a fixed hardware
 * priority level (discarding what was supplied in the interrupt's priority
 * argument), and will run even if irq_lock() is active. Be careful!
 */
#define IRQ_ZERO_LATENCY	(1 << 0)
#endif


/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
 *
 * Internally this function does a few things:
 *
 * 1. The enum statement has no effect but forces the compiler to only
 * accept constant values for the irq_p parameter, very important as the
 * numerical IRQ line is used to create a named section.
 *
 * 2. An instance of _IsrTableEntry is created containing the ISR and its
 * parameter. If you look at how _sw_isr_table is created, each entry in the
 * array is in its own section named by the IRQ line number. What we are doing
 * here is to override one of the default entries (which points to the
 * spurious IRQ handler) with what was supplied here.
 *
 * 3. The priority level for the interrupt is configured by a call to
 * _irq_priority_set()
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 *
 * @return The vector assigned to this interrupt
 */
#define _ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	enum { IRQ = irq_p }; \
	static struct _IsrTableEntry _CONCAT(_isr_irq, irq_p) \
		__attribute__ ((used)) \
		__attribute__ ((section(STRINGIFY(_CONCAT(.gnu.linkonce.isr_irq, irq_p))))) = \
			{isr_param_p, isr_p}; \
	_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_ARM_CORTEXM_IRQ_H_ */
