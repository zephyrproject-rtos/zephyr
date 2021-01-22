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
#define MMU_PS		BITL(7)		/** Page Size (non PTE)*/
#define MMU_PAT		BITL(7)		/** Page Attribute (PTE) */
#define MMU_G		BITL(8)		/** Global */
#ifdef XD_SUPPORTED
#define MMU_XD		BITL(63)	/** Execute Disable */
#else
#define MMU_XD		0
#endif

/* Unused PTE bits ignored by the CPU, which we use for our own OS purposes.
 * These bits ignored for all paging modes.
 */
#define MMU_IGNORED0	BITL(9)
#define MMU_IGNORED1	BITL(10)
#define MMU_IGNORED2	BITL(11)

/* Page fault error code flags. See Chapter 4.7 of the Intel SDM vol. 3A. */
#define PF_P		BIT(0)	/* 0 Non-present page  1 Protection violation */
#define PF_WR		BIT(1)  /* 0 Read              1 Write */
#define PF_US		BIT(2)  /* 0 Supervisor mode   1 User mode */
#define PF_RSVD		BIT(3)  /* 1 reserved bit set */
#define PF_ID		BIT(4)  /* 1 instruction fetch */
#define PF_PK		BIT(5)  /* 1 protection-key violation */
#define PF_SGX		BIT(15) /* 1 SGX-specific access control requirements */

/*
 * NOTE: All page table links are by physical, not virtual address.
 * For now, we have a hard requirement that the memory addresses of paging
 * structures must be convertible with a simple mathematical operation,
 * by applying the difference in the base kernel virtual and physical
 * addresses.
 *
 * Arbitrary mappings would induce a chicken-and-the-egg problem when walking
 * page tables. The codebase does not yet use techniques like recursive page
 * table mapping to alleviate this. It's simplest to just ensure the page
 * pool's pages can always be converted with simple math and a cast.
 *
 * The following conversion functions and macros are exclusively for use when
 * walking and creating page tables.
 */
#ifdef CONFIG_MMU
#define Z_X86_VIRT_OFFSET  (CONFIG_KERNEL_VM_BASE - CONFIG_SRAM_BASE_ADDRESS)
#else
#define Z_X86_VIRT_OFFSET	0
#endif

/* ASM code */
#define Z_X86_PHYS_ADDR(virt)	((virt) - Z_X86_VIRT_OFFSET)

#ifndef _ASMLANGUAGE
/* Installing new paging structures */
static inline uintptr_t z_x86_phys_addr(void *virt)
{
	return ((uintptr_t)virt - Z_X86_VIRT_OFFSET);
}

/* Examining page table links */
static inline void *z_x86_virt_addr(uintptr_t phys)
{
	return (void *)(phys + Z_X86_VIRT_OFFSET);
}

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
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_X86_PAE
#define PTABLES_ALIGN	0x1fU
#else
#define PTABLES_ALIGN	0xfffU
#endif

/* Set CR3 to a physical address. There must be a valid top-level paging
 * structure here or the CPU will triple fault. The incoming page tables must
 * have the same kernel mappings wrt supervisor mode. Don't use this function
 * unless you know exactly what you are doing.
 */
static inline void z_x86_cr3_set(uintptr_t phys)
{
	__ASSERT((phys & PTABLES_ALIGN) == 0U, "unaligned page tables");
#ifdef CONFIG_X86_64
	__asm__ volatile("movq %0, %%cr3\n\t" : : "r" (phys) : "memory");
#else
	__asm__ volatile("movl %0, %%cr3\n\t" : : "r" (phys) : "memory");
#endif
}

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
	return z_x86_virt_addr(z_x86_cr3_get());
}

/* Return cr2 value, which contains the page fault linear address.
 * See Section 6.15 of the IA32 Software Developer's Manual vol 3.
 * Used by page fault handling code.
 */
static inline void *z_x86_cr2_get(void)
{
	void *cr2;
#ifdef CONFIG_X86_64
	__asm__ volatile("movq %%cr2, %0\n\t" : "=r" (cr2));
#else
	__asm__ volatile("movl %%cr2, %0\n\t" : "=r" (cr2));
#endif
	return cr2;
}

/* Kernel's page table. This is in CR3 for all supervisor threads.
 * if KPTI is enabled, we switch to this when handling exceptions or syscalls
 */
extern pentry_t z_x86_kernel_ptables[];

/* Get the page tables used by this thread during normal execution */
static inline pentry_t *z_x86_thread_page_tables_get(struct k_thread *thread)
{
#if defined(CONFIG_USERSPACE) && !defined(CONFIG_X86_COMMON_PAGE_TABLE)
	if (!IS_ENABLED(CONFIG_X86_KPTI) ||
	    (thread->base.user_options & K_USER) != 0U) {
		/* If KPTI is enabled, supervisor threads always use
		 * the kernel's page tables and not the page tables associated
		 * with their memory domain.
		 */
		return z_x86_virt_addr(thread->arch.ptables);
	}
#endif
	return z_x86_kernel_ptables;
}

#ifdef CONFIG_SMP
/* Handling function for TLB shootdown inter-processor interrupts. */
void z_x86_tlb_ipi(const void *arg);
#endif

#ifdef CONFIG_X86_COMMON_PAGE_TABLE
void z_x86_swap_update_common_page_table(struct k_thread *incoming);
#endif
#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_ARCH_X86_INCLUDE_X86_MMU_H */
