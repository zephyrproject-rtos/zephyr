/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nios II specific kernel interface header
 * This header contains the Nios II specific kernel interface.  It is
 * included by the generic kernel interface header (include/arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_NIOS2_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_NIOS2_ARCH_H_

#include <system.h>
#include <arch/nios2/asm_inline.h>
#include <generated_dts_board.h>
#include "nios2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ALIGN  4

#define _NANO_ERR_CPU_EXCEPTION (0)     /* Any unhandled exception */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_SPURIOUS_INT (4)	/* Spurious interrupt */
#define _NANO_ERR_KERNEL_OOPS (5)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (6)	/* Kernel panic (fatal to system) */

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <irq.h>
#include <sw_isr_table.h>

/* physical/virtual address types required by the kernel */
typedef unsigned int paddr_t;
typedef unsigned int vaddr_t;

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
 * 2. An instance of struct _isr_table_entry is created containing the ISR and
 * its parameter. If you look at how _sw_isr_table is created, each entry in
 * the array is in its own section named by the IRQ line number. What we are
 * doing here is to override one of the default entries (which points to the
 * spurious IRQ handler) with what was supplied here.
 *
 * There is no notion of priority with the Nios II internal interrupt
 * controller and no flags are currently supported.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority (ignored)
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ triggering options (currently unused)
 *
 * @return The vector assigned to this interrupt
 */
#define _ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	irq_p; \
})

extern void _irq_spurious(void *unused);

static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	unsigned int key, tmp;

	__asm__ volatile (
	    "rdctl %[key], status\n\t"
	    "movi %[tmp], -2\n\t"
	    "and %[tmp], %[key], %[tmp]\n\t"
	    "wrctl status, %[tmp]\n\t"
	    : [key] "=r" (key), [tmp] "=r" (tmp)
	    : : "memory");

	return key;
}

static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
	/* If the CPU is built without certain features, then
	 * the only writable bit in the status register is PIE
	 * in which case we can just write the value stored in key,
	 * all the other writable bits will be the same.
	 *
	 * If not, other stuff could have changed and we need to
	 * specifically flip just that bit.
	 */

#if (ALT_CPU_NUM_OF_SHADOW_REG_SETS > 0) || \
		(defined ALT_CPU_EIC_PRESENT) || \
		(defined ALT_CPU_MMU_PRESENT) || \
		(defined ALT_CPU_MPU_PRESENT)
	__asm__ volatile (
	    "andi %[key], %[key], 1\n\t"
	    "beq %[key], zero, 1f\n\t"
	    "rdctl %[key], status\n\t"
	    "ori %[key], %[key], 1\n\t"
	    "wrctl status, %[key]\n\t"
	    "1:\n\t"
	    : [key] "+r" (key)
	    : : "memory");
#else
	__asm__ volatile (
	    "wrctl status, %[key]"
	    : : [key] "r" (key)
	    : "memory");
#endif
}

void _arch_irq_enable(unsigned int irq);
void _arch_irq_disable(unsigned int irq);

struct __esf {
	u32_t ra; /* return address r31 */
	u32_t r1; /* at */
	u32_t r2; /* return value */
	u32_t r3; /* return value */
	u32_t r4; /* register args */
	u32_t r5; /* register args */
	u32_t r6; /* register args */
	u32_t r7; /* register args */
	u32_t r8; /* Caller-saved general purpose */
	u32_t r9; /* Caller-saved general purpose */
	u32_t r10; /* Caller-saved general purpose */
	u32_t r11; /* Caller-saved general purpose */
	u32_t r12; /* Caller-saved general purpose */
	u32_t r13; /* Caller-saved general purpose */
	u32_t r14; /* Caller-saved general purpose */
	u32_t r15; /* Caller-saved general purpose */
	u32_t estatus;
	u32_t instr; /* Instruction being executed when exc occurred */
};

typedef struct __esf NANO_ESF;
extern const NANO_ESF _default_esf;

FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason,
					 const NANO_ESF *esf);

FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *esf);

enum nios2_exception_cause {
	NIOS2_EXCEPTION_UNKNOWN                      = -1,
	NIOS2_EXCEPTION_RESET                        = 0,
	NIOS2_EXCEPTION_CPU_ONLY_RESET_REQUEST       = 1,
	NIOS2_EXCEPTION_INTERRUPT                    = 2,
	NIOS2_EXCEPTION_TRAP_INST                    = 3,
	NIOS2_EXCEPTION_UNIMPLEMENTED_INST           = 4,
	NIOS2_EXCEPTION_ILLEGAL_INST                 = 5,
	NIOS2_EXCEPTION_MISALIGNED_DATA_ADDR         = 6,
	NIOS2_EXCEPTION_MISALIGNED_TARGET_PC         = 7,
	NIOS2_EXCEPTION_DIVISION_ERROR               = 8,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_INST_ADDR    = 9,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_INST         = 10,
	NIOS2_EXCEPTION_SUPERVISOR_ONLY_DATA_ADDR    = 11,
	NIOS2_EXCEPTION_TLB_MISS                     = 12,
	NIOS2_EXCEPTION_TLB_EXECUTE_PERM_VIOLATION   = 13,
	NIOS2_EXCEPTION_TLB_READ_PERM_VIOLATION      = 14,
	NIOS2_EXCEPTION_TLB_WRITE_PERM_VIOLATION     = 15,
	NIOS2_EXCEPTION_MPU_INST_REGION_VIOLATION    = 16,
	NIOS2_EXCEPTION_MPU_DATA_REGION_VIOLATION    = 17,
	NIOS2_EXCEPTION_ECC_TLB_ERR                  = 18,
	NIOS2_EXCEPTION_ECC_FETCH_ERR                = 19,
	NIOS2_EXCEPTION_ECC_REGISTER_FILE_ERR        = 20,
	NIOS2_EXCEPTION_ECC_DATA_ERR                 = 21,
	NIOS2_EXCEPTION_ECC_DATA_CACHE_WRITEBACK_ERR = 22
};

/* Bitfield indicating which exception cause codes report a valid
 * badaddr register. NIOS2_EXCEPTION_TLB_MISS and NIOS2_EXCEPTION_ECC_TLB_ERR
 * are deliberately not included here, you need to check if TLBMISC.D=1
 */
#define NIOS2_BADADDR_CAUSE_MASK \
	(BIT(NIOS2_EXCEPTION_SUPERVISOR_ONLY_DATA_ADDR) | \
	 BIT(NIOS2_EXCEPTION_MISALIGNED_DATA_ADDR) | \
	 BIT(NIOS2_EXCEPTION_MISALIGNED_TARGET_PC) | \
	 BIT(NIOS2_EXCEPTION_TLB_READ_PERM_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_TLB_WRITE_PERM_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_MPU_DATA_REGION_VIOLATION) | \
	 BIT(NIOS2_EXCEPTION_ECC_DATA_ERR))


extern u32_t _timer_cycle_get_32(void);
#define _arch_k_cycle_get_32()	_timer_cycle_get_32()

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif
