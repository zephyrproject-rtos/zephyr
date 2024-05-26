/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/x86/mmustructs.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/check.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <ctype.h>
#include <zephyr/spinlock.h>
#include <kernel_arch_func.h>
#include <x86_mmu.h>
#include <zephyr/init.h>
#include <kernel_internal.h>
#include <mmu.h>
#include <zephyr/drivers/interrupt_controller/loapic.h>
#include <mmu.h>
#include <zephyr/arch/x86/memmap.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* We will use some ignored bits in the PTE to backup permission settings
 * when the mapping was made. This is used to un-apply memory domain memory
 * partitions to page tables when the partitions are removed.
 */
#define MMU_RW_ORIG	MMU_IGNORED0
#define MMU_US_ORIG	MMU_IGNORED1
#define MMU_XD_ORIG	MMU_IGNORED2

/* Bits in the PTE that form the set of permission bits, when resetting */
#define MASK_PERM	(MMU_RW | MMU_US | MMU_XD)

/* When we want to set up a new mapping, discarding any previous state */
#define MASK_ALL	(~((pentry_t)0U))

/* Bits to set at mapping time for particular permissions. We set the actual
 * page table bit effecting the policy and also the backup bit.
 */
#define ENTRY_RW	(MMU_RW | MMU_RW_ORIG)
#define ENTRY_US	(MMU_US | MMU_US_ORIG)
#define ENTRY_XD	(MMU_XD | MMU_XD_ORIG)

/* Bit position which is always zero in a PTE. We'll use the PAT bit.
 * This helps disambiguate PTEs that do not have the Present bit set (MMU_P):
 * - If the entire entry is zero, it's an un-mapped virtual page
 * - If PTE_ZERO is set, we flipped this page due to KPTI
 * - Otherwise, this was a page-out
 */
#define PTE_ZERO	MMU_PAT

/* Protects x86_domain_list and serializes instantiation of intermediate
 * paging structures.
 */
__pinned_bss
static struct k_spinlock x86_mmu_lock;

#if defined(CONFIG_USERSPACE) && !defined(CONFIG_X86_COMMON_PAGE_TABLE)
/* List of all active and initialized memory domains. This is used to make
 * sure all memory mappings are the same across all page tables when invoking
 * range_map()
 */
__pinned_bss
static sys_slist_t x86_domain_list;
#endif

/*
 * Definitions for building an ontology of paging levels and capabilities
 * at each level
 */

/* Data structure describing the characteristics of a particular paging
 * level
 */
struct paging_level {
	/* What bits are used to store physical address */
	pentry_t mask;

	/* Number of entries in this paging structure */
	size_t entries;

	/* How many bits to right-shift a virtual address to obtain the
	 * appropriate entry within this table.
	 *
	 * The memory scope of each entry in this table is 1 << shift.
	 */
	unsigned int shift;
#ifdef CONFIG_EXCEPTION_DEBUG
	/* Name of this level, for debug purposes */
	const char *name;
#endif
};

/* Flags for all entries in intermediate paging levels.
 * Fortunately, the same bits are set for all intermediate levels for all
 * three paging modes.
 *
 * Obviously P is set.
 *
 * We want RW and US bit always set; actual access control will be
 * done at the leaf level.
 *
 * XD (if supported) always 0. Disabling execution done at leaf level.
 *
 * PCD/PWT always 0. Caching properties again done at leaf level.
 */
#define INT_FLAGS	(MMU_P | MMU_RW | MMU_US)

/* Paging level ontology for the selected paging mode.
 *
 * See Figures 4-4, 4-7, 4-11 in the Intel SDM, vol 3A
 */
__pinned_rodata
static const struct paging_level paging_levels[] = {
#ifdef CONFIG_X86_64
	/* Page Map Level 4 */
	{
		.mask = 0x7FFFFFFFFFFFF000ULL,
		.entries = 512U,
		.shift = 39U,
#ifdef CONFIG_EXCEPTION_DEBUG
		.name = "PML4"
#endif
	},
#endif /* CONFIG_X86_64 */
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
	/* Page Directory Pointer Table */
	{
		.mask = 0x7FFFFFFFFFFFF000ULL,
#ifdef CONFIG_X86_64
		.entries = 512U,
#else
		/* PAE version */
		.entries = 4U,
#endif
		.shift = 30U,
#ifdef CONFIG_EXCEPTION_DEBUG
		.name = "PDPT"
#endif
	},
#endif /* CONFIG_X86_64 || CONFIG_X86_PAE */
	/* Page Directory */
	{
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
		.mask = 0x7FFFFFFFFFFFF000ULL,
		.entries = 512U,
		.shift = 21U,
#else
		/* 32-bit */
		.mask = 0xFFFFF000U,
		.entries = 1024U,
		.shift = 22U,
#endif /* CONFIG_X86_64 || CONFIG_X86_PAE */
#ifdef CONFIG_EXCEPTION_DEBUG
		.name = "PD"
#endif
	},
	/* Page Table */
	{
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
		.mask = 0x07FFFFFFFFFFF000ULL,
		.entries = 512U,
		.shift = 12U,
#else
		/* 32-bit */
		.mask = 0xFFFFF000U,
		.entries = 1024U,
		.shift = 12U,
#endif /* CONFIG_X86_64 || CONFIG_X86_PAE */
#ifdef CONFIG_EXCEPTION_DEBUG
		.name = "PT"
#endif
	}
};

#define NUM_LEVELS	ARRAY_SIZE(paging_levels)
#define PTE_LEVEL	(NUM_LEVELS - 1)
#define PDE_LEVEL	(NUM_LEVELS - 2)

/*
 * Macros for reserving space for page tables
 *
 * We need to reserve a block of memory equal in size to the page tables
 * generated by gen_mmu.py so that memory addresses do not shift between
 * build phases. These macros ultimately specify INITIAL_PAGETABLE_SIZE.
 */
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
#ifdef CONFIG_X86_64
#define NUM_PML4_ENTRIES 512U
#define NUM_PDPT_ENTRIES 512U
#else
#define NUM_PDPT_ENTRIES 4U
#endif /* CONFIG_X86_64 */
#define NUM_PD_ENTRIES   512U
#define NUM_PT_ENTRIES   512U
#else
#define NUM_PD_ENTRIES   1024U
#define NUM_PT_ENTRIES   1024U
#endif /* !CONFIG_X86_64 && !CONFIG_X86_PAE */

/* Memory range covered by an instance of various table types */
#define PT_AREA		((uintptr_t)(CONFIG_MMU_PAGE_SIZE * NUM_PT_ENTRIES))
#define PD_AREA 	(PT_AREA * NUM_PD_ENTRIES)
#ifdef CONFIG_X86_64
#define PDPT_AREA	(PD_AREA * NUM_PDPT_ENTRIES)
#endif

#define VM_ADDR		CONFIG_KERNEL_VM_BASE
#define VM_SIZE		CONFIG_KERNEL_VM_SIZE

/* Define a range [PT_START, PT_END) which is the memory range
 * covered by all the page tables needed for the address space
 */
#define PT_START	((uintptr_t)ROUND_DOWN(VM_ADDR, PT_AREA))
#define PT_END		((uintptr_t)ROUND_UP(VM_ADDR + VM_SIZE, PT_AREA))

/* Number of page tables needed to cover address space. Depends on the specific
 * bounds, but roughly 1 page table per 2MB of RAM
 */
#define NUM_PT	((PT_END - PT_START) / PT_AREA)

#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
/* Same semantics as above, but for the page directories needed to cover
 * system RAM.
 */
#define PD_START	((uintptr_t)ROUND_DOWN(VM_ADDR, PD_AREA))
#define PD_END		((uintptr_t)ROUND_UP(VM_ADDR + VM_SIZE, PD_AREA))
/* Number of page directories needed to cover the address space. Depends on the
 * specific bounds, but roughly 1 page directory per 1GB of RAM
 */
#define NUM_PD	((PD_END - PD_START) / PD_AREA)
#else
/* 32-bit page tables just have one toplevel page directory */
#define NUM_PD	1
#endif

#ifdef CONFIG_X86_64
/* Same semantics as above, but for the page directory pointer tables needed
 * to cover the address space. On 32-bit there is just one 4-entry PDPT.
 */
#define PDPT_START	((uintptr_t)ROUND_DOWN(VM_ADDR, PDPT_AREA))
#define PDPT_END	((uintptr_t)ROUND_UP(VM_ADDR + VM_SIZE, PDPT_AREA))
/* Number of PDPTs needed to cover the address space. 1 PDPT per 512GB of VM */
#define NUM_PDPT	((PDPT_END - PDPT_START) / PDPT_AREA)

