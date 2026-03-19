/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISCV specific kernel interface header
 *
 * This header contains the RISCV specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * (include/zephyr/arch/cpu.h).
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_H_

#include <zephyr/arch/riscv/thread.h>
#include <zephyr/arch/exception.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/arch/riscv/sys_io.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/ffs.h>
#if defined(CONFIG_USERSPACE)
#include <zephyr/arch/riscv/syscall.h>
#endif /* CONFIG_USERSPACE */
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/riscv/csr.h>

/* stacks, for RISCV architecture stack should be 16byte-aligned */
#define ARCH_STACK_PTR_ALIGN  16

#define Z_RISCV_STACK_PMP_ALIGN \
	MAX(CONFIG_PMP_GRANULARITY, ARCH_STACK_PTR_ALIGN)

#ifdef CONFIG_PMP_STACK_GUARD
/*
 * The StackGuard is an area at the bottom of the kernel-mode stack made to
 * fault when accessed. It is _not_ faulting when in exception mode as we rely
 * on that area to save the exception stack frame and to process said fault.
 * Therefore the guard area must be large enough to hold the esf, plus some
 * configurable stack wiggle room to execute the fault handling code off of,
 * as well as some guard size to cover possible sudden stack pointer
 * displacement before the fault.
 */
#ifdef CONFIG_PMP_POWER_OF_TWO_ALIGNMENT
#define Z_RISCV_STACK_GUARD_SIZE \
	Z_POW2_CEIL(MAX(sizeof(struct arch_esf) + CONFIG_PMP_STACK_GUARD_MIN_SIZE, \
			Z_RISCV_STACK_PMP_ALIGN))
#define ARCH_KERNEL_STACK_OBJ_ALIGN	Z_RISCV_STACK_GUARD_SIZE
#else
#define Z_RISCV_STACK_GUARD_SIZE \
	ROUND_UP(sizeof(struct arch_esf) + CONFIG_PMP_STACK_GUARD_MIN_SIZE, \
		 Z_RISCV_STACK_PMP_ALIGN)
#define ARCH_KERNEL_STACK_OBJ_ALIGN	Z_RISCV_STACK_PMP_ALIGN
#endif

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

#elif defined(CONFIG_RISCV_MMU)
/* MMU guard page: one page unmapped at the bottom of every stack object */
#define Z_RISCV_STACK_GUARD_SIZE	CONFIG_MMU_PAGE_SIZE
#define ARCH_KERNEL_STACK_OBJ_ALIGN	CONFIG_MMU_PAGE_SIZE
#define ARCH_KERNEL_STACK_RESERVED	Z_RISCV_STACK_GUARD_SIZE

#else /* !CONFIG_PMP_STACK_GUARD && !CONFIG_RISCV_MMU */
#define Z_RISCV_STACK_GUARD_SIZE 0
#endif

#ifdef CONFIG_RISCV_MMU

/* MMU thread stack layout:
 *
 * +------------+ <- thread.stack_obj
 * | Guard page | } PAGE_SIZE (unmapped in kernel page tables)
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
		 CONFIG_MMU_PAGE_SIZE)
#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
	ROUND_UP(size, CONFIG_MMU_PAGE_SIZE)
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	CONFIG_MMU_PAGE_SIZE

#elif defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT)
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
	Z_POW2_CEIL(MAX(MAX(size, CONFIG_PRIVILEGED_STACK_SIZE), \
			Z_RISCV_STACK_PMP_ALIGN))
#define ARCH_THREAD_STACK_OBJ_ALIGN(size) \
		ARCH_THREAD_STACK_SIZE_ADJUST(size)

#else /* !CONFIG_RISCV_MMU && !CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */

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
		 Z_RISCV_STACK_PMP_ALIGN)
#define ARCH_THREAD_STACK_SIZE_ADJUST(size) \
	ROUND_UP(size, Z_RISCV_STACK_PMP_ALIGN)
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)	Z_RISCV_STACK_PMP_ALIGN
#endif /* CONFIG_RISCV_MMU / CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */

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
/** @brief mstatus MPP field value for User mode */
#define MSTATUS_MPP_U   (PRV_U << 11)
/** @brief mstatus MPP field value for Supervisor mode */
#define MSTATUS_MPP_S   (PRV_S << 11)
#define MSTATUS_MPP_M   (PRV_M << 11)
#define MSTATUS_MPIE_EN (1UL << 7)

#define MSTATUS_FS_OFF   (0UL << 13)
#define MSTATUS_FS_INIT  (1UL << 13)
#define MSTATUS_FS_CLEAN (2UL << 13)
#define MSTATUS_FS_DIRTY (3UL << 13)

