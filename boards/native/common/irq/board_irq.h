/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARDS_POSIX_COMMON_BOARD_IRQ_H
#define BOARDS_POSIX_COMMON_BOARD_IRQ_H

#include <zephyr/sw_isr_table.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void posix_isr_declare(unsigned int irq_p, int flags, void isr_p(const void *),
		       const void *isr_param_p);
void posix_irq_priority_set(unsigned int irq, unsigned int prio,
			    uint32_t flags);

/**
 * Configure a static interrupt.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 */
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	posix_isr_declare(irq_p, 0, isr_p, isr_param_p); \
	posix_irq_priority_set(irq_p, priority_p, flags_p); \
}


/**
 * Configure a 'direct' static interrupt.
 *
 * See include/irq.h for details.
 */
#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
{ \
	posix_isr_declare(irq_p, ISR_FLAG_DIRECT, \
			  (void (*)(const void *))isr_p, NULL); \
	posix_irq_priority_set(irq_p, priority_p, flags_p); \
}

/**
 * POSIX Architecture (board) specific ISR_DIRECT_DECLARE(),
 * See include/irq.h for more information.
 *
 * The return of "name##_body(void)" is the indication of the interrupt
 * (maybe) having caused a kernel decision to context switch
 *
 * Note that this convention is changed relative to the ARM and x86 archs
 *
 * All pre/post irq work of the interrupt is handled in the board
 * posix_irq_handler() both for direct and normal interrupts together
 */
#define ARCH_ISR_DIRECT_DECLARE(name) \
	static inline int name##_body(void); \
	int name(void) \
	{ \
		int check_reschedule; \
		check_reschedule = name##_body(); \
		return check_reschedule; \
	} \
	static inline int name##_body(void)

#define ARCH_ISR_DIRECT_HEADER()   do { } while (false)
#define ARCH_ISR_DIRECT_FOOTER(a)  do { } while (false)

#ifdef CONFIG_PM
extern void posix_irq_check_idle_exit(void);
#define ARCH_ISR_DIRECT_PM() posix_irq_check_idle_exit()
#else
#define ARCH_ISR_DIRECT_PM() do { } while (false)
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_COMMON_BOARD_IRQ_H */
