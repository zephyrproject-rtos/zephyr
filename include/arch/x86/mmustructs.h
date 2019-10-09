/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_MMUSTRUCTS_H_
#define ZEPHYR_INCLUDE_ARCH_X86_MMUSTRUCTS_H_

#include <sys/util.h>

#define MMU_PAGE_SIZE 4096U
#define MMU_PAGE_MASK 0xfffU
#define MMU_PAGE_SHIFT 12U
#define PAGES(x) ((x) << (MMU_PAGE_SHIFT))
#define MMU_ARE_IN_SAME_PAGE(a, b) \
	(((u32_t)(a) & ~MMU_PAGE_MASK) == ((u32_t)(b) & ~MMU_PAGE_MASK))
#define MMU_IS_ON_PAGE_BOUNDARY(a) (!((u32_t)(a) & MMU_PAGE_MASK))

/*
 * Common flags in the same bit position regardless of which structure level,
 * although not every flag is supported at every level, and some may be
 * ignored depending on the state of other bits (such as P or PS)
 *
 * These flags indicate bit position, and can be used for setting flags or
 * masks as needed.
 */

#define Z_X86_MMU_P		BIT64(0)	/** Present */
#define Z_X86_MMU_RW		BIT64(1)	/** Read-Write */
#define Z_X86_MMU_US		BIT64(2)	/** User-Supervisor */
#define Z_X86_MMU_PWT		BIT64(3)	/** Page Write Through */
#define Z_X86_MMU_PCD		BIT64(4)	/** Page Cache Disable */
#define Z_X86_MMU_A		BIT64(5)	/** Accessed */
#define Z_X86_MMU_D		BIT64(6)	/** Dirty */
#define Z_X86_MMU_PS		BIT64(7)	/** Page Size */
#define Z_X86_MMU_G		BIT64(8)	/** Global */
#define Z_X86_MMU_XD		BIT64(63)	/** Execute Disable */

/*
 * Structure-specific flags / masks
 */

#define Z_X86_MMU_PDE_PAT	BIT64(12)
#define Z_X86_MMU_PTE_PAT	BIT64(7)	/** Page Attribute Table */

#define Z_X86_MMU_PDPTE_PD_MASK		0x00000000FFFFF000ULL
#define Z_X86_MMU_PDE_PT_MASK		0x00000000FFFFF000ULL
#define Z_X86_MMU_PDE_2MB_MASK		0x00000000FFC00000ULL
#define Z_X86_MMU_PTE_ADDR_MASK		0x00000000FFFFF000ULL

/*
 * These flags indicate intention when setting access properties.
 */

#define MMU_ENTRY_NOT_PRESENT       0ULL
#define MMU_ENTRY_PRESENT           Z_X86_MMU_P

#define MMU_ENTRY_READ              0ULL
#define MMU_ENTRY_WRITE             Z_X86_MMU_RW

#define MMU_ENTRY_SUPERVISOR        0ULL
#define MMU_ENTRY_USER              Z_X86_MMU_US

#define MMU_ENTRY_WRITE_BACK        0ULL
#define MMU_ENTRY_WRITE_THROUGH     Z_X86_MMU_PWT

#define MMU_ENTRY_CACHING_ENABLE    0ULL
#define MMU_ENTRY_CACHING_DISABLE   Z_X86_MMU_PCD

#define MMU_ENTRY_NOT_ACCESSED      0ULL
#define MMU_ENTRY_ACCESSED          Z_X86_MMU_A

#define MMU_ENTRY_NOT_DIRTY         0ULL
#define MMU_ENTRY_DIRTY             Z_X86_MMU_D

#define MMU_ENTRY_NOT_GLOBAL        0ULL
#define MMU_ENTRY_GLOBAL            Z_X86_MMU_G

#define MMU_ENTRY_EXECUTE_DISABLE   Z_X86_MMU_XD
#define MMU_ENTRY_EXECUTE_ENABLE    0ULL

/* memory partition arch/soc independent attribute */
#define K_MEM_PARTITION_P_RW_U_RW   (MMU_ENTRY_WRITE | \
				     MMU_ENTRY_USER  | \
				     MMU_ENTRY_EXECUTE_DISABLE)

#define K_MEM_PARTITION_P_RW_U_NA   (MMU_ENTRY_WRITE | \
				     MMU_ENTRY_SUPERVISOR | \
				     MMU_ENTRY_EXECUTE_DISABLE)

