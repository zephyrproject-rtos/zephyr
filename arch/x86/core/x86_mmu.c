/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <arch/x86/mmustructs.h>
#include <linker/linker-defs.h>
#include <kernel_internal.h>
#include <kernel_structs.h>
#include <init.h>
#include <ctype.h>
#include <string.h>

/* Despite our use of PAE page tables, we do not (and will never) actually
 * support PAE. Use a 64-bit x86 target if you have that much RAM.
 */
BUILD_ASSERT(DT_PHYS_RAM_ADDR + (DT_RAM_SIZE * 1024ULL) - 1ULL <=
				 (unsigned long long)UINTPTR_MAX);

/* Common regions for all x86 processors.
 * Peripheral I/O ranges configured at the SOC level
 */

/* Mark text and rodata as read-only.
 * Userspace may read all text and rodata.
 */
MMU_BOOT_REGION(&_image_text_start, &_image_text_size,
		Z_X86_MMU_US);

MMU_BOOT_REGION(&_image_rodata_start, &_image_rodata_size,
		Z_X86_MMU_US | Z_X86_MMU_XD);

#ifdef CONFIG_USERSPACE
MMU_BOOT_REGION(&_app_smem_start, &_app_smem_size,
		Z_X86_MMU_RW | Z_X86_MMU_XD);
#endif

#ifdef CONFIG_COVERAGE_GCOV
MMU_BOOT_REGION(&__gcov_bss_start, &__gcov_bss_size,
		Z_X86_MMU_RW | Z_X86_MMU_US | Z_X86_MMU_XD);
#endif

#ifdef CONFIG_X86_64
extern char _locore_start[];
extern char _locore_size[];
extern char _lorodata_start[];
extern char _lorodata_size[];
extern char _lodata_start[];
extern char _lodata_size[];

/* Early boot regions that need to be in low memory to be comprehensible
 * by the CPU in 16-bit mode
 */

MMU_BOOT_REGION(&_locore_start, &_locore_size, 0);
MMU_BOOT_REGION(&_lorodata_start, &_lorodata_size, Z_X86_MMU_XD);
MMU_BOOT_REGION(&_lodata_start, &_lodata_size, Z_X86_MMU_RW | Z_X86_MMU_XD);
#endif

/* __kernel_ram_size includes all unused memory, which is used for heaps.
 * User threads cannot access this unless granted at runtime. This is done
 * automatically for stacks.
 */
MMU_BOOT_REGION(&__kernel_ram_start, &__kernel_ram_size,
		Z_X86_MMU_RW | Z_X86_MMU_XD);

/*
 * Inline functions for setting memory addresses in page table structures
 */

#ifdef CONFIG_X86_64
static inline void pml4e_update_pdpt(u64_t *pml4e, struct x86_mmu_pdpt *pdpt)
{
	uintptr_t pdpt_addr = (uintptr_t)pdpt;

	*pml4e = ((*pml4e & ~Z_X86_MMU_PML4E_PDPT_MASK) |
		  (pdpt_addr & Z_X86_MMU_PML4E_PDPT_MASK));
}
#endif /* CONFIG_X86_64 */

static inline void pdpte_update_pd(u64_t *pdpte, struct x86_mmu_pd *pd)
{
	uintptr_t pd_addr = (uintptr_t)pd;

#ifdef CONFIG_X86_64
	__ASSERT((*pdpte & Z_X86_MMU_PS) == 0, "PDPT is for 1GB page");
#endif
	*pdpte = ((*pdpte & ~Z_X86_MMU_PDPTE_PD_MASK) |
		  (pd_addr & Z_X86_MMU_PDPTE_PD_MASK));
}

static inline void pde_update_pt(u64_t *pde, struct x86_mmu_pt *pt)
{
	uintptr_t pt_addr = (uintptr_t)pt;

	__ASSERT((*pde & Z_X86_MMU_PS) == 0, "pde is for 2MB page");

	*pde = ((*pde & ~Z_X86_MMU_PDE_PT_MASK) |
		(pt_addr & Z_X86_MMU_PDE_PT_MASK));
}

static inline void pte_update_addr(u64_t *pte, uintptr_t addr)
{
	*pte = ((*pte & ~Z_X86_MMU_PTE_ADDR_MASK) |
		(addr & Z_X86_MMU_PTE_ADDR_MASK));
}

/*
 * Functions for dumping page tables to console
 */

/* Works for PDPT, PD, PT entries, the bits we check here are all the same.
 *
 * Not trying to capture every flag, just the most interesting stuff,
 * Present, write, XD, user, in typically encountered combinations.
 */
