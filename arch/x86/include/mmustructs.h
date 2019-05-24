/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_MMUSTRUCTS_H_
#define ZEPHYR_ARCH_X86_INCLUDE_MMUSTRUCTS_H_

#define MMU_PAGE_SIZE 4096U
#define MMU_PAGE_MASK 0xfffU
#define MMU_PAGE_SHIFT 12U
#define PAGES(x) ((x) << (MMU_PAGE_SHIFT))
#define MMU_ARE_IN_SAME_PAGE(a, b) \
	(((u32_t)(a) & ~MMU_PAGE_MASK) == ((u32_t)(b) & ~MMU_PAGE_MASK))
#define MMU_IS_ON_PAGE_BOUNDARY(a) (!((u32_t)(a) & MMU_PAGE_MASK))

/*
 * The following bitmasks correspond to the bit-fields in the
 * x86_mmu_pde_pt structure.
 */

#define MMU_PDE_P_MASK          0x00000001ULL
#define MMU_PDE_RW_MASK         0x00000002ULL
#define MMU_PDE_US_MASK         0x00000004ULL
#define MMU_PDE_PWT_MASK        0x00000008ULL
#define MMU_PDE_PCD_MASK        0x00000010ULL
#define MMU_PDE_A_MASK          0x00000020ULL
#define MMU_PDE_PS_MASK         0x00000080ULL
#define MMU_PDE_IGNORED_MASK    0x00000F40ULL

#define MMU_PDE_XD_MASK            0x8000000000000000ULL
#define MMU_PDE_PAGE_TABLE_MASK    0x00000000fffff000ULL
#define MMU_PDE_NUM_SHIFT          21U
#define MMU_PDE_NUM(v)             (((u32_t)(v) >> MMU_PDE_NUM_SHIFT) & 0x1ffU)
#define MMU_ENTRIES_PER_PGT        512U
#define MMU_PDPTE_NUM_SHIFT        30U
#define MMU_PDPTE_NUM(v)           (((u32_t)(v) >> MMU_PDPTE_NUM_SHIFT) & 0x3U)

/*
 * The following bitmasks correspond to the bit-fields in the
 * x86_mmu_pde_2mb structure.
 */

#define MMU_2MB_PDE_P_MASK          0x00000001ULL
#define MMU_2MB_PDE_RW_MASK         0x00000002ULL
#define MMU_2MB_PDE_US_MASK         0x00000004ULL
#define MMU_2MB_PDE_PWT_MASK        0x00000008ULL
#define MMU_2MB_PDE_PCD_MASK        0x00000010ULL
#define MMU_2MB_PDE_A_MASK          0x00000020ULL
#define MMU_2MB_PDE_D_MASK          0x00000040ULL
#define MMU_2MB_PDE_PS_MASK         0x00000080ULL
#define MMU_2MB_PDE_G_MASK          0x00000100ULL
#define MMU_2MB_PDE_IGNORED_MASK    0x00380e00ULL
#define MMU_2MB_PDE_PAT_MASK        0x00001000ULL
#define MMU_2MB_PDE_PAGE_TABLE_MASK 0x0007e000ULL
#define MMU_2MB_PDE_PAGE_MASK       0xffc00000ULL
#define MMU_2MB_PDE_CLEAR_PS        0x00000000ULL
#define MMU_2MB_PDE_SET_PS          0x00000080ULL


/*
 * The following bitmasks correspond to the bit-fields in the
 * x86_mmu_pte structure.
 */

