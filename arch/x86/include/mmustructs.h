/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MMUSTRUCTS_H
#define _MMUSTRUCTS_H

#define MMU_PAGE_SIZE 4096
#define MMU_PAGE_MASK 0xfff
#define MMU_PAGE_SHIFT 12
#define PAGES(x) ((x) << (MMU_PAGE_SHIFT))
#define MMU_ARE_IN_SAME_PAGE(a, b) \
	(((u32_t)(a) & ~MMU_PAGE_MASK) == ((u32_t)(b) & ~MMU_PAGE_MASK))
#define MMU_IS_ON_PAGE_BOUNDARY(a) (!((u32_t)(a) & MMU_PAGE_MASK))

/*
 * The following bitmasks correspond to the bit-fields in the
 * x86_mmu_pde_pt structure.
 */

#define MMU_PDE_P_MASK          0x00000001
#define MMU_PDE_RW_MASK         0x00000002
#define MMU_PDE_US_MASK         0x00000004
#define MMU_PDE_PWT_MASK        0x00000008
#define MMU_PDE_PCD_MASK        0x00000010
#define MMU_PDE_A_MASK          0x00000020
#define MMU_PDE_PS_MASK         0x00000080
#define MMU_PDE_IGNORED_MASK    0x00000F40
#define MMU_PDE_PAGE_TABLE_MASK 0xfffff000

/*
 * The following bitmasks correspond to the bit-fields in the
 * x86_mmu_pde_4mb structure.
 */

#define MMU_4MB_PDE_P_MASK          0x00000001
#define MMU_4MB_PDE_RW_MASK         0x00000002
#define MMU_4MB_PDE_US_MASK         0x00000004
#define MMU_4MB_PDE_PWT_MASK        0x00000008
#define MMU_4MB_PDE_PCD_MASK        0x00000010
#define MMU_4MB_PDE_A_MASK          0x00000020
#define MMU_4MB_PDE_D_MASK          0x00000040
#define MMU_4MB_PDE_PS_MASK         0x00000080
#define MMU_4MB_PDE_G_MASK          0x00000100
#define MMU_4MB_PDE_IGNORED_MASK    0x00380e00
#define MMU_4MB_PDE_PAT_MASK        0x00001000
#define MMU_4MB_PDE_PAGE_TABLE_MASK 0x0007e000
#define MMU_4MB_PDE_PAGE_MASK       0xffc00000

#define MMU_4MB_PDE_CLEAR_PS        0x00000000
#define MMU_4MB_PDE_SET_PS          0x00000080

#define MMU_PDE_NUM_SHIFT           22
#define MMU_PDE_NUM(v)              ((u32_t)(v) >> MMU_PDE_NUM_SHIFT)
#define MMU_PGT_NUM(v)              MMU_PDE_NUM(v)
#define MMU_P4M_NUM(v)              MMU_PDE_NUM(v)
#define MMU_ENTRIES_PER_PGT         1024

/*
 * The following bitmasks correspond to the bit-fields in the
 * x86_mmu_pte structure.
 */

#define MMU_PTE_P_MASK              0x00000001
#define MMU_PTE_RW_MASK             0x00000002
#define MMU_PTE_US_MASK             0x00000004
#define MMU_PTE_PWT_MASK            0x00000008
#define MMU_PTE_PCD_MASK            0x00000010
#define MMU_PTE_A_MASK              0x00000020
#define MMU_PTE_D_MASK              0x00000040
#define MMU_PTE_PAT_MASK            0x00000080
#define MMU_PTE_G_MASK              0x00000100
#define MMU_PTE_ALLOC_MASK          0x00000200
#define MMU_PTE_CUSTOM_MASK         0x00000c00
#define MMU_PTE_PAGE_MASK           0xfffff000
#define MMU_PTE_MASK_ALL            0xffffffff

#define MMU_PAGE_NUM_SHIFT 12
#define MMU_PAGE_NUM(v) (((u32_t)(v) >> MMU_PAGE_NUM_SHIFT) & 0x3ff)

/*
 * The following values are to are to be OR'ed together to mark the use or
 * unuse of various options in a PTE or PDE as appropriate.
 */

#define MMU_ENTRY_NOT_PRESENT       0x00000000
#define MMU_ENTRY_PRESENT           0x00000001

#define MMU_ENTRY_READ              0x00000000
#define MMU_ENTRY_WRITE             0x00000002

#define MMU_ENTRY_SUPERVISOR        0x00000000
#define MMU_ENTRY_USER              0x00000004

#define MMU_ENTRY_WRITE_BACK        0x00000000
#define MMU_ENTRY_WRITE_THROUGH     0x00000008

#define MMU_ENTRY_CACHING_ENABLE    0x00000000
#define MMU_ENTRY_CACHING_DISABLE   0x00000010

#define MMU_ENTRY_NOT_ACCESSED      0x00000000
#define MMU_ENTRY_ACCESSED          0x00000020

#define MMU_ENTRY_NOT_DIRTY         0x00000000
#define MMU_ENTRY_DIRTY             0x00000040

#define MMU_ENTRY_NOT_GLOBAL        0x00000000
#define MMU_ENTRY_GLOBAL            0x00000100

#define MMU_ENTRY_NOT_ALLOC         0x00000000
#define MMU_ENTRY_ALLOC             0x00000200

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/* Structure used by gen_mmu.py to create page directories and page tables.
 * In order to populate this structure use macro MMU_BOOT_REGION.
 */
struct mmu_region {
	u32_t address; /*Start address of the memory region */
	u32_t size; /* Size of the memory region*/
	u32_t flags; /* Permissions needed for this region*/
};