static char get_entry_code(u64_t value)
{
	char ret;

	if ((value & Z_X86_MMU_P) == 0) {
		ret = '.';
	} else {
		if ((value & Z_X86_MMU_RW) != 0) {
			/* Writable page */
			if ((value & Z_X86_MMU_XD) != 0) {
				/* RW */
				ret = 'w';
			} else {
				/* RWX */
				ret = 'a';
			}
		} else {
			if ((value & Z_X86_MMU_XD) != 0) {
				/* R */
				ret = 'r';
			} else {
				/* RX */
				ret = 'x';
			}
		}

		if ((value & Z_X86_MMU_US) != 0) {
			/* Uppercase indicates user mode access */
			ret = toupper(ret);
		}
	}

	return ret;
}

static void print_entries(u64_t entries_array[], size_t count)
{
	int column = 0;

	for (int i = 0; i < count; i++) {
		printk("%c", get_entry_code(entries_array[i]));

		column++;
		if (column == 64) {
			column = 0;
			printk("\n");
		}
	}

	if (column != 0) {
		printk("\n");
	}
}

static void z_x86_dump_pt(struct x86_mmu_pt *pt, uintptr_t base, int index)
{
	printk("Page table %d for 0x%016lX - 0x%016lX at %p\n",
	       index, base, base + Z_X86_PT_AREA - 1, pt);

	print_entries(pt->entry, Z_X86_NUM_PT_ENTRIES);
}

static void z_x86_dump_pd(struct x86_mmu_pd *pd, uintptr_t base, int index)
{
	printk("Page directory %d for 0x%016lX - 0x%016lX at %p\n",
	       index, base, base + Z_X86_PD_AREA - 1, pd);

	print_entries(pd->entry, Z_X86_NUM_PD_ENTRIES);

	for (int i = 0; i < Z_X86_NUM_PD_ENTRIES; i++) {
		struct x86_mmu_pt *pt;
		u64_t pde = pd->entry[i];

		if (((pde & Z_X86_MMU_P) == 0) || ((pde & Z_X86_MMU_PS) != 0)) {
			/* Skip non-present, or 2MB directory entries, there's
			 * no page table to examine */
			continue;
		}
		pt = z_x86_pde_get_pt(pde);

		z_x86_dump_pt(pt, base + (i * Z_X86_PT_AREA), i);
	}
}

static void z_x86_dump_pdpt(struct x86_mmu_pdpt *pdpt, uintptr_t base,
			    int index)
{
	printk("Page directory pointer table %d for 0x%0816lX - 0x%016lX at %p\n",
	       index, base, base + Z_X86_PDPT_AREA - 1, pdpt);

	print_entries(pdpt->entry, Z_X86_NUM_PDPT_ENTRIES);

	for (int i = 0; i < Z_X86_NUM_PDPT_ENTRIES; i++) {
		struct x86_mmu_pd *pd;
		u64_t pdpte = pdpt->entry[i];

		if ((pdpte & Z_X86_MMU_P) == 0) {
			continue;
		}
#ifdef CONFIG_X86_64
		if ((pdpte & Z_X86_MMU_PS) != 0) {
			continue;
		}
#endif
		pd = z_x86_pdpte_get_pd(pdpte);
		z_x86_dump_pd(pd, base + (i * Z_X86_PD_AREA), i);
	}
}

#ifdef CONFIG_X86_64
static void z_x86_dump_pml4(struct x86_mmu_pml4 *pml4)
{
	printk("Page mapping level 4 at %p for all memory addresses\n", pml4);

	print_entries(pml4->entry, Z_X86_NUM_PML4_ENTRIES);

	for (int i = 0; i < Z_X86_NUM_PML4_ENTRIES; i++) {
		struct x86_mmu_pdpt *pdpt;
		u64_t pml4e = pml4->entry[i];

		if ((pml4e & Z_X86_MMU_P) == 0) {
			continue;
		}

		pdpt = z_x86_pml4e_get_pdpt(pml4e);
		z_x86_dump_pdpt(pdpt, i * Z_X86_PDPT_AREA, i);
	}
}

void z_x86_dump_page_tables(struct x86_page_tables *ptables)
{
	z_x86_dump_pml4(z_x86_get_pml4(ptables));
}

#else
void z_x86_dump_page_tables(struct x86_page_tables *ptables)
{
	z_x86_dump_pdpt(z_x86_get_pdpt(ptables, 0), 0, 0);
}
#endif