#define K_MEM_PARTITION_P_RO_U_RO   (MMU_ENTRY_READ | \
				     MMU_ENTRY_USER | \
				     MMU_ENTRY_EXECUTE_DISABLE)

#define K_MEM_PARTITION_P_RO_U_NA   (MMU_ENTRY_READ  | \
				     MMU_ENTRY_SUPERVISOR | \
				     MMU_ENTRY_EXECUTE_DISABLE)

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX (MMU_ENTRY_WRITE | MMU_ENTRY_USER)

#define K_MEM_PARTITION_P_RWX_U_NA  (MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR)

#define K_MEM_PARTITION_P_RX_U_RX   (MMU_ENTRY_READ | MMU_ENTRY_USER)

#define K_MEM_PARTITION_P_RX_U_NA   (MMU_ENTRY_READ | MMU_ENTRY_SUPERVISOR)


 /* memory partition access permission mask */
#define K_MEM_PARTITION_PERM_MASK   (Z_X86_MMU_RW | Z_X86_MMU_US | \
				     Z_X86_MMU_XD)

#ifndef _ASMLANGUAGE
#include <sys/__assert.h>
#include <zephyr/types.h>

/* Structure used by gen_mmu.py to create page directories and page tables.
 * In order to populate this structure use macro MMU_BOOT_REGION.
 */
struct mmu_region {
	uintptr_t address; /*Start address of the memory region */
	size_t size; /* Size of the memory region*/
	u64_t flags; /* Permissions needed for this region*/
};

/* permission_flags are calculated using the macros
 * region_size has to be provided in bytes
 * for read write access = MMU_ENTRY_READ/MMU_ENTRY_WRITE
 * for supervisor/user mode access = MMU_ENTRY_SUPERVISOR/MMU_ENTRY_USER
 *
 * Preprocessor indirection layers used to ensure __COUNTER__ is expanded
 * properly.
 */

#define __MMU_BOOT_REGION(id, addr, region_size, permission_flags)	\
	static struct mmu_region region_##id				\
	__attribute__((__section__(".mmulist"), used))  =		\
	{								\
		.address = addr,					\
		.size = region_size,					\
		.flags = permission_flags,				\
	}

#define Z_MMU_BOOT_REGION(id, addr, region_size, permission_flags)	\
	__MMU_BOOT_REGION(id, addr, region_size, permission_flags)

#define MMU_BOOT_REGION(addr, region_size, permission_flags)		\
	Z_MMU_BOOT_REGION(__COUNTER__, addr, region_size, permission_flags)

#define Z_X86_NUM_PDPT_ENTRIES	4
#define Z_X86_NUM_PD_ENTRIES	512
#define Z_X86_NUM_PT_ENTRIES	512

/* Memory range covered by an instance of various table types */
#define Z_X86_PT_AREA	(MMU_PAGE_SIZE * Z_X86_NUM_PT_ENTRIES)
#define Z_X86_PD_AREA	(Z_X86_PT_AREA * Z_X86_NUM_PD_ENTRIES)
#define Z_X86_PDPT_AREA (Z_X86_PD_AREA * Z_X86_NUM_PDPT_ENTRIES)

typedef u64_t k_mem_partition_attr_t;

struct x86_mmu_pdpt {
	u64_t entry[Z_X86_NUM_PDPT_ENTRIES];
};

struct x86_mmu_pd {
	u64_t entry[Z_X86_NUM_PD_ENTRIES];
};

struct x86_mmu_pt {
	u64_t entry[Z_X86_NUM_PT_ENTRIES];
};

struct x86_page_tables {
	struct x86_mmu_pdpt pdpt;
};

/*
 * Inline functions for getting the next linked structure
 */

static inline u64_t *z_x86_pdpt_get_pdpte(struct x86_mmu_pdpt *pdpt,
					  uintptr_t addr)
{
	int index = (addr >> 30U) & (Z_X86_NUM_PDPT_ENTRIES - 1);

	return &pdpt->entry[index];
}

static inline struct x86_mmu_pd *z_x86_pdpte_get_pd(u64_t pdpte)
{
	uintptr_t addr = pdpte & Z_X86_MMU_PDPTE_PD_MASK;

	return (struct x86_mmu_pd *)addr;
}

static inline u64_t *z_x86_pd_get_pde(struct x86_mmu_pd *pd, uintptr_t addr)
{
	int index = (addr >> 21U) & (Z_X86_NUM_PD_ENTRIES - 1);

	return &pd->entry[index];
}

