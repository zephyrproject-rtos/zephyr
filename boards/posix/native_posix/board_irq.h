/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BOARD_IRQ_H
#define _BOARD_IRQ_H

#include "sw_isr_table.h"
#include "zephyr/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void _isr_declare(unsigned int irq_p, int flags, void isr_p(void *),
		void *isr_param_p);
void _irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags);

/**
 * Configure a static interrupt.
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
	_isr_declare(irq_p, 0, isr_p, isr_param_p); \
	_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})


/**
 * Configure a 'direct' static interrupt.
 *
 * See include/irq.h for details.
 */
#define _ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
({ \
	_isr_declare(irq_p, ISR_FLAG_DIRECT, (void (*)(void *))isr_p, NULL); \
	_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})


/**
 * The return of "name(void)" is the indicaton of the interrupt
 * (maybe) having caused a kernel decision to context switch
 *
 * Note that this convention is changed relative to the ARM and x86 archs
 */
#define _ARCH_ISR_DIRECT_DECLARE(name) \
	static inline int name##_body(void); \
	int name(void) \
	{ \
		int check_reschedule; \
		check_reschedule = name##_body(); \
		return check_reschedule; \
	} \
	static inline int name##_body(void)

#define _ARCH_ISR_DIRECT_PM()

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_IRQ_H */