/* All pages needed for page tables, using computed values plus one more for
 * the top-level PML4
 */
#define NUM_TABLE_PAGES	(NUM_PT + NUM_PD + NUM_PDPT + 1)
#else /* !CONFIG_X86_64 */
/* Number of pages we need to reserve in the stack for per-thread page tables */
#define NUM_TABLE_PAGES	(NUM_PT + NUM_PD)
#endif /* CONFIG_X86_64 */

#define INITIAL_PTABLE_PAGES \
	(NUM_TABLE_PAGES + CONFIG_X86_EXTRA_PAGE_TABLE_PAGES)

#ifdef CONFIG_X86_PAE
/* Toplevel PDPT wasn't included as it is not a page in size */
#define INITIAL_PTABLE_SIZE \
	((INITIAL_PTABLE_PAGES * CONFIG_MMU_PAGE_SIZE) + 0x20)
#else
#define INITIAL_PTABLE_SIZE \
	(INITIAL_PTABLE_PAGES * CONFIG_MMU_PAGE_SIZE)
#endif

/* "dummy" pagetables for the first-phase build. The real page tables
 * are produced by gen-mmu.py based on data read in zephyr-prebuilt.elf,
 * and this dummy array is discarded.
 */
Z_GENERIC_SECTION(.dummy_pagetables)
static __used char dummy_pagetables[INITIAL_PTABLE_SIZE];

/*
 * Utility functions
 */

/* For a table at a particular level, get the entry index that corresponds to
 * the provided virtual address
 */
__pinned_func
static inline int get_index(void *virt, int level)
{
	return (((uintptr_t)virt >> paging_levels[level].shift) %
		paging_levels[level].entries);
}

__pinned_func
static inline pentry_t *get_entry_ptr(pentry_t *ptables, void *virt, int level)
{
	return &ptables[get_index(virt, level)];
}

__pinned_func
static inline pentry_t get_entry(pentry_t *ptables, void *virt, int level)
{
	return ptables[get_index(virt, level)];
}

/* Get the physical memory address associated with this table entry */
__pinned_func
static inline uintptr_t get_entry_phys(pentry_t entry, int level)
{
	return entry & paging_levels[level].mask;
}

/* Return the virtual address of a linked table stored in the provided entry */
__pinned_func
static inline pentry_t *next_table(pentry_t entry, int level)
{
	return z_mem_virt_addr(get_entry_phys(entry, level));
}

/* Number of table entries at this level */
__pinned_func
static inline size_t get_num_entries(int level)
{
	return paging_levels[level].entries;
}

/* 4K for everything except PAE PDPTs */
__pinned_func
static inline size_t table_size(int level)
{
	return get_num_entries(level) * sizeof(pentry_t);
}

/* For a table at a particular level, size of the amount of virtual memory
 * that an entry within the table covers
 */
__pinned_func
static inline size_t get_entry_scope(int level)
{
	return (1UL << paging_levels[level].shift);
}

/* For a table at a particular level, size of the amount of virtual memory
 * that this entire table covers
 */
__pinned_func
static inline size_t get_table_scope(int level)
{
	return get_entry_scope(level) * get_num_entries(level);
}

/* Must have checked Present bit first! Non-present entries may have OS data
 * stored in any other bits
 */
__pinned_func
static inline bool is_leaf(int level, pentry_t entry)
{
	if (level == PTE_LEVEL) {
		/* Always true for PTE */
		return true;
	}

	return ((entry & MMU_PS) != 0U);
}

/* This does NOT (by design) un-flip KPTI PTEs, it's just the raw PTE value */
__pinned_func
static inline void pentry_get(int *paging_level, pentry_t *val,
			      pentry_t *ptables, void *virt)
{
	pentry_t *table = ptables;

	for (int level = 0; level < NUM_LEVELS; level++) {
		pentry_t entry = get_entry(table, virt, level);

		if ((entry & MMU_P) == 0 || is_leaf(level, entry)) {
			*val = entry;
			if (paging_level != NULL) {
				*paging_level = level;
			}
			break;
		} else {
			table = next_table(entry, level);
		}
	}
}

__pinned_func
static inline void tlb_flush_page(void *addr)
{
	/* Invalidate TLB entries corresponding to the page containing the
	 * specified address
	 */
	char *page = (char *)addr;

	__asm__ ("invlpg %0" :: "m" (*page));
}

#ifdef CONFIG_X86_KPTI
__pinned_func
static inline bool is_flipped_pte(pentry_t pte)
{
	return (pte & MMU_P) == 0 && (pte & PTE_ZERO) != 0;
}
#endif

#if defined(CONFIG_SMP)
__pinned_func
void z_x86_tlb_ipi(const void *arg)
{
	uintptr_t ptables_phys;

	ARG_UNUSED(arg);

#ifdef CONFIG_X86_KPTI
	/* We're always on the kernel's set of page tables in this context
	 * if KPTI is turned on
	 */
	ptables_phys = z_x86_cr3_get();
	__ASSERT(ptables_phys == z_mem_phys_addr(&z_x86_kernel_ptables), "");
#else
	/* We might have been moved to another memory domain, so always invoke
	 * z_x86_thread_page_tables_get() instead of using current CR3 value.
	 */
	ptables_phys = z_mem_phys_addr(z_x86_thread_page_tables_get(_current));
#endif
	/*
	 * In the future, we can consider making this smarter, such as
	 * propagating which page tables were modified (in case they are
	 * not active on this CPU) or an address range to call
	 * tlb_flush_page() on.
	 */
	LOG_DBG("%s on CPU %d\n", __func__, arch_curr_cpu()->id);

	z_x86_cr3_set(ptables_phys);
}

/* NOTE: This is not synchronous and the actual flush takes place some short
 * time after this exits.
 */
__pinned_func
static inline void tlb_shootdown(void)
{
	z_loapic_ipi(0, LOAPIC_ICR_IPI_OTHERS, CONFIG_TLB_IPI_VECTOR);
}
#endif /* CONFIG_SMP */

__pinned_func
static inline void assert_addr_aligned(uintptr_t addr)
{
#if __ASSERT_ON
	__ASSERT((addr & (CONFIG_MMU_PAGE_SIZE - 1)) == 0U,
		 "unaligned address 0x%" PRIxPTR, addr);
#else
	ARG_UNUSED(addr);
#endif
}

__pinned_func
static inline bool is_addr_aligned(uintptr_t addr)
{
	if ((addr & (CONFIG_MMU_PAGE_SIZE - 1)) == 0U) {
		return true;
	} else {
		return false;
	}
}

__pinned_func
static inline void assert_virt_addr_aligned(void *addr)
{
	assert_addr_aligned((uintptr_t)addr);
}

__pinned_func
static inline bool is_virt_addr_aligned(void *addr)
{
	return is_addr_aligned((uintptr_t)addr);
}

__pinned_func
static inline void assert_size_aligned(size_t size)
{
#if __ASSERT_ON
	__ASSERT((size & (CONFIG_MMU_PAGE_SIZE - 1)) == 0U,
		 "unaligned size %zu", size);
#else
	ARG_UNUSED(size);
#endif
}

__pinned_func
static inline bool is_size_aligned(size_t size)
{
	if ((size & (CONFIG_MMU_PAGE_SIZE - 1)) == 0U) {
		return true;
	} else {
		return false;
	}
}

__pinned_func
static inline void assert_region_page_aligned(void *addr, size_t size)
{
	assert_virt_addr_aligned(addr);
	assert_size_aligned(size);
}

__pinned_func
static inline bool is_region_page_aligned(void *addr, size_t size)
{
	if (!is_virt_addr_aligned(addr)) {
		return false;
	}

	return is_size_aligned(size);
}

/*
 * Debug functions. All conditionally compiled with CONFIG_EXCEPTION_DEBUG.
 */
#ifdef CONFIG_EXCEPTION_DEBUG

/* Add colors to page table dumps to indicate mapping type */
#define COLOR_PAGE_TABLES	1

#if COLOR_PAGE_TABLES
#define ANSI_DEFAULT "\x1B" "[0m"
#define ANSI_RED     "\x1B" "[1;31m"
#define ANSI_GREEN   "\x1B" "[1;32m"
#define ANSI_YELLOW  "\x1B" "[1;33m"
#define ANSI_BLUE    "\x1B" "[1;34m"
#define ANSI_MAGENTA "\x1B" "[1;35m"
#define ANSI_CYAN    "\x1B" "[1;36m"
#define ANSI_GREY    "\x1B" "[1;90m"

#define COLOR(x)	printk(_CONCAT(ANSI_, x))
#else
#define COLOR(x)	do { } while (false)
#endif