static inline struct x86_mmu_pt *z_x86_pde_get_pt(u64_t pde)
{
	uintptr_t addr = pde & Z_X86_MMU_PDE_PT_MASK;

	__ASSERT((pde & Z_X86_MMU_PS) == 0, "pde is for 2MB page");

	return (struct x86_mmu_pt *)addr;
}

static inline u64_t *z_x86_pt_get_pte(struct x86_mmu_pt *pt, uintptr_t addr)
{
	int index = (addr >> 12U) & (Z_X86_NUM_PT_ENTRIES - 1);

	return &pt->entry[index];
}

/*
 * Inline functions for obtaining page table structures from the top-level
 */

static inline struct x86_mmu_pdpt *
z_x86_get_pdpt(struct x86_page_tables *ptables, uintptr_t addr)
{
	ARG_UNUSED(addr);

	return &ptables->pdpt;
}

static inline u64_t *z_x86_get_pdpte(struct x86_page_tables *ptables,
				       uintptr_t addr)
{
	return z_x86_pdpt_get_pdpte(z_x86_get_pdpt(ptables, addr), addr);
}

static inline struct x86_mmu_pd *
z_x86_get_pd(struct x86_page_tables *ptables, uintptr_t addr)
{
	return z_x86_pdpte_get_pd(*z_x86_get_pdpte(ptables, addr));
}

static inline u64_t *z_x86_get_pde(struct x86_page_tables *ptables,
				     uintptr_t addr)
{
	return z_x86_pd_get_pde(z_x86_get_pd(ptables, addr), addr);
}

static inline struct x86_mmu_pt *
z_x86_get_pt(struct x86_page_tables *ptables, uintptr_t addr)
{
	return z_x86_pde_get_pt(*z_x86_get_pde(ptables, addr));
}

static inline u64_t *z_x86_get_pte(struct x86_page_tables *ptables,
				     uintptr_t addr)
{
	return z_x86_pt_get_pte(z_x86_get_pt(ptables, addr), addr);
}

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
 * @param ptables Top-level pointer to the page tables, as programmed in CR3
 */
void z_x86_dump_page_tables(struct x86_page_tables *ptables);

static inline struct x86_page_tables *z_x86_page_tables_get(void)
{
	struct x86_page_tables *ret;

	__asm__ volatile("movl %%cr3, %0\n\t" : "=r" (ret));

	return ret;
}

/* Kernel's page table. Always active when threads are running in supervisor
 * mode, or handling an interrupt.
 *
 * If KPTI is not enabled, this is used as a template to create per-thread
 * page tables for when threads run in user mode.
 */
extern struct x86_page_tables z_x86_kernel_ptables;
#ifdef CONFIG_X86_KPTI
/* Separate page tables for user mode threads. This is never installed into the
 * CPU; instead it is used as a template for creating per-thread page tables.
 */
extern struct x86_page_tables z_x86_user_ptables;
#define USER_PTABLES	z_x86_user_ptables
#else
#define USER_PTABLES	z_x86_kernel_ptables
#endif
/**
 * @brief Fetch page table flags for a particular page
 *
 * Given a memory address, return the flags for the containing page's
 * PDE and PTE entries. Intended for debugging.
 *
 * @param ptables Which set of page tables to use
 * @param addr Memory address to example
 * @param pde_flags Output parameter for page directory entry flags
 * @param pte_flags Output parameter for page table entry flags
 */
void z_x86_mmu_get_flags(struct x86_page_tables *ptables, void *addr,
			 u64_t *pde_flags, u64_t *pte_flags);

/**
 * @brief set flags in the MMU page tables
 *
 * Modify bits in the existing page tables for a particular memory
 * range, which must be page-aligned
 *
 * @param ptables Which set of page tables to use
 * @param ptr Starting memory address which must be page-aligned
 * @param size Size of the region, must be page size multiple
 * @param flags Value of bits to set in the page table entries
 * @param mask Mask indicating which particular bits in the page table entries
 *             to modify
 * @param flush Whether to flush the TLB for the modified pages, only needed
 *              when modifying the active page tables
 */
void z_x86_mmu_set_flags(struct x86_page_tables *ptables, void *ptr,
			 size_t size, u64_t flags, u64_t mask, bool flush);

int z_x86_mmu_validate(struct x86_page_tables *ptables, void *addr, size_t size,
		       bool write);
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_MMUSTRUCTS_H_ */