void z_x86_mmu_get_flags(struct x86_page_tables *ptables, void *addr,
			 u64_t *pde_flags, u64_t *pte_flags)
{
	*pde_flags = *z_x86_get_pde(ptables, (uintptr_t)addr) &
		~Z_X86_MMU_PDE_PT_MASK;

	if ((*pde_flags & Z_X86_MMU_P) != 0) {
		*pte_flags = *z_x86_get_pte(ptables, (uintptr_t)addr) &
			~Z_X86_MMU_PTE_ADDR_MASK;
	} else {
		*pte_flags = 0;
	}
}

/* Given an address/size pair, which corresponds to some memory address
 * within a table of table_size, return the maximum number of bytes to
 * examine so we look just to the end of the table and no further.
 *
 * If size fits entirely within the table, just return size.
 */
static size_t get_table_max(uintptr_t addr, size_t size, size_t table_size)
{
	size_t table_remaining;

	addr &= (table_size - 1);
	table_remaining = table_size - addr;

	if (size < table_remaining) {
		return size;
	} else {
		return table_remaining;
	}
}

/* Range [addr, addr + size) must fall within the bounds of the pt */
static int x86_mmu_validate_pt(struct x86_mmu_pt *pt, uintptr_t addr,
			       size_t size, bool write)
{
	uintptr_t pos = addr;
	size_t remaining = size;
	int ret = 0;

	while (true) {
		u64_t pte = *z_x86_pt_get_pte(pt, pos);

		if ((pte & Z_X86_MMU_P) == 0 || (pte & Z_X86_MMU_US) == 0 ||
		    (write && (pte & Z_X86_MMU_RW) == 0)) {
			ret = -1;
			break;
		}

		if (remaining <= MMU_PAGE_SIZE) {
			break;
		}

		remaining -= MMU_PAGE_SIZE;
		pos += MMU_PAGE_SIZE;
	}

	return ret;
}

/* Range [addr, addr + size) must fall within the bounds of the pd */
static int x86_mmu_validate_pd(struct x86_mmu_pd *pd, uintptr_t addr,
			       size_t size, bool write)
{
	uintptr_t pos = addr;
	size_t remaining = size;
	int ret = 0;
	size_t to_examine;

	while (remaining) {
		u64_t pde = *z_x86_pd_get_pde(pd, pos);

		if ((pde & Z_X86_MMU_P) == 0 || (pde & Z_X86_MMU_US) == 0 ||
		    (write && (pde & Z_X86_MMU_RW) == 0)) {
			ret = -1;
			break;
		}

		to_examine = get_table_max(pos, remaining, Z_X86_PT_AREA);

		if ((pde & Z_X86_MMU_PS) == 0) {
			/* Not a 2MB PDE. Need to check all the linked
			 * tables for this entry
			 */
			struct x86_mmu_pt *pt;

			pt = z_x86_pde_get_pt(pde);
			ret = x86_mmu_validate_pt(pt, pos, to_examine, write);
			if (ret != 0) {
				break;
			}
		} else {
			ret = 0;
		}

		remaining -= to_examine;
		pos += to_examine;
	}

	return ret;
}

/* Range [addr, addr + size) must fall within the bounds of the pdpt */
static int x86_mmu_validate_pdpt(struct x86_mmu_pdpt *pdpt, uintptr_t addr,
				 size_t size, bool write)
{
	uintptr_t pos = addr;
	size_t remaining = size;
	int ret = 0;
	size_t to_examine;

	while (remaining) {
		u64_t pdpte = *z_x86_pdpt_get_pdpte(pdpt, pos);

		if ((pdpte & Z_X86_MMU_P) == 0) {
			/* Non-present */
			ret = -1;
			break;
		}

#ifdef CONFIG_X86_64
		if ((pdpte & Z_X86_MMU_US) == 0 ||
		    (write && (pdpte & Z_X86_MMU_RW) == 0)) {
			ret = -1;
			break;
		}
#endif
		to_examine = get_table_max(pos, remaining, Z_X86_PD_AREA);

#ifdef CONFIG_X86_64
		/* Check if 1GB page, if not, examine linked page directory */
		if ((pdpte & Z_X86_MMU_PS) == 0) {
#endif
			struct x86_mmu_pd *pd = z_x86_pdpte_get_pd(pdpte);

			ret = x86_mmu_validate_pd(pd, pos, to_examine, write);
			if (ret != 0) {
				break;
			}
#ifdef CONFIG_X86_64
		} else {
			ret = 0;
		}
#endif
		remaining -= to_examine;
		pos += to_examine;
	}

	return ret;
}