__pinned_func
static char get_entry_code(pentry_t value)
{
	char ret;

	if (value == 0U) {
		/* Unmapped entry */
		ret = '.';
	} else {
		if ((value & MMU_RW) != 0U) {
			/* Writable page */
			if ((value & MMU_XD) != 0U) {
				/* RW */
				ret = 'w';
			} else {
				/* RWX */
				ret = 'a';
			}
		} else {
			if ((value & MMU_XD) != 0U) {
				/* R */
				ret = 'r';
			} else {
				/* RX */
				ret = 'x';
			}
		}

		if ((value & MMU_US) != 0U) {
			/* Uppercase indicates user mode access */
			ret = toupper((unsigned char)ret);
		}
	}

	return ret;
}

__pinned_func
static void print_entries(pentry_t entries_array[], uint8_t *base, int level,
			  size_t count)
{
	int column = 0;

	for (int i = 0; i < count; i++) {
		pentry_t entry = entries_array[i];

		uintptr_t phys = get_entry_phys(entry, level);
		uintptr_t virt =
			(uintptr_t)base + (get_entry_scope(level) * i);

		if ((entry & MMU_P) != 0U) {
			if (is_leaf(level, entry)) {
				if (phys == virt) {
					/* Identity mappings */
					COLOR(YELLOW);
				} else if (phys + Z_MEM_VM_OFFSET == virt) {
					/* Permanent RAM mappings */
					COLOR(GREEN);
				} else {
					/* General mapped pages */
					COLOR(CYAN);
				}
			} else {
				/* Intermediate entry */
				COLOR(MAGENTA);
			}
		} else {
			if (is_leaf(level, entry)) {
				if (entry == 0U) {
					/* Unmapped */
					COLOR(GREY);
#ifdef CONFIG_X86_KPTI
				} else if (is_flipped_pte(entry)) {
					/* KPTI, un-flip it */
					COLOR(BLUE);
					entry = ~entry;
					phys = get_entry_phys(entry, level);
					if (phys == virt) {
						/* Identity mapped */
						COLOR(CYAN);
					} else {
						/* Non-identity mapped */
						COLOR(BLUE);
					}
#endif
				} else {
					/* Paged out */
					COLOR(RED);
				}
			} else {
				/* Un-mapped intermediate entry */
				COLOR(GREY);
			}
		}

		printk("%c", get_entry_code(entry));

		column++;
		if (column == 64) {
			column = 0;
			printk("\n");
		}
	}
	COLOR(DEFAULT);

	if (column != 0) {
		printk("\n");
	}
}

__pinned_func
static void dump_ptables(pentry_t *table, uint8_t *base, int level)
{
	const struct paging_level *info = &paging_levels[level];

#ifdef CONFIG_X86_64
	/* Account for the virtual memory "hole" with sign-extension */
	if (((uintptr_t)base & BITL(47)) != 0) {
		base = (uint8_t *)((uintptr_t)base | (0xFFFFULL << 48));
	}
#endif

	printk("%s at %p (0x%" PRIxPTR "): ", info->name, table,
	       z_mem_phys_addr(table));
	if (level == 0) {
		printk("entire address space\n");
	} else {
		printk("for %p - %p\n", base,
		       base + get_table_scope(level) - 1);
	}

	print_entries(table, base, level, info->entries);

	/* Check if we're a page table */
	if (level == PTE_LEVEL) {
		return;
	}

	/* Dump all linked child tables */
	for (int j = 0; j < info->entries; j++) {
		pentry_t entry = table[j];
		pentry_t *next;

		if ((entry & MMU_P) == 0U ||
			(entry & MMU_PS) != 0U) {
			/* Not present or big page, skip */
			continue;
		}

		next = next_table(entry, level);
		dump_ptables(next, base + (j * get_entry_scope(level)),
			     level + 1);
	}
}

__pinned_func
void z_x86_dump_page_tables(pentry_t *ptables)
{
	dump_ptables(ptables, NULL, 0);
}

/* Enable to dump out the kernel's page table right before main() starts,
 * sometimes useful for deep debugging. May overwhelm twister.
 */
#define DUMP_PAGE_TABLES 0

#if DUMP_PAGE_TABLES
__pinned_func
static int dump_kernel_tables(void)
{
	z_x86_dump_page_tables(z_x86_kernel_ptables);

	return 0;
}

