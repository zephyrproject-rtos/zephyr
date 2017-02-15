/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISCV32 specific kernel interface header
 * This header contains the RISCV32 specific kernel interface.  It is
 * included by the generic kernel interface header (arch/cpu.h)
 */

#ifndef _RISCV32_ARCH_H_
#define _RISCV32_ARCH_H_

#include "exp.h"
#include "sys_io.h"

#include <irq.h>
#include <sw_isr_table.h>
#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* stacks, for RISCV architecture stack should be 16byte-aligned */
#define STACK_ALIGN  16

#ifndef _ASMLANGUAGE
#include <misc/util.h>

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN)

/* APIs need to support non-byte addressable architectures */

#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

/* macros convert value of it's argument to a string */
#define DO_TOSTR(s) #s
#define TOSTR(s) DO_TOSTR(s)

/* concatenate the values of the arguments into one */
#define DO_CONCAT(x, y) x ## y
#define CONCAT(x, y) DO_CONCAT(x, y)

/*
 * SOC-specific function to get the IRQ number generating the interrupt.
 * __soc_get_irq returns a bitfield of pending IRQs.
 */
extern uint32_t __soc_get_irq(void);

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
	irq_p; \
})

/*
 * use atomic instruction csrrc to lock global irq
 * csrrc: atomic read and clear bits in CSR register
 */
static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	unsigned int key, mstatus;

	__asm__ volatile ("csrrc %0, mstatus, %1"
			  : "=r" (mstatus)
			  : "r" (SOC_MSTATUS_IEN)
			  : "memory");

	key = (mstatus & SOC_MSTATUS_IEN);
	return key;
}

/*
 * use atomic instruction csrrs to unlock global irq
 * csrrs: atomic read and set bits in CSR register
 */
static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
	unsigned int mstatus;

	__asm__ volatile ("csrrs %0, mstatus, %1"
			  : "=r" (mstatus)
			  : "r" (key & SOC_MSTATUS_IEN)
			  : "memory");
}

extern uint32_t _timer_cycle_get_32(void);
#define _arch_k_cycle_get_32()	_timer_cycle_get_32()

#endif /*_ASMLANGUAGE */

#if defined(CONFIG_SOC_RISCV32_PULPINO)
#include <arch/riscv32/pulpino/asm_inline.h>
#elif defined(CONFIG_SOC_RISCV32_QEMU)
#include <arch/riscv32/riscv32-qemu/asm_inline.h>
#endif

#ifdef __cplusplus
}
#endif

#endif