#ifdef CONFIG_X86_64
static int x86_mmu_validate_pml4(struct x86_mmu_pml4 *pml4, uintptr_t addr,
				 size_t size, bool write)
{
	uintptr_t pos = addr;
	size_t remaining = size;
	int ret = 0;
	size_t to_examine;

	while (remaining) {
		u64_t pml4e = *z_x86_pml4_get_pml4e(pml4, pos);
		struct x86_mmu_pdpt *pdpt;

		if ((pml4e & Z_X86_MMU_P) == 0 || (pml4e & Z_X86_MMU_US) == 0 ||
		    (write && (pml4e & Z_X86_MMU_RW) == 0)) {
			ret = -1;
			break;
		}

		to_examine = get_table_max(pos, remaining, Z_X86_PDPT_AREA);
		pdpt = z_x86_pml4e_get_pdpt(pml4e);

		ret = x86_mmu_validate_pdpt(pdpt, pos, to_examine, write);
		if (ret != 0) {
			break;
		}

		remaining -= to_examine;
		pos += to_examine;
	}

	return ret;
}
#endif /* CONFIG_X86_64 */

int z_x86_mmu_validate(struct x86_page_tables *ptables, void *addr, size_t size,
		       bool write)
{
	int ret;

#ifdef CONFIG_X86_64
	struct x86_mmu_pml4 *pml4 = z_x86_get_pml4(ptables);

	ret = x86_mmu_validate_pml4(pml4, (uintptr_t)addr, size, write);
#else
	struct x86_mmu_pdpt *pdpt = z_x86_get_pdpt(ptables, (uintptr_t)addr);

	ret = x86_mmu_validate_pdpt(pdpt, (uintptr_t)addr, size, write);
#endif

#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	__asm__ volatile ("lfence" : : : "memory");
#endif

	return ret;
}

static inline void tlb_flush_page(void *addr)
{
	/* Invalidate TLB entries corresponding to the page containing the
	 * specified address
	 */
	char *page = (char *)addr;

	__asm__ ("invlpg %0" :: "m" (*page));
}

#ifdef CONFIG_X86_64
#define PML4E_FLAGS_MASK	(Z_X86_MMU_RW | Z_X86_MMU_US | Z_X86_MMU_P)

#define PDPTE_FLAGS_MASK	PML4E_FLAGS_MASK

#define PDE_FLAGS_MASK		PDPTE_FLAGS_MASK
#else
#define PDPTE_FLAGS_MASK	Z_X86_MMU_P

#define PDE_FLAGS_MASK		(Z_X86_MMU_RW | Z_X86_MMU_US | \
				 PDPTE_FLAGS_MASK)
#endif

#define PTE_FLAGS_MASK		(PDE_FLAGS_MASK | Z_X86_MMU_XD | \
				 Z_X86_MMU_PWT | \
				 Z_X86_MMU_PCD)

void z_x86_mmu_set_flags(struct x86_page_tables *ptables, void *ptr,
			 size_t size, u64_t flags, u64_t mask, bool flush)
{
	uintptr_t addr = (uintptr_t)ptr;

	__ASSERT((addr & MMU_PAGE_MASK) == 0U, "unaligned address provided");
	__ASSERT((size & MMU_PAGE_MASK) == 0U, "unaligned size provided");

	/* L1TF mitigation: non-present PTEs will have address fields
	 * zeroed. Expand the mask to include address bits if we are changing
	 * the present bit.
	 */
	if ((mask & Z_X86_MMU_P) != 0) {
		mask |= Z_X86_MMU_PTE_ADDR_MASK;
	}

	/* NOTE: All of this code assumes that 2MB or 1GB pages are not being
	 * modified.
	 */
	while (size != 0) {
		u64_t *pte;
		u64_t *pde;
		u64_t *pdpte;
#ifdef CONFIG_X86_64
		u64_t *pml4e;
#endif
		u64_t cur_flags = flags;
		bool exec = (flags & Z_X86_MMU_XD) == 0;

#ifdef CONFIG_X86_64
		pml4e = z_x86_pml4_get_pml4e(z_x86_get_pml4(ptables), addr);
		__ASSERT((*pml4e & Z_X86_MMU_P) != 0,
			 "set flags on non-present PML4e");
		*pml4e |= (flags & PML4E_FLAGS_MASK);

		if (exec) {
			*pml4e &= ~Z_X86_MMU_XD;
		}

		pdpte = z_x86_pdpt_get_pdpte(z_x86_pml4e_get_pdpt(*pml4e),
					     addr);
#else
		pdpte = z_x86_pdpt_get_pdpte(z_x86_get_pdpt(ptables, addr),
					     addr);
#endif
		__ASSERT((*pdpte & Z_X86_MMU_P) != 0,
			 "set flags on non-present PDPTE");
		*pdpte |= (flags & PDPTE_FLAGS_MASK);
#ifdef CONFIG_X86_64
		if (exec) {
			*pdpte &= ~Z_X86_MMU_XD;
		}
#endif
		pde = z_x86_pd_get_pde(z_x86_pdpte_get_pd(*pdpte), addr);
		__ASSERT((*pde & Z_X86_MMU_P) != 0,
			 "set flags on non-present PDE");
		*pde |= (flags & PDE_FLAGS_MASK);

		/* If any flags enable execution, clear execute disable at the
		 * page directory level
		 */
		if (exec) {
			*pde &= ~Z_X86_MMU_XD;
		}

		pte = z_x86_pt_get_pte(z_x86_pde_get_pt(*pde), addr);

		/* If we're setting the present bit, restore the address
		 * field. If we're clearing it, then the address field
		 * will be zeroed instead, mapping the PTE to the NULL page.
		 */
		if ((mask & Z_X86_MMU_P) != 0 && ((flags & Z_X86_MMU_P) != 0)) {
			cur_flags |= addr;
		}

		*pte = (*pte & ~mask) | cur_flags;
		if (flush) {
			tlb_flush_page((void *)addr);
		}

		size -= MMU_PAGE_SIZE;
		addr += MMU_PAGE_SIZE;
	}
}