/* This comes from openisa_rv32m1, but doesn't seem to hurt on other
 * platforms:
 * - Preserve machine privileges in MPP. If you see any documentation
 *   telling you that MPP is read-only on this SoC, don't believe its
 *   lies.
 * - Enable interrupts when exiting from exception into a new thread
 *   by setting MPIE now, so it will be copied into IE on mret.
 *
 * In S-mode (CONFIG_RISCV_S_MODE), use the sstatus equivalents:
 * - SPP=1 so that sret returns to S-mode (not U-mode)
 * - SPIE=1 so that interrupts are enabled after sret
 */
#ifdef CONFIG_RISCV_S_MODE
/* When MMU is enabled, set SUM so S-mode can access user-mapped pages */
#if defined(CONFIG_RISCV_MMU)
#define MSTATUS_DEF_RESTORE (SSTATUS_SPP | SSTATUS_SPIE | SSTATUS_SUM)
#else
#define MSTATUS_DEF_RESTORE (SSTATUS_SPP | SSTATUS_SPIE)
#endif
#else
#define MSTATUS_DEF_RESTORE (MSTATUS_MPP_M | MSTATUS_MPIE_EN)
#endif

/* Previous-privilege field and its user-mode value, abstracted across M/S mode.
 * Use as: (esf->mstatus & RV_STATUS_PP) == RV_STATUS_PP_U
 */
#ifdef CONFIG_RISCV_S_MODE
/** @brief Previous-privilege field mask in the status CSR (S-mode: SPP) */
#define RV_STATUS_PP    SSTATUS_SPP
/** @brief Previous-privilege field value representing User mode (S-mode) */
#define RV_STATUS_PP_U  0
#else
/** @brief Previous-privilege field mask in the status CSR (M-mode: MPP) */
#define RV_STATUS_PP    MSTATUS_MPP
/** @brief Previous-privilege field value representing User mode (M-mode) */
#define RV_STATUS_PP_U  PRV_U
#endif

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE
#define ARCH_IRQ_VECTOR_JUMP_CODE(v) "j " STRINGIFY(v)
#endif

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
#ifdef CONFIG_RISCV_MMU
	uint64_t *ptables; /* root page table for this domain */
	uint16_t asid; /* ASID assigned to this domain */
	sys_snode_t node; /* domain list linkage for kernel mapping sync */
#else
	unsigned int pmp_update_nr;
#endif
};

extern void z_irq_spurious(const void *unused);

/* Privilege-level abstraction for IRQ enable/disable CSR and bit */
#ifdef CONFIG_RISCV_S_MODE
/** @brief Name of the interrupt-status CSR as a string literal (S-mode) */
#define RISCV_STATUS_CSR "sstatus"
/** @brief Interrupt-enable bit in the status CSR (S-mode: SIE) */
#define RISCV_STATUS_IE  SSTATUS_SIE
#else
/** @brief Name of the interrupt-status CSR as a string literal (M-mode) */
#define RISCV_STATUS_CSR "mstatus"
/** @brief Interrupt-enable bit in the status CSR (M-mode: MIE) */
#define RISCV_STATUS_IE  MSTATUS_IEN
#endif

/*
 * use atomic instruction csrrc to lock global irq
 * csrrc: atomic read and clear bits in CSR register
 */
static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
#ifdef CONFIG_RISCV_SOC_HAS_CUSTOM_IRQ_LOCK_OPS
	return z_soc_irq_lock();
#else
	unsigned int key;

	__asm__ volatile ("csrrc %0, " RISCV_STATUS_CSR ", %1"
			  : "=r" (key)
			  : "rK" (RISCV_STATUS_IE)
			  : "memory");

	return key;
#endif
}

/*
 * use atomic instruction csrs to unlock global irq
 * csrs: atomic set bits in CSR register
 */
static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
#ifdef CONFIG_RISCV_SOC_HAS_CUSTOM_IRQ_LOCK_OPS
	z_soc_irq_unlock(key);
#else
	__asm__ volatile ("csrs " RISCV_STATUS_CSR ", %0"
			  :
			  : "r" (key & RISCV_STATUS_IE)
			  : "memory");
#endif
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
#ifdef CONFIG_RISCV_SOC_HAS_CUSTOM_IRQ_LOCK_OPS
	return z_soc_irq_unlocked(key);
#else
	return (key & RISCV_STATUS_IE) != 0;
#endif
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

extern uint64_t sys_clock_cycle_get_64(void);

static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

#include <zephyr/arch/riscv/error.h>

#ifdef __cplusplus
}
#endif

#endif /*_ASMLANGUAGE */

#if defined(CONFIG_RISCV_PRIVILEGED)
#include <zephyr/arch/riscv/riscv-privileged/asm_inline.h>
#endif


#endif
