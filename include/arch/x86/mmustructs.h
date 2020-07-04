/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_MMU_H
#define ZEPHYR_INCLUDE_ARCH_X86_MMU_H

#include <sys/util.h>

/* Macros for reserving space for page tables
 *
 * Z_X86_NUM_TABLE_PAGES. In order to produce a set of page tables which has
 * virtual mappings for all system RAM, Z_X86_NUM_TABLE_PAGES is the number of
 * memory pages required. If CONFIG_X86_PAE is enabled, an additional 0x20
 * bytes are required for the toplevel 4-entry PDPT.
 *
 * Z_X86_INITIAL_PAGETABLE_SIZE is the total amount of memory in bytes
 * required, for any paging mode.
 *
 * These macros are currently used for two purposes:
 * - Reserving memory in the stack for thread-level page tables (slated
 *   for eventual removal when USERSPACE is reworked to fully utilize
 *   virtual memory and page tables are maintained at the process level)
 * - Reserving room for dummy pagetable memory for the first link, so that
 *   memory addresses are not disturbed by the insertion of the real page
 *   tables created by gen_mmu.py in the second link phase.
 */
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
#ifdef CONFIG_X86_64
#define Z_X86_NUM_PML4_ENTRIES 512U
#define Z_X86_NUM_PDPT_ENTRIES 512U
#else
#define Z_X86_NUM_PDPT_ENTRIES 4U
#endif /* CONFIG_X86_64 */
#define Z_X86_NUM_PD_ENTRIES   512U
#define Z_X86_NUM_PT_ENTRIES   512U
#else
#define Z_X86_NUM_PD_ENTRIES   1024U
#define Z_X86_NUM_PT_ENTRIES   1024U
#endif /* !CONFIG_X86_64 && !CONFIG_X86_PAE */
/* Memory range covered by an instance of various table types */
#define Z_X86_PT_AREA  ((uintptr_t)(CONFIG_MMU_PAGE_SIZE * \
				    Z_X86_NUM_PT_ENTRIES))
#define Z_X86_PD_AREA  (Z_X86_PT_AREA * Z_X86_NUM_PD_ENTRIES)
#ifdef CONFIG_X86_64
#define Z_X86_PDPT_AREA (Z_X86_PD_AREA * Z_X86_NUM_PDPT_ENTRIES)
#endif

#define PHYS_RAM_ADDR DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define PHYS_RAM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

/* Define a range [Z_X86_PT_START, Z_X86_PT_END) which is the memory range
 * covered by all the page tables needed for system RAM
 */
#define Z_X86_PT_START	((uintptr_t)ROUND_DOWN(PHYS_RAM_ADDR, Z_X86_PT_AREA))
#define Z_X86_PT_END	((uintptr_t)ROUND_UP(PHYS_RAM_ADDR + PHYS_RAM_SIZE, \
					     Z_X86_PT_AREA))

/* Number of page tables needed to cover system RAM. Depends on the specific
 * bounds of system RAM, but roughly 1 page table per 2MB of RAM
 */
#define Z_X86_NUM_PT	((Z_X86_PT_END - Z_X86_PT_START) / Z_X86_PT_AREA)

#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
/* Same semantics as above, but for the page directories needed to cover
 * system RAM.
 */
#define Z_X86_PD_START	((uintptr_t)ROUND_DOWN(PHYS_RAM_ADDR, Z_X86_PD_AREA))
#define Z_X86_PD_END	((uintptr_t)ROUND_UP(PHYS_RAM_ADDR + PHYS_RAM_SIZE, \
					     Z_X86_PD_AREA))
/* Number of page directories needed to cover system RAM. Depends on the
 * specific bounds of system RAM, but roughly 1 page directory per 1GB of RAM
 */
#define Z_X86_NUM_PD	((Z_X86_PD_END - Z_X86_PD_START) / Z_X86_PD_AREA)
#else
/* 32-bit page tables just have one toplevel page directory */
#define Z_X86_NUM_PD	1
#endif

#ifdef CONFIG_X86_64
/* Same semantics as above, but for the page directory pointer tables needed
 * to cover system RAM. On 32-bit there is just one 4-entry PDPT.
 */