static char __aligned(MMU_PAGE_SIZE)
	page_pool[MMU_PAGE_SIZE * CONFIG_X86_MMU_PAGE_POOL_PAGES];

static char *page_pos = page_pool + sizeof(page_pool);

static void *get_page(void)
{
	page_pos -= MMU_PAGE_SIZE;

	__ASSERT(page_pos >= page_pool, "out of MMU pages\n");

	return page_pos;
}

#ifdef CONFIG_X86_64
#define PTABLES_ALIGN	4096
#else
#define PTABLES_ALIGN	32
#endif

__aligned(PTABLES_ALIGN) struct x86_page_tables z_x86_kernel_ptables;
#ifdef CONFIG_X86_KPTI
__aligned(PTABLES_ALIGN) struct x86_page_tables z_x86_user_ptables;
#endif

extern char z_shared_kernel_page_start[];

static inline bool is_within_system_ram(uintptr_t addr)
{
	return (addr >= DT_PHYS_RAM_ADDR) &&
		(addr < (DT_PHYS_RAM_ADDR + (DT_RAM_SIZE * 1024U)));
}

/* Ignored bit posiition at all levels */
#define IGNORED		BIT64(11)

static void maybe_clear_xd(u64_t *entry, bool exec)
{
	/* Execute disable bit needs special handling, we should only set it at
	 * intermediate levels if ALL containing pages have XD set (instead of
	 * just one).
	 *
	 * Use an ignored bit position in the PDE to store a marker on whether
	 * any configured region allows execution.
	 */
	if (exec) {
		*entry |= IGNORED;
		*entry &= ~Z_X86_MMU_XD;
	} else if ((*entry & IGNORED) == 0) {
		*entry |= Z_X86_MMU_XD;
	}
}

static void add_mmu_region_page(struct x86_page_tables *ptables,
				uintptr_t addr, u64_t flags, bool user_table)
{
#ifdef CONFIG_X86_64
	u64_t *pml4e;
#endif
	struct x86_mmu_pdpt *pdpt;
	u64_t *pdpte;
	struct x86_mmu_pd *pd;
	u64_t *pde;
	struct x86_mmu_pt *pt;
	u64_t *pte;
	bool exec = (flags & Z_X86_MMU_XD) == 0;

#ifdef CONFIG_X86_KPTI
	/* If we are generating a page table for user mode, and this address
	 * does not have the user flag set, and this address falls outside
	 * of system RAM, then don't bother generating any tables for it,
	 * we will never need them later as memory domains are limited to
	 * regions within system RAM.
	 */
	if (user_table && (flags & Z_X86_MMU_US) == 0 &&
	    !is_within_system_ram(addr)) {
		return;
	}
#endif

#ifdef CONFIG_X86_64
	pml4e = z_x86_pml4_get_pml4e(z_x86_get_pml4(ptables), addr);
	if ((*pml4e & Z_X86_MMU_P) == 0) {
		pdpt = get_page();
		pml4e_update_pdpt(pml4e, pdpt);
	} else {
		pdpt = z_x86_pml4e_get_pdpt(*pml4e);
	}
	*pml4e |= (flags & PML4E_FLAGS_MASK);
	maybe_clear_xd(pml4e, exec);
#else
	pdpt = z_x86_get_pdpt(ptables, addr);
#endif

	/* Setup the PDPTE entry for the address, creating a page directory
	 * if one didn't exist
	 */
	pdpte = z_x86_pdpt_get_pdpte(pdpt, addr);
	if ((*pdpte & Z_X86_MMU_P) == 0) {
		pd = get_page();
		pdpte_update_pd(pdpte, pd);
	} else {
		pd = z_x86_pdpte_get_pd(*pdpte);
	}
	*pdpte |= (flags & PDPTE_FLAGS_MASK);
#ifdef CONFIG_X86_64
	maybe_clear_xd(pdpte, exec);
#endif

	/* Setup the PDE entry for the address, creating a page table
	 * if necessary
	 */
	pde = z_x86_pd_get_pde(pd, addr);
	if ((*pde & Z_X86_MMU_P) == 0) {
		pt = get_page();
		pde_update_pt(pde, pt);
	} else {
		pt = z_x86_pde_get_pt(*pde);
	}
	*pde |= (flags & PDE_FLAGS_MASK);
	maybe_clear_xd(pde, exec);

#ifdef CONFIG_X86_KPTI
	if (user_table && (flags & Z_X86_MMU_US) == 0 &&
	    addr != (uintptr_t)(&z_shared_kernel_page_start)) {
		/* All non-user accessible pages except the shared page
		 * are marked non-present in the page table.
		 */
		return;
	}
#else
	ARG_UNUSED(user_table);
#endif

	/* Finally set up the page table entry */
	pte = z_x86_pt_get_pte(pt, addr);
	pte_update_addr(pte, addr);
	*pte |= (flags & PTE_FLAGS_MASK);
}