/* permission_flags are calculated using the macros
 * region_size has to be provided in bytes
 * for read write access = MMU_ENTRY_READ/MMU_ENTRY_WRITE
 * for supervisor/user mode access = MMU_ENTRY_SUPERVISOR/MMU_ENTRY_USER
 */

#define MMU_BOOT_REGION(addr, region_size, permission_flags)		\
	static struct mmu_region region_##addr				\
	__attribute__((__section__(".mmulist"), used))  =		\
	{								\
		.address = addr,					\
		.size = region_size,					\
		.flags = permission_flags,				\
	}

/*
 * The following defines the format of a 32-bit page directory entry
 * that references a page table (as opposed to a 4 Mb page).
 */
union x86_mmu_pde_pt {
	 /** access PT entry through use of bitmasks */
	u32_t  value;
	struct {
		/** present: must be 1 to reference a page table */
		u32_t p:1;

		/** read/write: if 0, writes may not be allowed to the region
		 * controlled by this entry
		 */
		u32_t rw:1;

		/** user/supervisor: if 0, accesses with CPL=3 are not allowed
		 * to the region controlled by this entry
		 */
		u32_t us:1;

		/** page-level write-through: determines the memory type used
		 * to access the page table referenced by this entry
		 */
		u32_t pwt:1;

		/** page-level cache disable: determines the memory
		 * type used to access the page table referenced by
		 * this entry
		 */
		u32_t pcd:1;

		/** accessed: if 1 -> entry has been used to translate
		 */
		u32_t a:1;

		u32_t ignored1:1;

		/** page size: ignored when CR4.PSE=0 */
		u32_t ps:1;

		u32_t ignored2:4;

		/** page table: physical address of page table */
		u32_t page_table:20;
	};
};


/*
 * The following defines the format of a 32-bit page directory entry
 * that references a 4 Mb page (as opposed to a page table).
 */

union x86_mmu_pde_4mb {
	u32_t  value;
	struct {
		/** present: must be 1 to map a 4 Mb page */
		u32_t p:1;

		/** read/write: if 0, writes may not be allowed to the 4 Mb
		 * page referenced by this entry
		 */
		u32_t rw:1;

		/** user/supervisor: if 0, accesses with CPL=3 are not allowed
		 * to the 4 Mb page referenced by this entry
		 */
		u32_t us:1;

		/** page-level write-through: determines the memory type used
		 * to access the 4 Mb page referenced by
		 * this entry
		 */
		u32_t pwt:1;

		/** page-level cache disable: determines the memory type used
		 * to access the 4 Mb page referenced by this entry
		 */
		u32_t pcd:1;

		/** accessed: if 1 -> entry has been used to translate */
		u32_t a:1;

		/** dirty: indicates whether software has written to the 4 Mb
		 * page referenced by this entry
		 */
		u32_t d:1;

		/** page size: must be 1 otherwise this entry references a page
		 * table entry
		 */
		u32_t ps:1;

		/** global: if CR4.PGE=1, then determines whether this
		 * translation is global, i.e. used regardless of PCID
		 */
		u32_t g:1;

		u32_t ignored1:3;

		/** If PAT is supported, indirectly determines the memory type
		 * used to access the 4 Mb page, otherwise must be 0
		 */
		u32_t pat:1;

		/** page table: physical address of page table */
		u32_t page_table:6;

		u32_t ignored2:3;

		/** page: physical address of the 4 Mb page */
		u32_t page:10;
	};
};

/*
 * The following defines the format of a 32-bit page table entry that maps
 * a 4 Kb page.
 */
union x86_mmu_pte {
	u32_t  value;

	struct {
		/** present: must be 1 to map a 4 Kb page */
		u32_t p:1;

		/** read/write: if 0, writes may not be allowed to the 4 Kb
		 * page controlled by this entry
		 */
		u32_t rw:1;

		/** user/supervisor: if 0, accesses with CPL=3 are not allowed
		 * to the 4 Kb page controlled by this entry
		 */
		u32_t us:1;

		/** page-level write-through: determines the memory type used
		 * to access the 4 Kb page referenced by this entry
		 */
		u32_t pwt:1;

		/** page-level cache disable: determines the memory type used
		 * to access the 4 Kb page referenced by this entry
		 */
		u32_t pcd:1;

		/** accessed: if 1 -> 4 Kb page has been referenced */
		u32_t a:1;

		/** dirty: if 1 -> 4 Kb page has been written to */
		u32_t d:1;

		/** If PAT is supported, indirectly determines the memory type
		 * used to access the 4 Kb page, otherwise must be 0
		 */
		u32_t pat:1;

		/** global: if CR4.PGE=1, then determines whether this
		 * translation is global, i.e. used regardless of PCID
		 */
		u32_t g:1;

		/** allocated: if 1 -> this PTE has been allocated/ reserved;
		 * this is only used by software, i.e. this bit is ignored by
		 * the MMU
		 */
		u32_t alloc:1;

		/** Ignored by h/w, available for use by s/w */
		u32_t custom:2;

		/** page: physical address of the 4 Kb page */
		u32_t page:20;
	};
};


union x86_mmu_pde {
	union x86_mmu_pde_pt pt;
	union x86_mmu_pde_4mb fourmb;
};

/** Page Directory structure for 32-bit paging mode */
struct x86_mmu_page_directory {
	union x86_mmu_pde entry[1024];
};

/** Page Table structure for 32-bit paging mode */
struct x86_mmu_page_table {
	union x86_mmu_pte entry[1024];
};

#endif /* _ASMLANGUAGE */

#endif /* _MMUSTRUCTS_H */