SYS_INIT(dump_kernel_tables, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

__pinned_func
static void str_append(char **buf, size_t *size, const char *str)
{
	int ret = snprintk(*buf, *size, "%s", str);

	if (ret >= *size) {
		/* Truncated */
		*size = 0U;
	} else {
		*size -= ret;
		*buf += ret;
	}

}

__pinned_func
static void dump_entry(int level, void *virt, pentry_t entry)
{
	const struct paging_level *info = &paging_levels[level];
	char buf[24] = { 0 };
	char *pos = buf;
	size_t sz = sizeof(buf);
	uint8_t *virtmap = (uint8_t *)ROUND_DOWN(virt, get_entry_scope(level));

	#define DUMP_BIT(bit) do { \
			if ((entry & MMU_##bit) != 0U) { \
				str_append(&pos, &sz, #bit " "); \
			} \
		} while (false)

	DUMP_BIT(RW);
	DUMP_BIT(US);
	DUMP_BIT(PWT);
	DUMP_BIT(PCD);
	DUMP_BIT(A);
	DUMP_BIT(D);
	DUMP_BIT(G);
	DUMP_BIT(XD);

	LOG_ERR("%sE: %p -> " PRI_ENTRY ": %s", info->name,
		virtmap, entry & info->mask, buf);

	#undef DUMP_BIT
}

__pinned_func
void z_x86_pentry_get(int *paging_level, pentry_t *val, pentry_t *ptables,
		      void *virt)
{
	pentry_get(paging_level, val, ptables, virt);
}

/*
 * Debug function for dumping out MMU table information to the LOG for a
 * specific virtual address, such as when we get an unexpected page fault.
 */
__pinned_func
void z_x86_dump_mmu_flags(pentry_t *ptables, void *virt)
{
	pentry_t entry = 0;
	int level = 0;

	pentry_get(&level, &entry, ptables, virt);

	if ((entry & MMU_P) == 0) {
		LOG_ERR("%sE: not present", paging_levels[level].name);
	} else {
		dump_entry(level, virt, entry);
	}
}
#endif /* CONFIG_EXCEPTION_DEBUG */

/* Reset permissions on a PTE to original state when the mapping was made */
__pinned_func
static inline pentry_t reset_pte(pentry_t old_val)
{
	pentry_t new_val;

	/* Clear any existing state in permission bits */
	new_val = old_val & (~K_MEM_PARTITION_PERM_MASK);

	/* Now set permissions based on the stashed original values */
	if ((old_val & MMU_RW_ORIG) != 0) {
		new_val |= MMU_RW;
	}
	if ((old_val & MMU_US_ORIG) != 0) {
		new_val |= MMU_US;
	}
#if defined(CONFIG_X86_64) || defined(CONFIG_X86_PAE)
	if ((old_val & MMU_XD_ORIG) != 0) {
		new_val |= MMU_XD;
	}
#endif
	return new_val;
}

/* Wrapper functions for some gross stuff we have to do for Kernel
 * page table isolation. If these are User mode page tables, the user bit
 * isn't set, and this is not the shared page, all the bits in the PTE
 * are flipped. This serves three purposes:
 *  - The page isn't present, implementing page table isolation
 *  - Flipping the physical address bits cheaply mitigates L1TF
 *  - State is preserved; to get original PTE, just complement again
 */
__pinned_func
static inline pentry_t pte_finalize_value(pentry_t val, bool user_table,
					  int level)
{
#ifdef CONFIG_X86_KPTI
	static const uintptr_t shared_phys_addr =
		Z_MEM_PHYS_ADDR(POINTER_TO_UINT(&z_shared_kernel_page_start));

	if (user_table && (val & MMU_US) == 0 && (val & MMU_P) != 0 &&
	    get_entry_phys(val, level) != shared_phys_addr) {
		val = ~val;
	}
#else
	ARG_UNUSED(user_table);
	ARG_UNUSED(level);
#endif
	return val;
}

/* Atomic functions for modifying PTEs. These don't map nicely to Zephyr's
 * atomic API since the only types supported are 'int' and 'void *' and
 * the size of pentry_t depends on other factors like PAE.
 */
#ifndef CONFIG_X86_PAE
/* Non-PAE, pentry_t is same size as void ptr so use atomic_ptr_* APIs */
__pinned_func
static inline pentry_t atomic_pte_get(const pentry_t *target)
{
	return (pentry_t)atomic_ptr_get((atomic_ptr_t *)target);
}

__pinned_func
static inline bool atomic_pte_cas(pentry_t *target, pentry_t old_value,
				  pentry_t new_value)
{
	return atomic_ptr_cas((atomic_ptr_t *)target, (void *)old_value,
			      (void *)new_value);
}
#else
/* Atomic builtins for 64-bit values on 32-bit x86 require floating point.
 * Don't do this, just lock local interrupts. Needless to say, this
 * isn't workable if someone ever adds SMP to the 32-bit x86 port.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP));

__pinned_func
static inline pentry_t atomic_pte_get(const pentry_t *target)
{
	return *target;
}

__pinned_func
static inline bool atomic_pte_cas(pentry_t *target, pentry_t old_value,
				  pentry_t new_value)
{
	bool ret = false;
	int key = arch_irq_lock();

	if (*target == old_value) {
		*target = new_value;
		ret = true;
	}
	arch_irq_unlock(key);

	return ret;
}
#endif /* CONFIG_X86_PAE */

/* Indicates that the target page tables will be used by user mode threads.
 * This only has implications for CONFIG_X86_KPTI where user thread facing
 * page tables need nearly all pages that don't have the US bit to also
 * not be Present.
 */
#define OPTION_USER		BIT(0)

/* Indicates that the operation requires TLBs to be flushed as we are altering
 * existing mappings. Not needed for establishing new mappings
 */
#define OPTION_FLUSH		BIT(1)

/* Indicates that each PTE's permission bits should be restored to their
 * original state when the memory was mapped. All other bits in the PTE are
 * preserved.
 */
#define OPTION_RESET		BIT(2)

/* Indicates that the mapping will need to be cleared entirely. This is
 * mainly used for unmapping the memory region.
 */
#define OPTION_CLEAR		BIT(3)

/**
 * Atomically update bits in a page table entry
 *
 * This is atomic with respect to modifications by other CPUs or preempted
 * contexts, which can be very important when making decisions based on
 * the PTE's prior "dirty" state.
 *
 * @param pte Pointer to page table entry to update
 * @param update_val Updated bits to set/clear in PTE. Ignored with
 *        OPTION_RESET or OPTION_CLEAR.
 * @param update_mask Which bits to modify in the PTE. Ignored with
 *        OPTION_RESET or OPTION_CLEAR.
 * @param options Control flags
 * @retval Old PTE value
 */
__pinned_func
static inline pentry_t pte_atomic_update(pentry_t *pte, pentry_t update_val,
					 pentry_t update_mask,
					 uint32_t options)
{
	bool user_table = (options & OPTION_USER) != 0U;
	bool reset = (options & OPTION_RESET) != 0U;
	bool clear = (options & OPTION_CLEAR) != 0U;
	pentry_t old_val, new_val;

	do {
		old_val = atomic_pte_get(pte);

		new_val = old_val;
#ifdef CONFIG_X86_KPTI
		if (is_flipped_pte(new_val)) {
			/* Page was flipped for KPTI. Un-flip it */
			new_val = ~new_val;
		}
#endif /* CONFIG_X86_KPTI */

		if (reset) {
			new_val = reset_pte(new_val);
		} else if (clear) {
			new_val = 0;
		} else {
			new_val = ((new_val & ~update_mask) |
				   (update_val & update_mask));
		}

		new_val = pte_finalize_value(new_val, user_table, PTE_LEVEL);
	} while (atomic_pte_cas(pte, old_val, new_val) == false);

#ifdef CONFIG_X86_KPTI
	if (is_flipped_pte(old_val)) {
		/* Page was flipped for KPTI. Un-flip it */
		old_val = ~old_val;
	}
#endif /* CONFIG_X86_KPTI */

	return old_val;
}

/**
 * Low level page table update function for a virtual page
 *
 * For the provided set of page tables, update the PTE associated with the
 * virtual address to a new value, using the mask to control what bits
 * need to be preserved.
 *
 * It is permitted to set up mappings without the Present bit set, in which
 * case all other bits may be used for OS accounting.
 *
 * This function is atomic with respect to the page table entries being
 * modified by another CPU, using atomic operations to update the requested
 * bits and return the previous PTE value.
 *
 * Common mask values:
 *  MASK_ALL  - Update all PTE bits. Existing state totally discarded.
 *  MASK_PERM - Only update permission bits. All other bits and physical
 *              mapping preserved.
 *
 * @param ptables Page tables to modify
 * @param virt Virtual page table entry to update
 * @param entry_val Value to update in the PTE (ignored if OPTION_RESET or
 *        OPTION_CLEAR)
 * @param [out] old_val_ptr Filled in with previous PTE value. May be NULL.
 * @param mask What bits to update in the PTE (ignored if OPTION_RESET or
 *        OPTION_CLEAR)
 * @param options Control options, described above
 *
 * @retval 0 if successful
 * @retval -EFAULT if large page encountered or missing page table level
 */
__pinned_func
static int page_map_set(pentry_t *ptables, void *virt, pentry_t entry_val,
			pentry_t *old_val_ptr, pentry_t mask, uint32_t options)
{
	pentry_t *table = ptables;
	bool flush = (options & OPTION_FLUSH) != 0U;
	int ret = 0;

	for (int level = 0; level < NUM_LEVELS; level++) {
		int index;
		pentry_t *entryp;

		index = get_index(virt, level);
		entryp = &table[index];

		/* Check if we're a PTE */
		if (level == PTE_LEVEL) {
			pentry_t old_val = pte_atomic_update(entryp, entry_val,
							     mask, options);
			if (old_val_ptr != NULL) {
				*old_val_ptr = old_val;
			}
			break;
		}

		/* We bail out early here due to no support for
		 * splitting existing bigpage mappings.
		 * If the PS bit is not supported at some level (like
		 * in a PML4 entry) it is always reserved and must be 0
		 */
		CHECKIF(!((*entryp & MMU_PS) == 0U)) {
			/* Cannot continue since we cannot split
			 * bigpage mappings.
			 */
			LOG_ERR("large page encountered");
			ret = -EFAULT;
			goto out;
		}

		table = next_table(*entryp, level);

		CHECKIF(!(table != NULL)) {
			/* Cannot continue since table is NULL,
			 * and it cannot be dereferenced in next loop
			 * iteration.
			 */
			LOG_ERR("missing page table level %d when trying to map %p",
				level + 1, virt);
			ret = -EFAULT;
			goto out;
		}
	}

out:
	if (flush) {
		tlb_flush_page(virt);
	}

	return ret;
}

/**
 * Map a physical region in a specific set of page tables.
 *
 * See documentation for page_map_set() for additional notes about masks and
 * supported options.
 *
 * It is vital to remember that all virtual-to-physical mappings must be
 * the same with respect to supervisor mode regardless of what thread is
 * scheduled (and therefore, if multiple sets of page tables exist, which one
 * is active).
 *
 * It is permitted to set up mappings without the Present bit set.
 *
 * @param ptables Page tables to modify
 * @param virt Base page-aligned virtual memory address to map the region.
 * @param phys Base page-aligned physical memory address for the region.
 *        Ignored if OPTION_RESET or OPTION_CLEAR. Also affected by the mask
 *        parameter. This address is not directly examined, it will simply be
 *        programmed into the PTE.
 * @param size Size of the physical region to map
 * @param entry_flags Non-address bits to set in every PTE. Ignored if
 *        OPTION_RESET. Also affected by the mask parameter.
 * @param mask What bits to update in each PTE. Un-set bits will never be
 *        modified. Ignored if OPTION_RESET or OPTION_CLEAR.
 * @param options Control options, described above
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters are supplied
 * @retval -EFAULT if errors encountered when updating page tables
 */
__pinned_func
static int range_map_ptables(pentry_t *ptables, void *virt, uintptr_t phys,
			     size_t size, pentry_t entry_flags, pentry_t mask,
			     uint32_t options)
{
	bool zero_entry = (options & (OPTION_RESET | OPTION_CLEAR)) != 0U;
	int ret = 0, ret2;

	CHECKIF(!is_addr_aligned(phys) || !is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	CHECKIF(!((entry_flags & paging_levels[0].mask) == 0U)) {
		LOG_ERR("entry_flags " PRI_ENTRY " overlaps address area",
			entry_flags);
		ret = -EINVAL;
		goto out;
	}

	/* This implementation is stack-efficient but not particularly fast.
	 * We do a full page table walk for every page we are updating.
	 * Recursive approaches are possible, but use much more stack space.
	 */
	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		uint8_t *dest_virt = (uint8_t *)virt + offset;
		pentry_t entry_val;

		if (zero_entry) {
			entry_val = 0;
		} else {
			entry_val = (pentry_t)(phys + offset) | entry_flags;
		}

		ret2 = page_map_set(ptables, dest_virt, entry_val, NULL, mask,
				   options);
		ARG_UNUSED(ret2);
		CHECKIF(ret2 != 0) {
			ret = ret2;
		}
	}

out:
	return ret;
}

/**
 * Establish or update a memory mapping for all page tables
 *
 * The physical region noted from phys to phys + size will be mapped to
 * an equal sized virtual region starting at virt, with the provided flags.
 * The mask value denotes what bits in PTEs will actually be modified.
 *
 * See range_map_ptables() for additional details.
 *
 * @param virt Page-aligned starting virtual address
 * @param phys Page-aligned starting physical address. Ignored if the mask
 *             parameter does not enable address bits or OPTION_RESET used.
 *             This region is not directly examined, it will simply be
 *             programmed into the page tables.
 * @param size Size of the physical region to map
 * @param entry_flags Desired state of non-address PTE bits covered by mask,
 *                    ignored if OPTION_RESET
 * @param mask What bits in the PTE to actually modify; unset bits will
 *             be preserved. Ignored if OPTION_RESET.
 * @param options Control options. Do not set OPTION_USER here. OPTION_FLUSH
 *                will trigger a TLB shootdown after all tables are updated.
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters are supplied
 * @retval -EFAULT if errors encountered when updating page tables
 */
__pinned_func
static int range_map(void *virt, uintptr_t phys, size_t size,
		     pentry_t entry_flags, pentry_t mask, uint32_t options)
{
	int ret = 0, ret2;

	LOG_DBG("%s: %p -> %p (%zu) flags " PRI_ENTRY " mask "
		PRI_ENTRY " opt 0x%x", __func__, (void *)phys, virt, size,
		entry_flags, mask, options);

#ifdef CONFIG_X86_64
	/* There's a gap in the "64-bit" address space, as 4-level paging
	 * requires bits 48 to 63 to be copies of bit 47. Test this
	 * by treating as a signed value and shifting.
	 */
	__ASSERT(((((intptr_t)virt) << 16) >> 16) == (intptr_t)virt,
		 "non-canonical virtual address mapping %p (size %zu)",
		 virt, size);
#endif /* CONFIG_X86_64 */

	CHECKIF(!((options & OPTION_USER) == 0U)) {
		LOG_ERR("invalid option for mapping");
		ret = -EINVAL;
		goto out;
	}

	/* All virtual-to-physical mappings are the same in all page tables.
	 * What can differ is only access permissions, defined by the memory
	 * domain associated with the page tables, and the threads that are
	 * members of that domain.
	 *
	 * Any new mappings need to be applied to all page tables.
	 */
#if defined(CONFIG_USERSPACE) && !defined(CONFIG_X86_COMMON_PAGE_TABLE)
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&x86_domain_list, node) {
		struct arch_mem_domain *domain =
			CONTAINER_OF(node, struct arch_mem_domain, node);

		ret2 = range_map_ptables(domain->ptables, virt, phys, size,
					 entry_flags, mask,
					 options | OPTION_USER);
		ARG_UNUSED(ret2);
		CHECKIF(ret2 != 0) {
			ret = ret2;
		}
	}
#endif /* CONFIG_USERSPACE */

	ret2 = range_map_ptables(z_x86_kernel_ptables, virt, phys, size,
				 entry_flags, mask, options);
	ARG_UNUSED(ret2);
	CHECKIF(ret2 != 0) {
		ret = ret2;
	}

out:
#ifdef CONFIG_SMP
	if ((options & OPTION_FLUSH) != 0U) {
		tlb_shootdown();
	}
#endif /* CONFIG_SMP */

	return ret;
}

__pinned_func
static inline int range_map_unlocked(void *virt, uintptr_t phys, size_t size,
				     pentry_t entry_flags, pentry_t mask,
				     uint32_t options)
{
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&x86_mmu_lock);
	ret = range_map(virt, phys, size, entry_flags, mask, options);
	k_spin_unlock(&x86_mmu_lock, key);

	return ret;
}