static void add_mmu_region(struct x86_page_tables *ptables,
			   struct mmu_region *rgn,
			   bool user_table)
{
	size_t size;
	u64_t flags;
	uintptr_t addr;

	__ASSERT((rgn->address & MMU_PAGE_MASK) == 0U,
		 "unaligned address provided");
	__ASSERT((rgn->size & MMU_PAGE_MASK) == 0U,
		 "unaligned size provided");
	addr = rgn->address;
	flags = rgn->flags | Z_X86_MMU_P;

	/* Iterate through the region a page at a time, creating entries as
	 * necessary.
	 */
	size = rgn->size;
	while (size > 0) {
		add_mmu_region_page(ptables, addr, flags, user_table);

		size -= MMU_PAGE_SIZE;
		addr += MMU_PAGE_SIZE;
	}
}

/* Called from x86's z_arch_kernel_init() */
void z_x86_paging_init(void)
{
	size_t pages_free;

	Z_STRUCT_SECTION_FOREACH(mmu_region, rgn) {
		add_mmu_region(&z_x86_kernel_ptables, rgn, false);
#ifdef CONFIG_X86_KPTI
		add_mmu_region(&z_x86_user_ptables, rgn, true);
#endif
	}

	pages_free = (page_pos - page_pool) / MMU_PAGE_SIZE;

	if (pages_free != 0) {
		printk("Optimal CONFIG_X86_MMU_PAGE_POOL_PAGES %zu\n",
		       CONFIG_X86_MMU_PAGE_POOL_PAGES - pages_free);
	}

#ifdef CONFIG_X86_64
	/* MMU already enabled at boot for long mode, we just need to
	 * program CR3 with our newly generated page tables.
	 */
	__asm__ volatile("movq %0, %%cr3\n\t"
			 : : "r" (&z_x86_kernel_ptables) : "memory");
#else
	z_x86_enable_paging();
#endif
}

#ifdef CONFIG_X86_USERSPACE
int z_arch_buffer_validate(void *addr, size_t size, int write)
{
	return z_x86_mmu_validate(z_x86_thread_page_tables_get(_current), addr,
				  size, write != 0);
}

static uintptr_t thread_pd_create(uintptr_t pages,
				  struct x86_page_tables *thread_ptables,
				  struct x86_page_tables *master_ptables)
{
	uintptr_t pos = pages, phys_addr = Z_X86_PD_START;

	for (int i = 0; i < Z_X86_NUM_PD; i++, phys_addr += Z_X86_PD_AREA) {
		u64_t *pdpte;
		struct x86_mmu_pd *master_pd, *dest_pd;

		/* Obtain PD in master tables for the address range and copy
		 * into the per-thread PD for this range
		 */
		master_pd = z_x86_get_pd(master_ptables, phys_addr);
		dest_pd = (struct x86_mmu_pd *)pos;

		(void)memcpy(dest_pd, master_pd, sizeof(struct x86_mmu_pd));

		/* Update pointer in per-thread pdpt to point to the per-thread
		 * directory we just copied
		 */
		pdpte = z_x86_get_pdpte(thread_ptables, phys_addr);
		pdpte_update_pd(pdpte, dest_pd);
		pos += MMU_PAGE_SIZE;
	}

	return pos;
}

