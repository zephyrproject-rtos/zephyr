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

#ifdef CONFIG_X86_PAE_MODE
#define MMU_PDE_XD_MASK             0x8000000000000000
#define MMU_PDE_PAGE_TABLE_MASK     0x00000000fffff000
#define MMU_PDE_NUM_SHIFT           21
#define MMU_PDE_NUM(v)              (((u32_t)(v) >> MMU_PDE_NUM_SHIFT) & 0x1ff)
#define MMU_ENTRIES_PER_PGT         512
#define MMU_PDPTE_NUM_SHIFT         30
#define MMU_PDPTE_NUM(v)            (((u32_t)(v) >> MMU_PDPTE_NUM_SHIFT) & 0x3)
#else
#define MMU_PDE_PAGE_TABLE_MASK     0xfffff000
#define MMU_PDE_NUM_SHIFT           22
#define MMU_PDE_NUM(v)              ((u32_t)(v) >> MMU_PDE_NUM_SHIFT)
#define MMU_ENTRIES_PER_PGT         1024
#endif

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

#ifdef CONFIG_X86_PAE_MODE
#define MMU_PTE_XD_MASK             0x8000000000000000
#define MMU_PTE_PAGE_MASK           0x00000000fffff000
#define MMU_PTE_MASK_ALL            0xffffffffffffffff
#define MMU_PAGE_NUM(v)             (((u32_t)(v) >> MMU_PAGE_NUM_SHIFT) & 0x1ff)
#else
#define MMU_PTE_PAGE_MASK           0xfffff000
#define MMU_PTE_MASK_ALL            0xffffffff
#define MMU_PAGE_NUM(v)             (((u32_t)(v) >> MMU_PAGE_NUM_SHIFT) & 0x3ff)
#endif

#define MMU_PAGE_NUM_SHIFT 12

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

#ifdef CONFIG_X86_PAE_MODE
#define MMU_ENTRY_EXECUTE_DISABLE   0x8000000000000000
#else
#define MMU_ENTRY_EXECUTE_DISABLE   0x0
#endif

/* Special flag argument for MMU_BOOT region invocations */

/* Indicates that pages within this region may have their user/supervisor
 * permissions adjusted at runtime. Unnecessary if MMU_ENTRY_USER is already
 * set.
 *
 * The result of this is a guarantee that the 'user' bit for all PDEs referring
 * to the region will be set, even if the boot configuration has no user pages
 * in it.
 */
#define MMU_ENTRY_RUNTIME_USER      0x10000000

/* Indicates that pages within this region may have their read/write
 * permissions adjusted at runtime. Unnecessary if MMU_ENTRY_WRITE is already
 * set.
 *
 * The result of this is a guarantee that the 'write' bit for all PDEs
 * referring to the region will be set, even if the boot configuration has no
 * writable pages in it.
 */
#define MMU_ENTRY_RUNTIME_WRITE	    0x20000000


/* Helper macros to ease the usage of the MMU page table structures.
 */
#ifdef CONFIG_X86_PAE_MODE

/*
 * Returns the page table entry for the addr
 * use the union to extract page entry related information.
 */
#define X86_MMU_GET_PTE(addr)\
	((union x86_mmu_pae_pte *)\
	 (&X86_MMU_GET_PT_ADDR(addr)->entry[MMU_PAGE_NUM(addr)]))

/*
 * Returns the Page table address for the particular address.
 * Page Table address(returned value) is always 4KBytes aligned.
 */
#define X86_MMU_GET_PT_ADDR(addr) \
	((struct x86_mmu_page_table *)\
	 (X86_MMU_GET_PDE(addr)->page_table << MMU_PAGE_SHIFT))

/* Returns the page directory entry for the addr
 * use the union to extract page directory entry related information.
 */
#define X86_MMU_GET_PDE(addr)\
	((union x86_mmu_pae_pde *)					\
	 (&X86_MMU_GET_PD_ADDR(addr)->entry[MMU_PDE_NUM(addr)]))

/* Returns the page directory entry for the addr
 * use the union to extract page directory entry related information.
 */
#define X86_MMU_GET_PD_ADDR(addr) \
	((struct x86_mmu_page_directory *)		       \
	 (X86_MMU_GET_PDPTE(addr)->page_directory << MMU_PAGE_SHIFT))

/* Returns the page directory pointer entry */
#define X86_MMU_GET_PDPTE(addr) \
	((union x86_mmu_pae_pdpte *)		       \
	 (&X86_MMU_PDPT->entry[MMU_PDPTE_NUM(addr)]))

/* Return the Page directory address.
 * input is the entry number
 */
#define X86_MMU_GET_PD_ADDR_INDEX(index) \
	((struct x86_mmu_page_directory *)		       \
	 (X86_MMU_GET_PDPTE_INDEX(index)->page_directory << MMU_PAGE_SHIFT))

/* Returns the page directory pointer entry.
 * Input is the entry number
 */
#define X86_MMU_GET_PDPTE_INDEX(index) \
	((union x86_mmu_pae_pdpte *)(&X86_MMU_PDPT->entry[index]))