#define MMU_PTE_P_MASK            0x00000001ULL
#define MMU_PTE_RW_MASK           0x00000002ULL
#define MMU_PTE_US_MASK           0x00000004ULL
#define MMU_PTE_PWT_MASK          0x00000008ULL
#define MMU_PTE_PCD_MASK          0x00000010ULL
#define MMU_PTE_A_MASK            0x00000020ULL
#define MMU_PTE_D_MASK            0x00000040ULL
#define MMU_PTE_PAT_MASK          0x00000080ULL
#define MMU_PTE_G_MASK            0x00000100ULL
#define MMU_PTE_ALLOC_MASK        0x00000200ULL
#define MMU_PTE_CUSTOM_MASK       0x00000c00ULL
#define MMU_PTE_XD_MASK           0x8000000000000000ULL
#define MMU_PTE_PAGE_MASK         0x00000000fffff000ULL
#define MMU_PTE_MASK_ALL          0xffffffffffffffffULL
#define MMU_PAGE_NUM(v)           (((u32_t)(v) >> MMU_PAGE_NUM_SHIFT) & 0x1ffU)
#define MMU_PAGE_NUM_SHIFT 12

/*
 * The following values are to are to be OR'ed together to mark the use or
 * unuse of various options in a PTE or PDE as appropriate.
 */

#define MMU_ENTRY_NOT_PRESENT       0x00000000ULL
#define MMU_ENTRY_PRESENT           0x00000001ULL

#define MMU_ENTRY_READ              0x00000000ULL
#define MMU_ENTRY_WRITE             0x00000002ULL

#define MMU_ENTRY_SUPERVISOR        0x00000000ULL
#define MMU_ENTRY_USER              0x00000004ULL

#define MMU_ENTRY_WRITE_BACK        0x00000000ULL
#define MMU_ENTRY_WRITE_THROUGH     0x00000008ULL

#define MMU_ENTRY_CACHING_ENABLE    0x00000000ULL
#define MMU_ENTRY_CACHING_DISABLE   0x00000010ULL

#define MMU_ENTRY_NOT_ACCESSED      0x00000000ULL
#define MMU_ENTRY_ACCESSED          0x00000020ULL

#define MMU_ENTRY_NOT_DIRTY         0x00000000ULL
#define MMU_ENTRY_DIRTY             0x00000040ULL

#define MMU_ENTRY_NOT_GLOBAL        0x00000000ULL
#define MMU_ENTRY_GLOBAL            0x00000100ULL

#define MMU_ENTRY_NOT_ALLOC         0x00000000ULL
#define MMU_ENTRY_ALLOC             0x00000200ULL

#define MMU_ENTRY_EXECUTE_DISABLE   0x8000000000000000ULL

/* Special flag argument for MMU_BOOT region invocations */

/* Indicates that pages within this region may have their user/supervisor
 * permissions adjusted at runtime. Unnecessary if MMU_ENTRY_USER is already
 * set.
 *
 * The result of this is a guarantee that the 'user' bit for all PDEs referring
 * to the region will be set, even if the boot configuration has no user pages
 * in it.
 */
#define MMU_ENTRY_RUNTIME_USER      0x10000000ULL

/* Indicates that pages within this region may have their read/write
 * permissions adjusted at runtime. Unnecessary if MMU_ENTRY_WRITE is already
 * set.
 *
 * The result of this is a guarantee that the 'write' bit for all PDEs
 * referring to the region will be set, even if the boot configuration has no
 * writable pages in it.
 */
#define MMU_ENTRY_RUNTIME_WRITE	    0x20000000ULL


/* Helper macros to ease the usage of the MMU page table structures.
 */

/*
 * Returns the page table entry for the addr
 * use the union to extract page entry related information.
 */
#define X86_MMU_GET_PTE(pdpt, addr)\
	((union x86_mmu_pte *)\
	 (&X86_MMU_GET_PT_ADDR(pdpt, addr)->entry[MMU_PAGE_NUM(addr)]))

/*
 * Returns the Page table address for the particular address.
 * Page Table address(returned value) is always 4KBytes aligned.
 */
#define X86_MMU_GET_PT_ADDR(pdpt, addr) \
	((struct x86_mmu_pt *)\
	 (X86_MMU_GET_PDE(pdpt, addr)->pt << MMU_PAGE_SHIFT))