__pinned_func
static pentry_t flags_to_entry(uint32_t flags)
{
	pentry_t entry_flags = MMU_P;

	/* Translate flags argument into HW-recognized entry flags.
	 *
	 * Support for PAT is not implemented yet. Many systems may have
	 * BIOS-populated MTRR values such that these cache settings are
	 * redundant.
	 */
	switch (flags & K_MEM_CACHE_MASK) {
	case K_MEM_CACHE_NONE:
		entry_flags |= MMU_PCD;
		break;
	case K_MEM_CACHE_WT:
		entry_flags |= MMU_PWT;
		break;
	case K_MEM_CACHE_WB:
		break;
	default:
		__ASSERT(false, "bad memory mapping flags 0x%x", flags);
	}

	if ((flags & K_MEM_PERM_RW) != 0U) {
		entry_flags |= ENTRY_RW;
	}

	if ((flags & K_MEM_PERM_USER) != 0U) {
		entry_flags |= ENTRY_US;
	}

	if ((flags & K_MEM_PERM_EXEC) == 0U) {
		entry_flags |= ENTRY_XD;
	}

	return entry_flags;
}

/* map new region virt..virt+size to phys with provided arch-neutral flags */
__pinned_func
void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	int ret;

	ret = range_map_unlocked(virt, phys, size, flags_to_entry(flags),
				 MASK_ALL, 0);
	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);
}

/* unmap region addr..addr+size, reset entries and flush TLB */
void arch_mem_unmap(void *addr, size_t size)
{
	int ret;

	ret = range_map_unlocked((void *)addr, 0, size, 0, 0,
				 OPTION_FLUSH | OPTION_CLEAR);
	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);
}

#ifdef Z_VM_KERNEL
__boot_func
static void identity_map_remove(uint32_t level)
{
	size_t size, scope = get_entry_scope(level);
	pentry_t *table;
	uint32_t cur_level;
	uint8_t *pos;
	pentry_t entry;
	pentry_t *entry_ptr;

	k_mem_region_align((uintptr_t *)&pos, &size,
			   (uintptr_t)CONFIG_SRAM_BASE_ADDRESS,
			   (size_t)CONFIG_SRAM_SIZE * 1024U, scope);

	while (size != 0U) {
		/* Need to get to the correct table */
		table = z_x86_kernel_ptables;
		for (cur_level = 0; cur_level < level; cur_level++) {
			entry = get_entry(table, pos, cur_level);
			table = next_table(entry, level);
		}

		entry_ptr = get_entry_ptr(table, pos, level);

		/* set_pte */
		*entry_ptr = 0;
		pos += scope;
		size -= scope;
	}
}
#endif

/* Invoked to remove the identity mappings in the page tables,
 * they were only needed to transition the instruction pointer at early boot
 */
__boot_func
void z_x86_mmu_init(void)
{
#ifdef Z_VM_KERNEL
	/* We booted with physical address space being identity mapped.
	 * As we are now executing in virtual address space,
	 * the identity map is no longer needed. So remove them.
	 *
	 * Without PAE, only need to remove the entries at the PD level.
	 * With PAE, need to also remove the entry at PDP level.
	 */
	identity_map_remove(PDE_LEVEL);

#ifdef CONFIG_X86_PAE
	identity_map_remove(0);
#endif
#endif
}

#ifdef CONFIG_X86_STACK_PROTECTION
__pinned_func
void z_x86_set_stack_guard(k_thread_stack_t *stack)
{
	int ret;

	/* Applied to all page tables as this affects supervisor mode.
	 * XXX: This never gets reset when the thread exits, which can
	 * cause problems if the memory is later used for something else.
	 * See #29499
	 *
	 * Guard page is always the first page of the stack object for both
	 * kernel and thread stacks.
	 */
	ret = range_map_unlocked(stack, 0, CONFIG_MMU_PAGE_SIZE,
				 MMU_P | ENTRY_XD, MASK_PERM, OPTION_FLUSH);
	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);
}
#endif /* CONFIG_X86_STACK_PROTECTION */

#ifdef CONFIG_USERSPACE
__pinned_func
static bool page_validate(pentry_t *ptables, uint8_t *addr, bool write)
{
	pentry_t *table = (pentry_t *)ptables;

	for (int level = 0; level < NUM_LEVELS; level++) {
		pentry_t entry = get_entry(table, addr, level);

		if (is_leaf(level, entry)) {
#ifdef CONFIG_X86_KPTI
			if (is_flipped_pte(entry)) {
				/* We flipped this to prevent user access
				 * since just clearing US isn't sufficient
				 */
				return false;
			}
#endif
			/* US and RW bits still carry meaning if non-present.
			 * If the data page is paged out, access bits are
			 * preserved. If un-mapped, the whole entry is 0.
			 */
			if (((entry & MMU_US) == 0U) ||
			    (write && ((entry & MMU_RW) == 0U))) {
				return false;
			}
		} else {
			if ((entry & MMU_P) == 0U) {
				/* Missing intermediate table, address is
				 * un-mapped
				 */
				return false;
			}
			table = next_table(entry, level);
		}
	}

	return true;
}

