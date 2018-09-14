/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa specific kernel interface header
 * This header contains the Xtensa specific kernel interface.  It is included
 * by the generic kernel interface header (include/arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_

#include <irq.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <generated_dts_board.h>
#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include "sys_io.h" /* Include from the very same folder of this file */
#include <zephyr/types.h>
#include <sw_isr_table.h>
#include <arch/xtensa/xtensa_irq.h>
#include <xtensa/config/core.h>

#define STACK_ALIGN 16

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_RESERVED_IRQ (4)	/* Reserved interrupt */
#define _NANO_ERR_KERNEL_OOPS (5)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (6)	/* Kernel panic (fatal to system) */

/* Xtensa GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { u32_t name1, name2; }

#include <arch/xtensa/exc.h>

/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_msb_set(u32_t op)
{
	if (!op)
		return 0;
	return 32 - __builtin_clz(op);
}

/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_lsb_set(u32_t op)
{
	return __builtin_ffs(op);
}

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void _irq_priority_set(u32_t irq, u32_t prio, u32_t flags);


/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time; if this
 * can't be done use irq_connect_dynamic() instead.
 *
 * Internally this function does a few things:
 *
 * 1. The enum statement has no effect but forces the compiler to only
 * accept constant values for the irq_p parameter, very important as the
 * numerical IRQ line is used to create a named section.
 *
 * 2. An instance of _isr_table_entry is created containing the ISR and its
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
	_ISR_DECLARE(irq_p, flags_p, isr_p, isr_param_p); \
	irq_p; \
})

/* Spurious interrupt handler. Throws an error if called */
extern void _irq_spurious(void *unused);

#ifdef CONFIG_XTENSA_ASM2
#define XTENSA_ERR_NORET /**/
#else
#define XTENSA_ERR_NORET FUNC_NORETURN
#endif

XTENSA_ERR_NORET void _SysFatalErrorHandler(unsigned int reason,
					    const NANO_ESF *esf);

XTENSA_ERR_NORET void _NanoFatalErrorHandler(unsigned int reason,
					     const NANO_ESF *pEsf);

extern u32_t _timer_cycle_get_32(void);
#define _arch_k_cycle_get_32()	_timer_cycle_get_32()

#endif /* !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)  */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_ */