/* Returns the page directory entry for the addr
 * use the union to extract page directory entry related information.
 */
#define X86_MMU_GET_PDE(pdpt, addr)\
	((union x86_mmu_pde_pt *)					\
	 (&X86_MMU_GET_PD_ADDR(pdpt, addr)->entry[MMU_PDE_NUM(addr)]))

/* Returns the page directory entry for the addr
 * use the union to extract page directory entry related information.
 */
#define X86_MMU_GET_PD_ADDR(pdpt, addr) \
	((struct x86_mmu_pd *)		       \
	 (X86_MMU_GET_PDPTE(pdpt, addr)->pd << MMU_PAGE_SHIFT))

/* Returns the page directory pointer entry */
#define X86_MMU_GET_PDPTE(pdpt, addr) \
	(&((pdpt)->entry[MMU_PDPTE_NUM(addr)]))

/* Return the Page directory address.
 * input is the entry number
 */
#define X86_MMU_GET_PD_ADDR_INDEX(pdpt, index) \
	((struct x86_mmu_pd *)		       \
	 (X86_MMU_GET_PDPTE_INDEX(pdpt, index)->pd << MMU_PAGE_SHIFT))

/* Returns the page directory pointer entry.
 * Input is the entry number
 */
#define X86_MMU_GET_PDPTE_INDEX(pdpt, index) \
	(&((pdpt)->entry[index]))

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
#define K_MEM_PARTITION_PERM_MASK   (MMU_PTE_RW_MASK |\
				     MMU_PTE_US_MASK |\
				     MMU_PTE_XD_MASK)

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/* Structure used by gen_mmu.py to create page directories and page tables.
 * In order to populate this structure use macro MMU_BOOT_REGION.
 */
struct mmu_region {
	u32_t address; /*Start address of the memory region */
	u32_t size; /* Size of the memory region*/
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

/*
 * The following defines the format of a 64-bit page directory pointer entry
 * that references a page directory table
 */
union x86_mmu_pdpte {
	 /** access Page directory entry through use of bitmasks */
	u64_t  value;
	struct {
		/** present: must be 1 to reference a page table */
		u64_t p:1;

		u64_t reserved:2;

		/** page-level write-through: determines the memory type used
		 * to access the page table referenced by this entry
		 */
		u64_t pwt:1;

		/** page-level cache disable: determines the memory
		 * type used to access the page table referenced by
		 * this entry
		 */
		u64_t pcd:1;

		u64_t ignored1:7;

		/** page table: physical address of page table */
		u64_t pd:20;

		u64_t ignored3:32;
	};
};

/*
 * The following defines the format of a 32-bit page directory entry
 * that references a page table (as opposed to a 2 Mb page).
 */
union x86_mmu_pde_pt {
	 /** access Page directory entry through use of bitmasks */
	u64_t  value;
	struct {
		/** present: must be 1 to reference a page table */
		u64_t p:1;

		/** read/write: if 0, writes may not be allowed to the region
		 * controlled by this entry
		 */
		u64_t rw:1;

		/** user/supervisor: if 0, accesses with CPL=3 are not allowed
		 * to the region controlled by this entry
		 */
		u64_t us:1;

		/** page-level write-through: determines the memory type used
		 * to access the page table referenced by this entry
		 */
		u64_t pwt:1;

		/** page-level cache disable: determines the memory
		 * type used to access the page table referenced by
		 * this entry
		 */
		u64_t pcd:1;

		/** accessed: if 1 -> entry has been used to translate
		 */
		u64_t a:1;

		u64_t ignored1:1;

		/** page size: ignored when CR4.PSE=0 */
		u64_t ps:1;

		u64_t ignored2:4;

		/** page table: physical address of page table */
		u64_t pt:20;

		u64_t ignored3:31;

