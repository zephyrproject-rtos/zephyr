/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *               2020 RISE Research Institutes of Sweden <www.ri.se>
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

#include <arch/riscv/syscall.h>
#include <arch/riscv/thread.h>
#include <arch/riscv/exp.h>
#include <arch/common/sys_io.h>
#include <arch/common/ffs.h>

#include <irq.h>
#include <sw_isr_table.h>
#include <soc.h>
#include <devicetree.h>

/* stacks, for RISCV architecture stack should be 16byte-aligned */
#define ARCH_STACK_PTR_ALIGN  16

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

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
#define RV_OP_LOADFPREG fld
#define RV_OP_STOREFPREG fsd
#else
#define RV_OP_LOADFPREG flw
#define RV_OP_STOREFPREG fsw
#endif

/* Common mstatus bits. All supported cores today have the same
 * layouts.
 */

#define MSTATUS_IEN     (1UL << 3)
#define MSTATUS_MPP_M   (3UL << 11)
#define MSTATUS_MPIE_EN (1UL << 7)
#define MSTATUS_FS_INIT (1UL << 13)
#define MSTATUS_FS_MASK ((1UL << 13) | (1UL << 14))

/* This comes from openisa_rv32m1, but doesn't seem to hurt on other
 * platforms:
 * - Preserve machine privileges in MPP. If you see any documentation
 *   telling you that MPP is read-only on this SoC, don't believe its
 *   lies.
 * - Enable interrupts when exiting from exception into a new thread
 *   by setting MPIE now, so it will be copied into IE on mret.
 */
#define MSTATUS_DEF_RESTORE (MSTATUS_MPP_M | MSTATUS_MPIE_EN)

#ifndef _ASMLANGUAGE
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ROUND_UP(x) ROUND_UP(x, ARCH_STACK_PTR_ALIGN)

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
extern uint32_t __soc_get_irq(void);

void arch_irq_enable(unsigned int irq);
void arch_irq_disable(unsigned int irq);
int arch_irq_is_enabled(unsigned int irq);
void arch_irq_priority_set(unsigned int irq, unsigned int prio);
void z_irq_spurious(void *unused);

#if defined(CONFIG_RISCV_HAS_PLIC)
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	arch_irq_priority_set(irq_p, priority_p); \
}
#else
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
}
#endif

/*
 * use atomic instruction csrrc to lock global irq
 * csrrc: atomic read and clear bits in CSR register
 */
static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key;
	ulong_t mstatus;

	__asm__ volatile ("csrrc %0, mstatus, %1"
			  : "=r" (mstatus)
			  : "r" (MSTATUS_IEN)
			  : "memory");

	key = (mstatus & MSTATUS_IEN);
	return key;
}

/*
 * use atomic instruction csrrs to unlock global irq
 * csrrs: atomic read and set bits in CSR register
 */
static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	ulong_t mstatus;

	__asm__ volatile ("csrrs %0, mstatus, %1"
			  : "=r" (mstatus)
			  : "r" (key & MSTATUS_IEN)
			  : "memory");
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	/* FIXME: looking at arch_irq_lock, this should be reducable
	 * to just testing that key is nonzero (because it should only
	 * have the single bit set).  But there is a mask applied to
	 * the argument in arch_irq_unlock() that has me worried
	 * that something elseswhere might try to set a bit?  Do it
	 * the safe way for now.
	 */
	return (key & MSTATUS_IEN) == MSTATUS_IEN;
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

extern uint32_t z_timer_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return z_timer_cycle_get_32();
}

#ifdef CONFIG_USERSPACE

typedef uint32_t k_mem_partition_attr_t;

#define Z_PRIVILEGE_STACK_ALIGN 4

/* Bit offset to segments within pmpcfg# registers */
#define RV_CFG_OFFSET	8

/* PMP addressing modes */
#define RV_PMP_OFF	(0 << 3)
#define RV_PMP_TOR	(1 << 3)
#define RV_PMP_NA4	(2 << 3)
#define RV_PMP_NAPOT	(3 << 3)

/* PMP permissions */
#define RV_PMP_R	(1 << 0)
#define RV_PMP_W	(1 << 1)
#define RV_PMP_X	(1 << 2)
#define RV_PMP_RO	(RV_PMP_R)
#define RV_PMP_RW	(RV_PMP_R | RV_PMP_W)
#define RV_PMP_RX	(RV_PMP_R | RV_PMP_X)
#define RV_PMP_RWX	(RV_PMP_R | RV_PMP_W | RV_PMP_X)

/* Read/write/execute access permission attributes */
#define K_MEM_PARTITION_P_NA_U_NA	(RV_PMP_TOR)
#define K_MEM_PARTITION_P_RW_U_RW	(RV_PMP_TOR | RV_PMP_RW)
#define K_MEM_PARTITION_P_RW_U_RO	(RV_PMP_TOR | RV_PMP_R)
#define K_MEM_PARTITION_P_RW_U_NA	(RV_PMP_TOR)
#define K_MEM_PARTITION_P_RO_U_RO	(RV_PMP_TOR | RV_PMP_R)
#define K_MEM_PARTITION_P_RO_U_NA	(RV_PMP_TOR)
#define K_MEM_PARTITION_P_RWX_U_RWX	(RV_PMP_TOR | RV_PMP_RWX)
#define K_MEM_PARTITION_P_RWX_U_RX	(RV_PMP_TOR | RV_PMP_RX)
#define K_MEM_PARTITION_P_RX_U_RX	(RV_PMP_TOR | RV_PMP_RX)

#define POW2_CEIL(x) ((1 << (31 - __builtin_clz(x))) < x ?  \
		1 << (31 - __builtin_clz(x) + 1) : \
		1 << (31 - __builtin_clz(x)))

/* Macros for encoding and decoding PMP NAPOT address registers */ 
#define RV_NAPOT_CALC(addr, size) ((0xffffffff >> \
		(32 - (__builtin_ctz(POW2_CEIL(size) >> 3)))) | \
		(((uint32_t)addr) >> 2))
#define RV_NAPOT_SIZE(pmpaddr) (1 << (__builtin_ctz(~pmpaddr) + 3))
#define RV_NAPOT_ADDR(pmpaddr) ((pmpaddr >> __builtin_ctz(~pmpaddr)) \
		<< (__builtin_ctz(~pmpaddr) + 2))

/*
 * The following macros generate naturally-aligned power-of-two privilege stacks
 * for userspace threads. See sys/thread_stack.h for more information.
 */
#define ARCH_THREAD_STACK_LEN(size) (POW2_CEIL(size))
#define ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct z_thread_stack_element __aligned(POW2_CEIL(size)) \
		sym[POW2_CEIL(size)]
#define ARCH_THREAD_STACK_SIZEOF(sym) (sizeof(sym))
#define ARCH_THREAD_STACK_BUFFER(sym) ((char *)(sym))
#define ARCH_THREAD_STACK_RESERVED 0
#define ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct z_thread_stack_element __noinit \
		__aligned(POW2_CEIL(size)) \
		sym[nmemb][ARCH_THREAD_STACK_LEN(size)]
#define ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct z_thread_stack_element __noinit \
		__aligned(POW2_CEIL(size)) sym[POW2_CEIL(size)]

#endif /* CONFIG_USERSPACE */

#ifdef __cplusplus
}
#endif

#endif /*_ASMLANGUAGE */

#if defined(CONFIG_SOC_FAMILY_RISCV_PRIVILEGE)
#include <arch/riscv/riscv-privilege/asm_inline.h>
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_H_ */