#else
/* Normal 32-Bit paging */
#define X86_MMU_GET_PT_ADDR(addr) \
	((struct x86_mmu_page_table *)\
	 (X86_MMU_PD->entry[MMU_PDE_NUM(addr)].pt.page_table \
	  << MMU_PAGE_SHIFT))

/* Returns the page table entry for the addr
 * use the union to extract page entry related information.
 */
#define X86_MMU_GET_PTE(addr)\
	((union x86_mmu_pte *)\
	 (&X86_MMU_GET_PT_ADDR(addr)->entry[MMU_PAGE_NUM(addr)]))

/* Returns the page directory entry for the addr
 * use the union to extract page directory entry related information.
 */
#define X86_MMU_GET_PDE(addr)\
	((union x86_mmu_pde_pt *)\
	 (&X86_MMU_PD->entry[MMU_PDE_NUM(addr)].pt))

#define X86_MMU_GET_PD_ADDR(addr) (X86_MMU_PD)

/* Returns the 4 MB page directory entry for the addr
 * use the union to extract page directory entry related information.
 */
#define X86_MMU_GET_4MB_PDE(addr)\
	((union x86_mmu_pde_4mb *)\
	 (&X86_MMU_PD->entry[MMU_PDE_NUM(addr)].fourmb))

#endif	/* CONFIG_X86_PAE_MODE */

#ifdef CONFIG_X86_USERSPACE

/* Flags which are only available for PAE mode page tables  */
#ifdef CONFIG_X86_PAE_MODE

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


#else  /* 32-bit paging mode enabled */

/* memory partition arch/soc independent attribute */
#define K_MEM_PARTITION_P_RW_U_RW   (MMU_ENTRY_WRITE | MMU_ENTRY_USER)

#define K_MEM_PARTITION_P_RW_U_NA   (MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR)

#define K_MEM_PARTITION_P_RO_U_RO   (MMU_ENTRY_READ | MMU_ENTRY_USER)

#define K_MEM_PARTITION_P_RO_U_NA   (MMU_ENTRY_READ  | MMU_ENTRY_SUPERVISOR)

/* memory partition access permission mask */
#define K_MEM_PARTITION_PERM_MASK   (MMU_PTE_RW_MASK | MMU_PTE_US_MASK)

#endif	/* CONFIG_X86_PAE_MODE */

#endif	 /* CONFIG_X86_USERSPACE */

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

#define _MMU_BOOT_REGION(id, addr, region_size, permission_flags)	\
	__MMU_BOOT_REGION(id, addr, region_size, permission_flags)

#define MMU_BOOT_REGION(addr, region_size, permission_flags)		\
	_MMU_BOOT_REGION(__COUNTER__, addr, region_size, permission_flags)

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

/* PAE paging mode structures and unions */

/*
 * The following defines the format of a 64-bit page directory pointer entry
 * that references a page directory table
 */
union x86_mmu_pae_pdpte {
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
		u64_t page_directory:20;

		u64_t ignored3:32;
	};
};

/*
 * The following defines the format of a 32-bit page directory entry
 * that references a page table (as opposed to a 4 Mb page).
 */
union x86_mmu_pae_pde {
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
		u64_t page_table:20;

		u64_t ignored3:31;

		/* Execute disable */
		u64_t xd:1;
	};
};


/*
 * The following defines the format of a 64-bit page directory entry
 * that references a 2 Mb page (as opposed to a page table).
 */

union x86_mmu_pae_pde_2mb {
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
		u64_t page_table:11;

		u64_t reserved2:31;

		/** execute disable */
		u64_t xd:1;
	};
};

/*
 * The following defines the format of a 64-bit page table entry that maps
 * a 4 Kb page.
 */
union x86_mmu_pae_pte {
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


#ifdef CONFIG_X86_PAE_MODE
typedef u64_t x86_page_entry_data_t;
#else
typedef u32_t x86_page_entry_data_t;
#endif

typedef x86_page_entry_data_t k_mem_partition_attr_t;

#ifdef CONFIG_X86_PAE_MODE
struct x86_mmu_page_directory_pointer {
	union x86_mmu_pae_pdpte entry[512];
};
#endif

union x86_mmu_pde {
#ifndef CONFIG_X86_PAE_MODE
	union x86_mmu_pde_pt pt;
	union x86_mmu_pde_4mb fourmb;
#else
	union x86_mmu_pae_pde pt;
	union x86_mmu_pae_pde_2mb twomb;
#endif
};

/** Page Directory structure for 32-bit/PAE paging mode */
struct x86_mmu_page_directory {
#ifndef CONFIG_X86_PAE_MODE
	union x86_mmu_pde entry[1024];
#else
	union x86_mmu_pae_pde entry[512];
#endif
};

/** Page Table structure for 32-bit/PAE paging mode */
struct x86_mmu_page_table {
#ifndef CONFIG_X86_PAE_MODE
	union x86_mmu_pte entry[1024];
#else
	union x86_mmu_pae_pte entry[512];
#endif
};

#endif /* _ASMLANGUAGE */

#endif /* _MMUSTRUCTS_H */