__pinned_func
static inline void bcb_fence(void)
{
#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	__asm__ volatile ("lfence" : : : "memory");
#endif
}

__pinned_func
int arch_buffer_validate(void *addr, size_t size, int write)
{
	pentry_t *ptables = z_x86_thread_page_tables_get(_current);
	uint8_t *virt;
	size_t aligned_size;
	int ret = 0;

	/* addr/size arbitrary, fix this up into an aligned region */
	(void)k_mem_region_align((uintptr_t *)&virt, &aligned_size,
				 (uintptr_t)addr, size, CONFIG_MMU_PAGE_SIZE);

	for (size_t offset = 0; offset < aligned_size;
	     offset += CONFIG_MMU_PAGE_SIZE) {
		if (!page_validate(ptables, virt + offset, write)) {
			ret = -1;
			break;
		}
	}

	bcb_fence();

	return ret;
}
#ifdef CONFIG_X86_COMMON_PAGE_TABLE
/* Very low memory configuration. A single set of page tables is used for
 * all threads. This relies on some assumptions:
 *
 * - No KPTI. If that were supported, we would need both a kernel and user
 *   set of page tables.
 * - No SMP. If that were supported, we would need per-core page tables.
 * - Memory domains don't affect supervisor mode.
 * - All threads have the same virtual-to-physical mappings.
 * - Memory domain APIs can't be called by user mode.
 *
 * Because there is no SMP, only one set of page tables, and user threads can't
 * modify their own memory domains, we don't have to do much when
 * arch_mem_domain_* APIs are called. We do use a caching scheme to avoid
 * updating page tables if the last user thread scheduled was in the same
 * domain.
 *
 * We don't set CONFIG_ARCH_MEM_DOMAIN_DATA, since we aren't setting
 * up any arch-specific memory domain data (per domain page tables.)
 *
 * This is all nice and simple and saves a lot of memory. The cost is that
 * context switching is not trivial CR3 update. We have to reset all partitions
 * for the current domain configuration and then apply all the partitions for
 * the incoming thread's domain if they are not the same. We also need to
 * update permissions similarly on the thread stack region.
 */

__pinned_func
static inline int reset_region(uintptr_t start, size_t size)
{
	return range_map_unlocked((void *)start, 0, size, 0, 0,
				  OPTION_FLUSH | OPTION_RESET);
}

__pinned_func
static inline int apply_region(uintptr_t start, size_t size, pentry_t attr)
{
	return range_map_unlocked((void *)start, 0, size, attr, MASK_PERM,
				  OPTION_FLUSH);
}

/* Cache of the current memory domain applied to the common page tables and
 * the stack buffer region that had User access granted.
 */
static __pinned_bss struct k_mem_domain *current_domain;
static __pinned_bss uintptr_t current_stack_start;
static __pinned_bss size_t current_stack_size;

__pinned_func
void z_x86_swap_update_common_page_table(struct k_thread *incoming)
{
	k_spinlock_key_t key;

	if ((incoming->base.user_options & K_USER) == 0) {
		/* Incoming thread is not a user thread. Memory domains don't
		 * affect supervisor threads and we don't need to enable User
		 * bits for its stack buffer; do nothing.
		 */
		return;
	}

	/* Step 1: Make sure the thread stack is set up correctly for the
	 * for the incoming thread
	 */
	if (incoming->stack_info.start != current_stack_start ||
	    incoming->stack_info.size != current_stack_size) {
		if (current_stack_size != 0U) {
			reset_region(current_stack_start, current_stack_size);
		}

		/* The incoming thread's stack region needs User permissions */
		apply_region(incoming->stack_info.start,
			     incoming->stack_info.size,
			     K_MEM_PARTITION_P_RW_U_RW);

		/* Update cache */
		current_stack_start = incoming->stack_info.start;
		current_stack_size = incoming->stack_info.size;
	}

	/* Step 2: The page tables always have some memory domain applied to
	 * them. If the incoming thread's memory domain is different,
	 * update the page tables
	 */
	key = k_spin_lock(&z_mem_domain_lock);
	if (incoming->mem_domain_info.mem_domain == current_domain) {
		/* The incoming thread's domain is already applied */
		goto out_unlock;
	}

	/* Reset the current memory domain regions... */
	if (current_domain != NULL) {
		for (int i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
			struct k_mem_partition *ptn =
				&current_domain->partitions[i];

			if (ptn->size == 0) {
				continue;
			}
			reset_region(ptn->start, ptn->size);
		}
	}

	/* ...and apply all the incoming domain's regions */
	for (int i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
		struct k_mem_partition *ptn =
			&incoming->mem_domain_info.mem_domain->partitions[i];

		if (ptn->size == 0) {
			continue;
		}
		apply_region(ptn->start, ptn->size, ptn->attr);
	}
	current_domain = incoming->mem_domain_info.mem_domain;
out_unlock:
	k_spin_unlock(&z_mem_domain_lock, key);
}

/* If a partition was added or removed in the cached domain, update the
 * page tables.
 */
__pinned_func
int arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				      uint32_t partition_id)
{
	struct k_mem_partition *ptn;

	if (domain != current_domain) {
		return 0;
	}

	ptn = &domain->partitions[partition_id];

	return reset_region(ptn->start, ptn->size);
}

__pinned_func
int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				   uint32_t partition_id)
{
	struct k_mem_partition *ptn;

	if (domain != current_domain) {
		return 0;
	}

	ptn = &domain->partitions[partition_id];

	return apply_region(ptn->start, ptn->size, ptn->attr);
}

/* Rest of the APIs don't need to do anything */
__pinned_func
int arch_mem_domain_thread_add(struct k_thread *thread)
{
	return 0;
}

__pinned_func
int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	return 0;
}
#else
/* Memory domains each have a set of page tables assigned to them */

/*
 * Pool of free memory pages for copying page tables, as needed.
 */
#define PTABLE_COPY_SIZE	(INITIAL_PTABLE_PAGES * CONFIG_MMU_PAGE_SIZE)

static uint8_t __pinned_noinit
	page_pool[PTABLE_COPY_SIZE * CONFIG_X86_MAX_ADDITIONAL_MEM_DOMAINS]
	__aligned(CONFIG_MMU_PAGE_SIZE);

__pinned_data
static uint8_t *page_pos = page_pool + sizeof(page_pool);

/* Return a zeroed and suitably aligned memory page for page table data
 * from the global page pool
 */
__pinned_func
static void *page_pool_get(void)
{
	void *ret;

	if (page_pos == page_pool) {
		ret = NULL;
	} else {
		page_pos -= CONFIG_MMU_PAGE_SIZE;
		ret = page_pos;
	}

	if (ret != NULL) {
		memset(ret, 0, CONFIG_MMU_PAGE_SIZE);
	}

	return ret;
}

/* Debugging function to show how many pages are free in the pool */
__pinned_func
static inline unsigned int pages_free(void)
{
	return (page_pos - page_pool) / CONFIG_MMU_PAGE_SIZE;
}

/**
*  Duplicate an entire set of page tables
 *
 * Uses recursion, but depth at any given moment is limited by the number of
 * paging levels.
 *
 * x86_mmu_lock must be held.
 *
 * @param dst a zeroed out chunk of memory of sufficient size for the indicated
 *            paging level.
 * @param src some paging structure from within the source page tables to copy
 *            at the indicated paging level
 * @param level Current paging level
 * @retval 0 Success
 * @retval -ENOMEM Insufficient page pool memory
 */