		/* Execute disable */
		u64_t xd:1;
	};
};


/*
 * The following defines the format of a 64-bit page directory entry
 * that references a 2 Mb page (as opposed to a page table).
 */

union x86_mmu_pde_2mb {
	u32_t  value;
	struct {
		/** present: must be 1 to map a 4 Mb page */
		u64_t p:1;

		/** read/write: if 0, writes may not be allowed to the 4 Mb
		 * page referenced by this entry
		 */
		u64_t rw:1;

		/** user/supervisor: if 0, accesses with CPL=3 are not allowed
		 * to the 4 Mb page referenced by this entry
		 */
		u64_t us:1;

		/** page-level write-through: determines the memory type used
		 * to access the 4 Mb page referenced by
		 * this entry
		 */
		u64_t pwt:1;

		/** page-level cache disable: determines the memory type used
		 * to access the 4 Mb page referenced by this entry
		 */
		u64_t pcd:1;

		/** accessed: if 1 -> entry has been used to translate */
		u64_t a:1;

		/** dirty: indicates whether software has written to the 4 Mb
		 * page referenced by this entry
		 */
		u64_t d:1;

		/** page size: must be 1 otherwise this entry references a page
		 * table entry
		 */
		u64_t ps:1;

		/** global: if CR4.PGE=1, then determines whether this
		 * translation is global, i.e. used regardless of PCID
		 */
		u64_t g:1;

		u64_t ignored1:3;

		/** If PAT is supported, indirectly determines the memory type
		 * used to access the 4 Mb page, otherwise must be 0
		 */
		u64_t pat:1;

		u64_t reserved1:8;

		/** page table: physical address of page table */
		u64_t pt:11;

		u64_t reserved2:31;

		/** execute disable */
		u64_t xd:1;
	};
};

/*
 * The following defines the format of a 64-bit page table entry that maps
 * a 4 Kb page.
 */
union x86_mmu_pte {
	u64_t  value;

	struct {
		/** present: must be 1 to map a 4 Kb page */
		u64_t p:1;

		/** read/write: if 0, writes may not be allowed to the 4 Kb
		 * page controlled by this entry
		 */
		u64_t rw:1;

		/** user/supervisor: if 0, accesses with CPL=3 are not allowed
		 * to the 4 Kb page controlled by this entry
		 */
		u64_t us:1;

		/** page-level write-through: determines the memory type used
		 * to access the 4 Kb page referenced by this entry
		 */
		u64_t pwt:1;

		/** page-level cache disable: determines the memory type used
		 * to access the 4 Kb page referenced by this entry
		 */
		u64_t pcd:1;

		/** accessed: if 1 -> 4 Kb page has been referenced */
		u64_t a:1;

		/** dirty: if 1 -> 4 Kb page has been written to */
		u64_t d:1;

		/** If PAT is supported, indirectly determines the memory type
		 * used to access the 4 Kb page, otherwise must be 0
		 */
		u64_t pat:1;

		/** global: if CR4.PGE=1, then determines whether this
		 * translation is global, i.e. used regardless of PCID
		 */
		u64_t g:1;

		/** allocated: if 1 -> this PTE has been allocated/ reserved;
		 * this is only used by software, i.e. this bit is ignored by
		 * the MMU
		 */
		u64_t ignore1:3;

		/** page: physical address of the 4 Kb page */
		u64_t page:20;

		u64_t ignore2:31;

		/* Execute disable */
		u64_t xd:1;
	};
};


typedef u64_t x86_page_entry_data_t;

typedef x86_page_entry_data_t k_mem_partition_attr_t;

struct x86_mmu_pdpt {
	union x86_mmu_pdpte entry[4];
};

union x86_mmu_pde {
	union x86_mmu_pde_pt pt;
	union x86_mmu_pde_2mb twomb;
};

struct x86_mmu_pd {
	union x86_mmu_pde entry[512];
};

struct x86_mmu_pt {
	union x86_mmu_pte entry[512];
};

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_MMUSTRUCTS_H_ */
