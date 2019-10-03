/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISCV specific kernel interface header
 * This header contains the RISCV specific kernel interface.  It is
 * included by the generic kernel interface header (arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_H_

#include "exp.h"
#include <arch/common/sys_io.h>
#include <arch/common/ffs.h>

#include <irq.h>
#include <sw_isr_table.h>
#include <soc.h>
#include <generated_dts_board.h>

/* stacks, for RISCV architecture stack should be 16byte-aligned */
#define STACK_ALIGN  16

#ifdef CONFIG_64BIT
#define RV_OP_LOADREG ld
#define RV_OP_STOREREG sd
#define RV_REGSIZE 8
#define RV_REGSHIFT 3
#else
#define RV_OP_LOADREG lw
#define RV_OP_STOREREG sw
#define RV_REGSIZE 4
#define RV_REGSHIFT 2
#endif

#ifndef _ASMLANGUAGE
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN)

/* macros convert value of its argument to a string */
#define DO_TOSTR(s) #s
#define TOSTR(s) DO_TOSTR(s)

/* concatenate the values of the arguments into one */
#define DO_CONCAT(x, y) x ## y
#define CONCAT(x, y) DO_CONCAT(x, y)

/*
 * SOC-specific function to get the IRQ number generating the interrupt.
 * __soc_get_irq returns a bitfield of pending IRQs.
 */
extern u32_t __soc_get_irq(void);

void z_arch_irq_enable(unsigned int irq);
void z_arch_irq_disable(unsigned int irq);
int z_arch_irq_is_enabled(unsigned int irq);
void z_arch_irq_priority_set(unsigned int irq, unsigned int prio);
void z_irq_spurious(void *unused);

#if defined(CONFIG_RISCV_HAS_PLIC)
#define Z_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	z_arch_irq_priority_set(irq_p, priority_p); \
	irq_p; \
})
#else
#define Z_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	irq_p; \
})
#endif

/*
 * use atomic instruction csrrc to lock global irq
 * csrrc: atomic read and clear bits in CSR register
 */
static ALWAYS_INLINE unsigned int z_arch_irq_lock(void)
{
	unsigned int key;
	ulong_t mstatus;

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
static ALWAYS_INLINE void z_arch_irq_unlock(unsigned int key)
{
	ulong_t mstatus;

	__asm__ volatile ("csrrs %0, mstatus, %1"
			  : "=r" (mstatus)
			  : "r" (key & SOC_MSTATUS_IEN)
			  : "memory");
}

static ALWAYS_INLINE bool z_arch_irq_unlocked(unsigned int key)
{
	/* FIXME: looking at z_arch_irq_lock, this should be reducable
	 * to just testing that key is nonzero (because it should only
	 * have the single bit set).  But there is a mask applied to
	 * the argument in z_arch_irq_unlock() that has me worried
	 * that something elseswhere might try to set a bit?  Do it
	 * the safe way for now.
	 */
	return (key & SOC_MSTATUS_IEN) == SOC_MSTATUS_IEN;
}

static ALWAYS_INLINE void z_arch_nop(void)
{
	__asm__ volatile("nop");
}

extern u32_t z_timer_cycle_get_32(void);

static inline u32_t z_arch_k_cycle_get_32(void)
{
	return z_timer_cycle_get_32();
}

#ifdef __cplusplus
}
#endif

#endif /*_ASMLANGUAGE */

#if defined(CONFIG_SOC_FAMILY_RISCV_PRIVILEGE)
#include <arch/riscv/riscv-privilege/asm_inline.h>
#endif


#endif