#define Z_X86_PDPT_START	((uintptr_t)ROUND_DOWN(PHYS_RAM_ADDR, \
						       Z_X86_PDPT_AREA))
#define Z_X86_PDPT_END	((uintptr_t)ROUND_UP(PHYS_RAM_ADDR + PHYS_RAM_SIZE, \
					     Z_X86_PDPT_AREA))
/* Number of PDPTs needed to cover system RAM. Depends on the
 * specific bounds of system RAM, but roughly 1 PDPT per 512GB of RAM
 */
#define Z_X86_NUM_PDPT	((Z_X86_PDPT_END - Z_X86_PDPT_START) / Z_X86_PDPT_AREA)

/* All pages needed for page tables, using computed values plus one more for
 * the top-level PML4
 */
#define Z_X86_NUM_TABLE_PAGES   (Z_X86_NUM_PT + Z_X86_NUM_PD + \
				 Z_X86_NUM_PDPT + 1)
#else /* !CONFIG_X86_64 */
/* Number of pages we need to reserve in the stack for per-thread page tables */
#define Z_X86_NUM_TABLE_PAGES	(Z_X86_NUM_PT + Z_X86_NUM_PD)
#endif /* CONFIG_X86_64 */

#ifdef CONFIG_X86_PAE
/* Toplevel PDPT wasn't included as it is not a page in size */
#define Z_X86_INITIAL_PAGETABLE_SIZE	((Z_X86_NUM_TABLE_PAGES * \
					  CONFIG_MMU_PAGE_SIZE) + 0x20)
#else
#define Z_X86_INITIAL_PAGETABLE_SIZE	(Z_X86_NUM_TABLE_PAGES * \
					 CONFIG_MMU_PAGE_SIZE)
#endif

/*
 * K_MEM_PARTITION_* defines
 *
 * Slated for removal when virtual memory is implemented, memory
 * mapping APIs will replace memory domains.
 */
#define Z_X86_MMU_RW		BIT64(1)	/** Read-Write */
#define Z_X86_MMU_US		BIT64(2)	/** User-Supervisor */
#if defined(CONFIG_X86_PAE) || defined(CONFIG_X86_64)
#define Z_X86_MMU_XD		BIT64(63)	/** Execute Disable */
#else
#define Z_X86_MMU_XD		0
#endif

/* Always true with 32-bit page tables, don't enable
 * CONFIG_EXECUTE_XOR_WRITE and expect it to work for you
 */
#define K_MEM_PARTITION_IS_EXECUTABLE(attr)	(((attr) & Z_X86_MMU_XD) == 0)
#define K_MEM_PARTITION_IS_WRITABLE(attr)	(((attr) & Z_X86_MMU_RW) != 0)

/* memory partition arch/soc independent attribute */
#define K_MEM_PARTITION_P_RW_U_RW	(Z_X86_MMU_RW | Z_X86_MMU_US | \
					 Z_X86_MMU_XD)
#define K_MEM_PARTITION_P_RW_U_NA	(Z_X86_MMU_RW | Z_X86_MMU_XD)
#define K_MEM_PARTITION_P_RO_U_RO	(Z_X86_MMU_US | Z_X86_MMU_XD)
#define K_MEM_PARTITION_P_RO_U_NA	Z_X86_MMU_XD
/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX	(Z_X86_MMU_RW | Z_X86_MMU_US)
#define K_MEM_PARTITION_P_RWX_U_NA	Z_X96_MMU_RW
#define K_MEM_PARTITION_P_RX_U_RX	Z_X86_MMU_US
#define K_MEM_PARTITION_P_RX_U_NA	(0)
 /* memory partition access permission mask */
#define K_MEM_PARTITION_PERM_MASK	(Z_X86_MMU_RW | Z_X86_MMU_US | \
					 Z_X86_MMU_XD)

#ifndef _ASMLANGUAGE
/* Page table entry data type at all levels. Defined here due to
 * k_mem_partition_attr_t, eventually move to private x86_mmu.h
 */
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
typedef uint64_t pentry_t;
#else
typedef uint32_t pentry_t;
#endif
typedef pentry_t k_mem_partition_attr_t;
#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_X86_MMU_H */