__pinned_func
static int copy_page_table(pentry_t *dst, pentry_t *src, int level)
{
	if (level == PTE_LEVEL) {
		/* Base case: leaf page table */
		for (int i = 0; i < get_num_entries(level); i++) {
			dst[i] = pte_finalize_value(reset_pte(src[i]), true,
						    PTE_LEVEL);
		}
	} else {
		/* Recursive case: allocate sub-structures as needed and
		 * make recursive calls on them
		 */
		for (int i = 0; i < get_num_entries(level); i++) {
			pentry_t *child_dst;
			int ret;

			if ((src[i] & MMU_P) == 0) {
				/* Non-present, skip */
				continue;
			}

			if ((level == PDE_LEVEL) && ((src[i] & MMU_PS) != 0)) {
				/* large page: no lower level table */
				dst[i] = pte_finalize_value(src[i], true,
							    PDE_LEVEL);
				continue;
			}

			__ASSERT((src[i] & MMU_PS) == 0,
				 "large page encountered");

			child_dst = page_pool_get();
			if (child_dst == NULL) {
				return -ENOMEM;
			}

			/* Page table links are by physical address. RAM
			 * for page tables is identity-mapped, but double-
			 * cast needed for PAE case where sizeof(void *) and
			 * sizeof(pentry_t) are not the same.
			 */
			dst[i] = ((pentry_t)z_mem_phys_addr(child_dst) |
				  INT_FLAGS);

			ret = copy_page_table(child_dst,
					      next_table(src[i], level),
					      level + 1);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

__pinned_func
static int region_map_update(pentry_t *ptables, void *start,
			      size_t size, pentry_t flags, bool reset)
{
	uint32_t options = OPTION_USER;
	int ret;
	k_spinlock_key_t key;

	if (reset) {
		options |= OPTION_RESET;
	}
	if (ptables == z_x86_page_tables_get()) {
		options |= OPTION_FLUSH;
	}

	key = k_spin_lock(&x86_mmu_lock);
	ret = range_map_ptables(ptables, start, 0, size, flags, MASK_PERM,
				options);
	k_spin_unlock(&x86_mmu_lock, key);

#ifdef CONFIG_SMP
	tlb_shootdown();
#endif

	return ret;
}

__pinned_func
static inline int reset_region(pentry_t *ptables, void *start, size_t size)
{
	LOG_DBG("%s(%p, %p, %zu)", __func__, ptables, start, size);
	return region_map_update(ptables, start, size, 0, true);
}

__pinned_func
static inline int apply_region(pentry_t *ptables, void *start,
				size_t size, pentry_t attr)
{
	LOG_DBG("%s(%p, %p, %zu, " PRI_ENTRY ")", __func__, ptables, start,
		size, attr);
	return region_map_update(ptables, start, size, attr, false);
}

__pinned_func
static void set_stack_perms(struct k_thread *thread, pentry_t *ptables)
{
	LOG_DBG("update stack for thread %p's ptables at %p: %p (size %zu)",
		thread, ptables, (void *)thread->stack_info.start,
		thread->stack_info.size);
	apply_region(ptables, (void *)thread->stack_info.start,
		     thread->stack_info.size,
		     MMU_P | MMU_XD | MMU_RW | MMU_US);
}

/*
 * Arch interface implementations for memory domains and userspace
 */

__boot_func
int arch_mem_domain_init(struct k_mem_domain *domain)
{
	int ret;
	k_spinlock_key_t key  = k_spin_lock(&x86_mmu_lock);

	LOG_DBG("%s(%p)", __func__, domain);
#if __ASSERT_ON
	sys_snode_t *node;

	/* Assert that we have not already initialized this domain */
	SYS_SLIST_FOR_EACH_NODE(&x86_domain_list, node) {
		struct arch_mem_domain *list_domain =
			CONTAINER_OF(node, struct arch_mem_domain, node);

		__ASSERT(list_domain != &domain->arch,
			 "%s(%p) called multiple times", __func__, domain);
	}
#endif /* __ASSERT_ON */
#ifndef CONFIG_X86_KPTI
	/* If we're not using KPTI then we can use the build time page tables
	 * (which are mutable) as the set of page tables for the default
	 * memory domain, saving us some memory.
	 *
	 * We skip adding this domain to x86_domain_list since we already
	 * update z_x86_kernel_ptables directly in range_map().
	 */
	if (domain == &k_mem_domain_default) {
		domain->arch.ptables = z_x86_kernel_ptables;
		k_spin_unlock(&x86_mmu_lock, key);
		return 0;
	}
#endif /* CONFIG_X86_KPTI */
#ifdef CONFIG_X86_PAE
	/* PDPT is stored within the memory domain itself since it is
	 * much smaller than a full page
	 */
	(void)memset(domain->arch.pdpt, 0, sizeof(domain->arch.pdpt));
	domain->arch.ptables = domain->arch.pdpt;
#else
	/* Allocate a page-sized top-level structure, either a PD or PML4 */
	domain->arch.ptables = page_pool_get();
	if (domain->arch.ptables == NULL) {
		k_spin_unlock(&x86_mmu_lock, key);
		return -ENOMEM;
	}
#endif /* CONFIG_X86_PAE */

	LOG_DBG("copy_page_table(%p, %p, 0)", domain->arch.ptables,
		z_x86_kernel_ptables);

	/* Make a copy of the boot page tables created by gen_mmu.py */
	ret = copy_page_table(domain->arch.ptables, z_x86_kernel_ptables, 0);
	if (ret == 0) {
		sys_slist_append(&x86_domain_list, &domain->arch.node);
	}
	k_spin_unlock(&x86_mmu_lock, key);

	return ret;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				     uint32_t partition_id)
{
	struct k_mem_partition *partition = &domain->partitions[partition_id];

	/* Reset the partition's region back to defaults */
	return reset_region(domain->arch.ptables, (void *)partition->start,
			    partition->size);
}

/* Called on thread exit or when moving it to a different memory domain */
int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;

	if ((thread->base.user_options & K_USER) == 0) {
		return 0;
	}

	if ((thread->base.thread_state & _THREAD_DEAD) == 0) {
		/* Thread is migrating to another memory domain and not
		 * exiting for good; we weren't called from
		 * z_thread_abort().  Resetting the stack region will
		 * take place in the forthcoming thread_add() call.
		 */
		return 0;
	}

	/* Restore permissions on the thread's stack area since it is no
	 * longer a member of the domain.
	 */
	return reset_region(domain->arch.ptables,
			    (void *)thread->stack_info.start,
			    thread->stack_info.size);
}

__pinned_func
int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				   uint32_t partition_id)
{
	struct k_mem_partition *partition = &domain->partitions[partition_id];

	/* Update the page tables with the partition info */
	return apply_region(domain->arch.ptables, (void *)partition->start,
			    partition->size, partition->attr | MMU_P);
}

/* Invoked from memory domain API calls, as well as during thread creation */
__pinned_func
int arch_mem_domain_thread_add(struct k_thread *thread)
{
	int ret = 0;

	/* New memory domain we are being added to */
	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;
	/* This is only set for threads that were migrating from some other
	 * memory domain; new threads this is NULL.
	 *
	 * Note that NULL check on old_ptables must be done before any
	 * address translation or else (NULL + offset) != NULL.
	 */
	pentry_t *old_ptables = UINT_TO_POINTER(thread->arch.ptables);
	bool is_user = (thread->base.user_options & K_USER) != 0;
	bool is_migration = (old_ptables != NULL) && is_user;

	/* Allow US access to the thread's stack in its new domain if
	 * we are migrating. If we are not migrating this is done in
	 * z_x86_current_stack_perms()
	 */
	if (is_migration) {
		old_ptables = z_mem_virt_addr(thread->arch.ptables);
		set_stack_perms(thread, domain->arch.ptables);
	}

	thread->arch.ptables = z_mem_phys_addr(domain->arch.ptables);
	LOG_DBG("set thread %p page tables to %p", thread,
		(void *)thread->arch.ptables);

	/* Check if we're doing a migration from a different memory domain
	 * and have to remove permissions from its old domain.
	 *
	 * XXX: The checks we have to do here and in
	 * arch_mem_domain_thread_remove() are clumsy, it may be worth looking
	 * into adding a specific arch_mem_domain_thread_migrate() API.
	 * See #29601
	 */
	if (is_migration) {
		ret = reset_region(old_ptables,
				   (void *)thread->stack_info.start,
				   thread->stack_info.size);
	}

#if !defined(CONFIG_X86_KPTI) && !defined(CONFIG_X86_COMMON_PAGE_TABLE)
	/* Need to switch to using these new page tables, in case we drop
	 * to user mode before we are ever context switched out.
	 * IPI takes care of this if the thread is currently running on some
	 * other CPU.
	 */
	if (thread == _current && thread->arch.ptables != z_x86_cr3_get()) {
		z_x86_cr3_set(thread->arch.ptables);
	}
#endif /* CONFIG_X86_KPTI */

	return ret;
}
#endif /* !CONFIG_X86_COMMON_PAGE_TABLE */

__pinned_func
int arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

/* Invoked from z_x86_userspace_enter */
__pinned_func
void z_x86_current_stack_perms(void)
{
	/* Clear any previous context in the stack buffer to prevent
	 * unintentional data leakage.
	 */
	(void)memset((void *)_current->stack_info.start, 0xAA,
		     _current->stack_info.size - _current->stack_info.delta);

	/* Only now is it safe to grant access to the stack buffer since any
	 * previous context has been erased.
	 */
#ifdef CONFIG_X86_COMMON_PAGE_TABLE
	/* Re run swap page table update logic since we're entering User mode.
	 * This will grant stack and memory domain access if it wasn't set
	 * already (in which case this returns very quickly).
	 */
	z_x86_swap_update_common_page_table(_current);
#else
	/* Memory domain access is already programmed into the page tables.
	 * Need to enable access to this new user thread's stack buffer in
	 * its domain-specific page tables.
	 */
	set_stack_perms(_current, z_x86_thread_page_tables_get(_current));
#endif
}
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES
__boot_func
static void mark_addr_page_reserved(uintptr_t addr, size_t len)
{
	uintptr_t pos = ROUND_DOWN(addr, CONFIG_MMU_PAGE_SIZE);
	uintptr_t end = ROUND_UP(addr + len, CONFIG_MMU_PAGE_SIZE);

	for (; pos < end; pos += CONFIG_MMU_PAGE_SIZE) {
		if (!z_is_page_frame(pos)) {
			continue;
		}

		struct z_page_frame *pf = z_phys_to_page_frame(pos);

		pf->flags |= Z_PAGE_FRAME_RESERVED;
	}
}

__boot_func
void arch_reserved_pages_update(void)
{
#ifdef CONFIG_X86_PC_COMPATIBLE
	/*
	 * Best is to do some E820 or similar enumeration to specifically
	 * identify all page frames which are reserved by the hardware or
	 * firmware. Or use x86_memmap[] with Multiboot if available.
	 *
	 * But still, reserve everything in the first megabyte of physical
	 * memory on PC-compatible platforms.
	 */
	mark_addr_page_reserved(0, MB(1));
#endif /* CONFIG_X86_PC_COMPATIBLE */

#ifdef CONFIG_X86_MEMMAP
	for (int i = 0; i < CONFIG_X86_MEMMAP_ENTRIES; i++) {
		struct x86_memmap_entry *entry = &x86_memmap[i];

		switch (entry->type) {
		case X86_MEMMAP_ENTRY_UNUSED:
			__fallthrough;
		case X86_MEMMAP_ENTRY_RAM:
			continue;

		case X86_MEMMAP_ENTRY_ACPI:
			__fallthrough;
		case X86_MEMMAP_ENTRY_NVS:
			__fallthrough;
		case X86_MEMMAP_ENTRY_DEFECTIVE:
			__fallthrough;
		default:
			/* If any of three above cases satisfied, exit switch
			 * and mark page reserved
			 */
			break;
		}

		mark_addr_page_reserved(entry->base, entry->length);
	}
#endif /* CONFIG_X86_MEMMAP */
}
#endif /* CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES */

int arch_page_phys_get(void *virt, uintptr_t *phys)
{
	pentry_t pte = 0;
	int level, ret;

	__ASSERT(POINTER_TO_UINT(virt) % CONFIG_MMU_PAGE_SIZE == 0U,
		 "unaligned address %p to %s", virt, __func__);

	pentry_get(&level, &pte, z_x86_page_tables_get(), virt);

	if ((pte & MMU_P) != 0) {
		if (phys != NULL) {
			*phys = (uintptr_t)get_entry_phys(pte, PTE_LEVEL);
		}
		ret = 0;
	} else {
		/* Not mapped */
		ret = -EFAULT;
	}

	return ret;
}

#ifdef CONFIG_DEMAND_PAGING
#define PTE_MASK (paging_levels[PTE_LEVEL].mask)

__pinned_func
void arch_mem_page_out(void *addr, uintptr_t location)
{
	int ret;
	pentry_t mask = PTE_MASK | MMU_P | MMU_A;

	/* Accessed bit set to guarantee the entry is not completely 0 in
	 * case of location value 0. A totally 0 PTE is un-mapped.
	 */
	ret = range_map(addr, location, CONFIG_MMU_PAGE_SIZE, MMU_A, mask,
			OPTION_FLUSH);
	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);
}