/* thread_ptables must be initialized, as well as all the page directories */
static uintptr_t thread_pt_create(uintptr_t pages,
				  struct x86_page_tables *thread_ptables,
				  struct x86_page_tables *master_ptables)
{
	uintptr_t pos = pages, phys_addr = Z_X86_PT_START;

	for (int i = 0; i < Z_X86_NUM_PT; i++, phys_addr += Z_X86_PT_AREA) {
		u64_t *pde;
		struct x86_mmu_pt *master_pt, *dest_pt;

		/* Same as we did with the directories, obtain PT in master
		 * tables for the address range and copy into per-thread PT
		 * for this range
		 */
		master_pt = z_x86_get_pt(master_ptables, phys_addr);
		dest_pt = (struct x86_mmu_pt *)pos;
		(void)memcpy(dest_pt, master_pt, sizeof(struct x86_mmu_pt));

		/* And then wire this up to the relevant per-thread
		 * page directory entry
		 */
		pde = z_x86_get_pde(thread_ptables, phys_addr);
		pde_update_pt(pde, dest_pt);
		pos += MMU_PAGE_SIZE;
	}

	return pos;
}

/* Initialize the page tables for a thread. This will contain, once done,
 * the boot-time configuration for a user thread page tables. There are
 * no pre-conditions on the existing state of the per-thread tables.
 */
static void copy_page_tables(struct k_thread *thread,
			     struct x86_page_tables *master_ptables)
{
	uintptr_t pos, start;
	struct x86_page_tables *thread_ptables =
		z_x86_thread_page_tables_get(thread);
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)thread->stack_obj;

	__ASSERT(thread->stack_obj != NULL, "no stack object assigned");
	__ASSERT(z_x86_page_tables_get() != thread_ptables,
		 "tables are active");
	__ASSERT(((uintptr_t)thread_ptables & 0x1f) == 0,
		 "unaligned page tables at %p", thread_ptables);

	(void)memcpy(thread_ptables, master_ptables,
		     sizeof(struct x86_page_tables));

	/* pos represents the page we are working with in the reserved area
	 * in the stack buffer for per-thread tables. As we create tables in
	 * this area, pos is incremented to the next free page.
	 *
	 * The layout of the stack object, when this is done:
	 *
	 * +---------------------------+  <- thread->stack_obj
	 * | PDE(0)                    |
	 * +---------------------------+
	 * | ...                       |
	 * +---------------------------+
	 * | PDE(Z_X86_NUM_PD - 1)     |
	 * +---------------------------+
	 * | PTE(0)                    |
	 * +---------------------------+
	 * | ...                       |
	 * +---------------------------+
	 * | PTE(Z_X86_NUM_PT - 1)     |
	 * +---------------------------+ <- pos once this logic completes
	 * | Stack guard               |
	 * +---------------------------+
	 * | Privilege elevation stack |
	 * | PDPT                      |
	 * +---------------------------+ <- thread->stack_info.start
	 * | Thread stack              |
	 * | ...                       |
	 *
	 */
	start = (uintptr_t)(&header->page_tables);
	pos = thread_pd_create(start, thread_ptables, master_ptables);
	pos = thread_pt_create(pos, thread_ptables, master_ptables);

	__ASSERT(pos == (start + Z_X86_THREAD_PT_AREA),
		 "wrong amount of stack object memory used");
}

static void reset_mem_partition(struct x86_page_tables *thread_ptables,
				struct k_mem_partition *partition)
{
	uintptr_t addr = partition->start;
	size_t size = partition->size;

	__ASSERT((addr & MMU_PAGE_MASK) == 0U, "unaligned address provided");
	__ASSERT((size & MMU_PAGE_MASK) == 0U, "unaligned size provided");

	while (size != 0) {
		u64_t *thread_pte, *master_pte;

		thread_pte = z_x86_get_pte(thread_ptables, addr);
		master_pte = z_x86_get_pte(&USER_PTABLES, addr);

		*thread_pte = *master_pte;

		size -= MMU_PAGE_SIZE;
		addr += MMU_PAGE_SIZE;
	}
}

static void apply_mem_partition(struct x86_page_tables *ptables,
				struct k_mem_partition *partition)
{
	u64_t x86_attr;
	u64_t mask;

	if (IS_ENABLED(CONFIG_X86_KPTI)) {
		x86_attr = partition->attr | Z_X86_MMU_P;
		mask = K_MEM_PARTITION_PERM_MASK | Z_X86_MMU_P;
	} else {
		x86_attr = partition->attr;
		mask = K_MEM_PARTITION_PERM_MASK;
	}

