/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal memory management interfaces implemented in x86_mmu.c.
 * None of these are application-facing, use only if you know what you are
 * doing!
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_X86_MMU_H
#define ZEPHYR_ARCH_X86_INCLUDE_X86_MMU_H

#include <kernel.h>
#include <arch/x86/mmustructs.h>

#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
#define XD_SUPPORTED
#define BITL		BIT64
#define PRI_ENTRY	"0x%016llx"
#else
#define BITL		BIT
#define PRI_ENTRY	"0x%08x"
#endif

/*
 * Common flags in the same bit position regardless of which structure level,
 * although not every flag is supported at every level, and some may be
 * ignored depending on the state of other bits (such as P or PS)
 *
 * These flags indicate bit position, and can be used for setting flags or
 * masks as needed.
 */

#define MMU_P		BITL(0)		/** Present */
#define MMU_RW		BITL(1)		/** Read-Write */
#define MMU_US		BITL(2)		/** User-Supervisor */
#define MMU_PWT		BITL(3)		/** Page Write Through */
#define MMU_PCD		BITL(4)		/** Page Cache Disable */
#define MMU_A		BITL(5)		/** Accessed */
#define MMU_D		BITL(6)		/** Dirty */
#define MMU_PS		BITL(7)		/** Page Size */
#define MMU_G		BITL(8)		/** Global */
#ifdef XD_SUPPORTED
#define MMU_XD		BITL(63)	/** Execute Disable */
#else
#define MMU_XD		0
#endif

#ifdef CONFIG_EXCEPTION_DEBUG
/**
 * Dump out page table entries for a particular virtual memory address
 *
 * For the provided memory address, dump out interesting information about
 * its mapping to the error log
 *
 * @param ptables Page tables to walk
 * @param virt Virtual address to inspect
 */
void z_x86_dump_mmu_flags(pentry_t *ptables, void *virt);

/**
 * Fetch the page table entry for a virtual memory address
 *
 * @param paging_level [out] what paging level the entry was found at.
 *                     0=toplevel
 * @param val Value stored in page table entry, with address and flags
 * @param ptables Toplevel pointer to page tables
 * @param virt Virtual address to lookup
 */
void z_x86_pentry_get(int *paging_level, pentry_t *val, pentry_t *ptables,
		      void *virt);

/**
 * Debug function for dumping out page tables
 *
 * Iterates through the entire linked set of page table structures,
 * dumping out codes for the configuration of each table entry.
 *
 * Entry codes:
 *
 *   . - not present
 *   w - present, writable, not executable
 *   a - present, writable, executable
 *   r - present, read-only, not executable
 *   x - present, read-only, executable
 *
 * Entry codes in uppercase indicate that user mode may access.
 *
 * Color is used to indicate the physical mapping characteristics:
 *
 *   yellow - Identity mapping (virt = phys)
 *    green - Fixed virtual memory mapping (virt = phys + constant)
 *  magenta - entry is child page table
 *     cyan - General mapped memory
 *
 * @param ptables Top-level pointer to the page tables, as programmed in CR3
 */
void z_x86_dump_page_tables(pentry_t *ptables);
#endif /* CONFIG_EXCEPTION_DEBUG */

#ifdef CONFIG_HW_STACK_PROTECTION
/* Legacy function - set identity-mapped MMU stack guard page to RO in the
 * kernel's page tables to prevent writes and generate an exception
 */
void z_x86_set_stack_guard(k_thread_stack_t *stack);
#endif

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_X86_KPTI
/* Defined in linker script. Contains all the data that must be mapped
 * in a KPTI table even though US bit is not set (trampoline stack, GDT,
 * IDT, etc)
 */
extern uint8_t z_shared_kernel_page_start;
#endif /* CONFIG_X86_KPTI */

/* Set up per-thread page tables just prior to entering user mode */
void z_x86_thread_pt_init(struct k_thread *thread);

/* Apply a memory domain policy to a set of thread page tables */
void z_x86_apply_mem_domain(struct k_thread *thread,
			    struct k_mem_domain *mem_domain);
#endif /* CONFIG_USERSPACE */

/* Return cr3 value, which is the physical (not virtual) address of the
 * current set of page tables
 */
static inline uintptr_t z_x86_cr3_get(void)
{
	uintptr_t cr3;
#ifdef CONFIG_X86_64
	__asm__ volatile("movq %%cr3, %0\n\t" : "=r" (cr3));
#else
	__asm__ volatile("movl %%cr3, %0\n\t" : "=r" (cr3));
#endif
	return cr3;
}

/* Return the virtual address of the page tables installed in this CPU in CR3 */
static inline pentry_t *z_x86_page_tables_get(void)
{
	return (pentry_t *)z_x86_cr3_get();
}

/* Kernel's page table. This is in CR3 for all supervisor threads.
 * if KPTI is enabled, we switch to this when handling exceptions or syscalls
 */
extern pentry_t z_x86_kernel_ptables;

/* Get the page tables used by this thread during normal execution */
static inline pentry_t *z_x86_thread_page_tables_get(struct k_thread *thread)
{
#ifdef CONFIG_USERSPACE
	return (pentry_t *)(thread->arch.ptables);
#else
	return &z_x86_kernel_ptables;
#endif
}
#endif /* ZEPHYR_ARCH_X86_INCLUDE_X86_MMU_H */
