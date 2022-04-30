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

#include <arch/riscv/thread.h>
#include <arch/riscv/exp.h>
#include <arch/common/sys_bitops.h>
#include <arch/common/sys_io.h>
#include <arch/common/ffs.h>
#if defined(CONFIG_USERSPACE)
#include <arch/riscv/syscall.h>
#endif /* CONFIG_USERSPACE */
#include <irq.h>
#include <sw_isr_table.h>
#include <soc.h>
#include <devicetree.h>
#include <arch/riscv/csr.h>
#include <arch/riscv/exp.h>

/* stacks, for RISCV architecture stack should be 16byte-aligned */
#define ARCH_STACK_PTR_ALIGN  16

#ifdef CONFIG_PMP_STACK_GUARD
/*
 * The StackGuard is an area at the bottom of the kernel-mode stack made to
 * fault when accessed. It is _not_ faulting when in exception mode as we rely
 * on that area to save the exception stack frame and to process said fault.
 * Therefore the guard area must be large enough to hold the esf, plus some
 * configurable stack wiggle room to execute the fault handling code off of,
 * as well as some guard size to cover possible sudden stack pointer
 * displacement before the fault.
 *
 * The m-mode PMP set is not overly used so no need to force NAPOT.
 */
#define Z_RISCV_STACK_GUARD_SIZE \
	ROUND_UP(sizeof(z_arch_esf_t) + CONFIG_PMP_STACK_GUARD_MIN_SIZE, \
		 ARCH_STACK_PTR_ALIGN)

/* Kernel-only stacks have the following layout if a stack guard is enabled:
 *
 * +------------+ <- thread.stack_obj
 * | Guard      | } Z_RISCV_STACK_GUARD_SIZE
 * +------------+ <- thread.stack_info.start
 * | Kernel     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 */
#define ARCH_KERNEL_STACK_RESERVED	Z_RISCV_STACK_GUARD_SIZE

#else /* !CONFIG_PMP_STACK_GUARD */
#define Z_RISCV_STACK_GUARD_SIZE 0
#endif

#ifdef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
/* The privilege elevation stack is located in another area of memory
 * generated at build time by gen_kobject_list.py
 *
 * +------------+ <- thread.arch.priv_stack_start
 * | Guard      | } Z_RISCV_STACK_GUARD_SIZE
 * +------------+
 * | Priv Stack | } CONFIG_PRIVILEGED_STACK_SIZE
 * +------------+ <- thread.arch.priv_stack_start +
 *                   CONFIG_PRIVILEGED_STACK_SIZE +
 *                   Z_RISCV_STACK_GUARD_SIZE
 *
 * The main stack will be initially (or potentially only) used by kernel
 * mode so we need to make room for a possible stack guard area when enabled:
 *
 * +------------+ <- thread.stack_obj
 * | Guard      | } Z_RISCV_STACK_GUARD_SIZE
 * +............| <- thread.stack_info.start
 * | Thread     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 *
 * When transitioning to user space, the guard area will be removed from
 * the main stack. Any thread running in user mode will have full access
 * to the region denoted by thread.stack_info. Make it PMP-NAPOT compatible.
 *
 * +------------+ <- thread.stack_obj = thread.stack_info.start
 * | Thread     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 */
#define ARCH_THREAD_STACK_RESERVED Z_RISCV_STACK_GUARD_SIZE
#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
		Z_POW2_CEIL(MAX(size, CONFIG_PRIVILEGED_STACK_SIZE))
#define ARCH_THREAD_STACK_OBJ_ALIGN(size) \
		ARCH_THREAD_STACK_SIZE_ADJUST(size)

#else /* !CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT */

/* The stack object will contain the PMP guard, the privilege stack, and then
 * the usermode stack buffer in that order:
 *
 * +------------+ <- thread.stack_obj
 * | Guard      | } Z_RISCV_STACK_GUARD_SIZE
 * +------------+
 * | Priv Stack | } CONFIG_PRIVILEGED_STACK_SIZE
 * +------------+ <- thread.stack_info.start
 * | Thread     |
 * | stack      |
 * |            |
 * +............|
 * | TLS        | } thread.stack_info.delta
 * +------------+ <- thread.stack_info.start + thread.stack_info.size
 */
#define ARCH_THREAD_STACK_RESERVED \
	ROUND_UP(Z_RISCV_STACK_GUARD_SIZE + CONFIG_PRIVILEGED_STACK_SIZE, \
		 ARCH_STACK_PTR_ALIGN)

#endif /* CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT */

#ifdef CONFIG_64BIT
#define RV_REGSIZE 8
#define RV_REGSHIFT 3
#else
#define RV_REGSIZE 4
#define RV_REGSHIFT 2
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

/* Kernel macros for memory attribution
 * (access permissions and cache-ability).
 *
 * The macros are to be stored in k_mem_partition_attr_t
 * objects. The format of a k_mem_partition_attr_t object
 * is an uint8_t composed by configuration register flags
 * located in arch/riscv/include/core_pmp.h
 */

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW ((k_mem_partition_attr_t) \
	{PMP_R | PMP_W})
#define K_MEM_PARTITION_P_RW_U_RO ((k_mem_partition_attr_t) \
	{PMP_R})
#define K_MEM_PARTITION_P_RW_U_NA ((k_mem_partition_attr_t) \
	{0})
#define K_MEM_PARTITION_P_RO_U_RO ((k_mem_partition_attr_t) \
	{PMP_R})
#define K_MEM_PARTITION_P_RO_U_NA ((k_mem_partition_attr_t) \
	{0})
#define K_MEM_PARTITION_P_NA_U_NA ((k_mem_partition_attr_t) \
	{0})

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX ((k_mem_partition_attr_t) \
	{PMP_R | PMP_W | PMP_X})
#define K_MEM_PARTITION_P_RX_U_RX ((k_mem_partition_attr_t) \
	{PMP_R | PMP_X})

/* Typedef for the k_mem_partition attribute */
typedef struct {
	uint8_t pmp_attr;
} k_mem_partition_attr_t;

struct arch_mem_domain {
	unsigned int pmp_update_nr;
};

void arch_irq_enable(unsigned int irq);
void arch_irq_disable(unsigned int irq);
int arch_irq_is_enabled(unsigned int irq);
void arch_irq_priority_set(unsigned int irq, unsigned int prio);
void z_irq_spurious(const void *unused);

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

extern uint32_t sys_clock_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

#ifdef CONFIG_USERSPACE
#include <arch/riscv/error.h>
#endif /* CONFIG_USERSPACE */

#ifdef __cplusplus
}
#endif

#endif /*_ASMLANGUAGE */

#if defined(CONFIG_SOC_FAMILY_RISCV_PRIVILEGE)
#include <arch/riscv/riscv-privilege/asm_inline.h>
#endif


#endif
