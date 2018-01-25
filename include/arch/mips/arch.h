/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MIPS_ARCH_H_
#define _MIPS_ARCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "exp.h"
#include <irq.h>

#define STACK_ALIGN  8

#ifndef _ASMLANGUAGE
#include <stdlib.h> /* required to provide cpu.h with size_t */
#include <mips/cpu.h>
#include <mips/hal.h>
#include <misc/util.h>
#include <sw_isr_table.h>

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN)

/*
 * SOC-specific function to get the IRQ number generating the interrupt.
 * __soc_get_irq returns a bitfield of pending IRQs.
 */
extern u32_t __soc_get_irq(void);

void _arch_irq_enable(unsigned int irq);
void _arch_irq_disable(unsigned int irq);
int _arch_irq_is_enabled(unsigned int irq);
void _irq_spurious(void *unused);


/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
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
	_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
})

/*
 * Globally disable interrupts
 */
static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	if (mips_getsr() & SR_IE) {
		mips_bicsr(SR_IE);
		return 1;
	}
	return 0;
}

/*
 * Globally enable interrupts
 */
static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
	if (key) {
		mips_bissr(SR_IE);
	} else {
		mips_bicsr(SR_IE);
	}
}

extern u32_t _timer_cycle_get_32(void);
#define _arch_k_cycle_get_32()	_timer_cycle_get_32()

#endif /*_ASMLANGUAGE */

#if defined(CONFIG_SOC_DANUBE)
#include <arch/mips/danube/asm_inline.h>
#endif

#ifdef __cplusplus
}
#endif

#endif