	__ASSERT(partition->start >= DT_PHYS_RAM_ADDR,
		 "region at %08lx[%u] extends below system ram start 0x%08x",
		 partition->start, partition->size, DT_PHYS_RAM_ADDR);
	__ASSERT(((partition->start + partition->size) <=
		  (DT_PHYS_RAM_ADDR + (DT_RAM_SIZE * 1024U))),
		 "region at %08lx[%u] end at %08lx extends beyond system ram end 0x%08x",
		 partition->start, partition->size,
		 partition->start + partition->size,
		 (DT_PHYS_RAM_ADDR + (DT_RAM_SIZE * 1024U)));

	z_x86_mmu_set_flags(ptables, (void *)partition->start, partition->size,
			    x86_attr, mask, false);
}

void z_x86_apply_mem_domain(struct x86_page_tables *ptables,
			    struct k_mem_domain *mem_domain)
{
	for (int i = 0, pcount = 0; pcount < mem_domain->num_partitions; i++) {
		struct k_mem_partition *partition;

		partition = &mem_domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		apply_mem_partition(ptables, partition);
	}
}

/* Called on creation of a user thread or when a supervisor thread drops to
 * user mode.
 *
 * Sets up the per-thread page tables, such that when they are activated on
 * context switch, everything is ready to go.
 */
void z_x86_thread_pt_init(struct k_thread *thread)
{
	struct x86_page_tables *ptables = z_x86_thread_page_tables_get(thread);

	/* USER_PDPT contains the page tables with the boot time memory
	 * policy. We use it as a template to set up the per-thread page
	 * tables.
	 *
	 * With KPTI, this is a distinct set of tables z_x86_user_pdpt from the
	 * kernel page tables in z_x86_kernel_pdpt; it has all non user
	 * accessible pages except the trampoline page marked as non-present.
	 * Without KPTI, they are the same object.
	 */
	copy_page_tables(thread, &USER_PTABLES);

	/* Enable access to the thread's own stack buffer */
	z_x86_mmu_set_flags(ptables, (void *)thread->stack_info.start,
			    ROUND_UP(thread->stack_info.size, MMU_PAGE_SIZE),
			    Z_X86_MMU_P | K_MEM_PARTITION_P_RW_U_RW,
			    Z_X86_MMU_P | K_MEM_PARTITION_PERM_MASK,
			    false);
}

/*
 * Memory domain interface
 *
 * In all cases, if one of these APIs is called on a supervisor thread,
 * we don't need to do anything. If the thread later drops into supervisor
 * mode the per-thread page tables will be generated and the memory domain
 * configuration applied.
 */
void z_arch_mem_domain_partition_remove(struct k_mem_domain *domain,
					u32_t partition_id)
{
	sys_dnode_t *node, *next_node;

	/* Removing a partition. Need to reset the relevant memory range
	 * to the defaults in USER_PDPT for each thread.
	 */
	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		struct k_thread *thread =
			CONTAINER_OF(node, struct k_thread, mem_domain_info);

		if ((thread->base.user_options & K_USER) == 0) {
			continue;
		}

		reset_mem_partition(z_x86_thread_page_tables_get(thread),
				    &domain->partitions[partition_id]);
	}
}

void z_arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	for (int i = 0, pcount = 0; pcount < domain->num_partitions; i++) {
		struct k_mem_partition *partition;

		partition = &domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		z_arch_mem_domain_partition_remove(domain, i);
	}
}

void z_arch_mem_domain_thread_remove(struct k_thread *thread)
{
	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;

	/* Non-user threads don't have per-thread page tables set up */
	if ((thread->base.user_options & K_USER) == 0) {
		return;
	}

	for (int i = 0, pcount = 0; pcount < domain->num_partitions; i++) {
		struct k_mem_partition *partition;

		partition = &domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		reset_mem_partition(z_x86_thread_page_tables_get(thread),
				    partition);
	}
}

void z_arch_mem_domain_partition_add(struct k_mem_domain *domain,
				     u32_t partition_id)
{
	sys_dnode_t *node, *next_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		struct k_thread *thread =
			CONTAINER_OF(node, struct k_thread, mem_domain_info);

		if ((thread->base.user_options & K_USER) == 0) {
			continue;
		}

		apply_mem_partition(z_x86_thread_page_tables_get(thread),
				    &domain->partitions[partition_id]);
	}
}

void z_arch_mem_domain_thread_add(struct k_thread *thread)
{
	if ((thread->base.user_options & K_USER) == 0) {
		return;
	}

	z_x86_apply_mem_domain(z_x86_thread_page_tables_get(thread),
			       thread->mem_domain_info.mem_domain);
}

int z_arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}
#endif	/* CONFIG_X86_USERSPACE*/