__pinned_func
void arch_mem_page_in(void *addr, uintptr_t phys)
{
	int ret;
	pentry_t mask = PTE_MASK | MMU_P | MMU_D | MMU_A;

	ret = range_map(addr, phys, CONFIG_MMU_PAGE_SIZE, MMU_P, mask,
			OPTION_FLUSH);
	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);
}

__pinned_func
void arch_mem_scratch(uintptr_t phys)
{
	page_map_set(z_x86_page_tables_get(), Z_SCRATCH_PAGE,
		     phys | MMU_P | MMU_RW | MMU_XD, NULL, MASK_ALL,
		     OPTION_FLUSH);
}

__pinned_func
uintptr_t arch_page_info_get(void *addr, uintptr_t *phys, bool clear_accessed)
{
	pentry_t all_pte, mask;
	uint32_t options;

	/* What to change, if anything, in the page_map_set() calls */
	if (clear_accessed) {
		mask = MMU_A;
		options = OPTION_FLUSH;
	} else {
		/* In this configuration page_map_set() just queries the
		 * page table and makes no changes
		 */
		mask = 0;
		options = 0U;
	}

	page_map_set(z_x86_kernel_ptables, addr, 0, &all_pte, mask, options);

	/* Un-mapped PTEs are completely zeroed. No need to report anything
	 * else in this case.
	 */
	if (all_pte == 0) {
		return ARCH_DATA_PAGE_NOT_MAPPED;
	}

#if defined(CONFIG_USERSPACE) && !defined(CONFIG_X86_COMMON_PAGE_TABLE)
	/* Don't bother looking at other page tables if non-present as we
	 * are not required to report accurate accessed/dirty in this case
	 * and all mappings are otherwise the same.
	 */
	if ((all_pte & MMU_P) != 0) {
		sys_snode_t *node;

		/* IRQs are locked, safe to do this */
		SYS_SLIST_FOR_EACH_NODE(&x86_domain_list, node) {
			pentry_t cur_pte;
			struct arch_mem_domain *domain =
				CONTAINER_OF(node, struct arch_mem_domain,
					     node);

			page_map_set(domain->ptables, addr, 0, &cur_pte,
				     mask, options | OPTION_USER);

			/* Logical OR of relevant PTE in all page tables.
			 * addr/location and present state should be identical
			 * among them.
			 */
			all_pte |= cur_pte;
		}
	}
#endif /* USERSPACE && ~X86_COMMON_PAGE_TABLE */

	/* NOTE: We are truncating the PTE on PAE systems, whose pentry_t
	 * are larger than a uintptr_t.
	 *
	 * We currently aren't required to report back XD state (bit 63), and
	 * Zephyr just doesn't support large physical memory on 32-bit
	 * systems, PAE was only implemented for XD support.
	 */
	if (phys != NULL) {
		*phys = (uintptr_t)get_entry_phys(all_pte, PTE_LEVEL);
	}

	/* We don't filter out any other bits in the PTE and the kernel
	 * ignores them. For the case of ARCH_DATA_PAGE_NOT_MAPPED,
	 * we use a bit which is never set in a real PTE (the PAT bit) in the
	 * current system.
	 *
	 * The other ARCH_DATA_PAGE_* macros are defined to their corresponding
	 * bits in the PTE.
	 */
	return (uintptr_t)all_pte;
}

__pinned_func
enum arch_page_location arch_page_location_get(void *addr, uintptr_t *location)
{
	pentry_t pte;
	int level;

	/* TODO: since we only have to query the current set of page tables,
	 * could optimize this with recursive page table mapping
	 */
	pentry_get(&level, &pte, z_x86_page_tables_get(), addr);

	if (pte == 0) {
		/* Not mapped */
		return ARCH_PAGE_LOCATION_BAD;
	}

	__ASSERT(level == PTE_LEVEL, "bigpage found at %p", addr);
	*location = (uintptr_t)get_entry_phys(pte, PTE_LEVEL);

	if ((pte & MMU_P) != 0) {
		return ARCH_PAGE_LOCATION_PAGED_IN;
	} else {
		return ARCH_PAGE_LOCATION_PAGED_OUT;
	}
}

#ifdef CONFIG_X86_KPTI
__pinned_func
bool z_x86_kpti_is_access_ok(void *addr, pentry_t *ptables)
{
	pentry_t pte;
	int level;

	pentry_get(&level, &pte, ptables, addr);

	/* Might as well also check if it's un-mapped, normally we don't
	 * fetch the PTE from the page tables until we are inside
	 * z_page_fault() and call arch_page_fault_status_get()
	 */
	if (level != PTE_LEVEL || pte == 0 || is_flipped_pte(pte)) {
		return false;
	}

	return true;
}
#endif /* CONFIG_X86_KPTI */
#endif /* CONFIG_DEMAND_PAGING */
