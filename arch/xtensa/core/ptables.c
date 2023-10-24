/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/arch/xtensa/xtensa_mmu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/toolchain.h>
#include <xtensa/corebits.h>
#include <xtensa_mmu_priv.h>

#include <kernel_arch_func.h>
#include <mmu.h>

/* Fixed data TLB way to map the page table */
#define MMU_PTE_WAY 7

/* Fixed data TLB way to map VECBASE */
#define MMU_VECBASE_WAY 8

/* Level 1 contains page table entries
 * necessary to map the page table itself.
 */
#define XTENSA_L1_PAGE_TABLE_ENTRIES 1024U

/* Level 2 contains page table entries
 * necessary to map the page table itself.
 */
#define XTENSA_L2_PAGE_TABLE_ENTRIES 1024U

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MMU_PAGE_SIZE == 0x1000,
	     "MMU_PAGE_SIZE value is invalid, only 4 kB pages are supported\n");

/*
 * Level 1 page table has to be 4Kb to fit into one of the wired entries.
 * All entries are initialized as INVALID, so an attempt to read an unmapped
 * area will cause a double exception.
 */
uint32_t l1_page_table[XTENSA_L1_PAGE_TABLE_ENTRIES] __aligned(KB(4));

/*
 * Each table in the level 2 maps a 4Mb memory range. It consists of 1024 entries each one
 * covering a 4Kb page.
 */
static uint32_t l2_page_tables[CONFIG_XTENSA_MMU_NUM_L2_TABLES][XTENSA_L2_PAGE_TABLE_ENTRIES]
				__aligned(KB(4));

/*
 * This additional variable tracks which l2 tables are in use. This is kept separated from
 * the tables to keep alignment easier.
 */
static ATOMIC_DEFINE(l2_page_tables_track, CONFIG_XTENSA_MMU_NUM_L2_TABLES);

extern char _heap_end[];
extern char _heap_start[];
extern char __data_start[];
extern char __data_end[];
extern char _bss_start[];
extern char _bss_end[];

/*
 * Static definition of all code & data memory regions of the
 * current Zephyr image. This information must be available &
 * processed upon MMU initialization.
 */

static const struct xtensa_mmu_range mmu_zephyr_ranges[] = {
	/*
	 * Mark the zephyr execution regions (data, bss, noinit, etc.)
	 * cacheable, read / write and non-executable
	 */
	{
		/* This includes .data, .bss and various kobject sections. */
		.start = (uint32_t)_image_ram_start,
		.end   = (uint32_t)_image_ram_end,
#ifdef CONFIG_XTENSA_RPO_CACHE
		.attrs = Z_XTENSA_MMU_W,
#else
		.attrs = Z_XTENSA_MMU_W | Z_XTENSA_MMU_CACHED_WB,
#endif
		.name = "data",
	},
	/* System heap memory */
	{
		.start = (uint32_t)_heap_start,
		.end   = (uint32_t)_heap_end,
#ifdef CONFIG_XTENSA_RPO_CACHE
		.attrs = Z_XTENSA_MMU_W,
#else
		.attrs = Z_XTENSA_MMU_W | Z_XTENSA_MMU_CACHED_WB,
#endif
		.name = "heap",
	},
	/* Mark text segment cacheable, read only and executable */
	{
		.start = (uint32_t)__text_region_start,
		.end   = (uint32_t)__text_region_end,
		.attrs = Z_XTENSA_MMU_X | Z_XTENSA_MMU_CACHED_WB,
		.name = "text",
	},
	/* Mark rodata segment cacheable, read only and non-executable */
	{
		.start = (uint32_t)__rodata_region_start,
		.end   = (uint32_t)__rodata_region_end,
		.attrs = Z_XTENSA_MMU_CACHED_WB,
		.name = "rodata",
	},
};

static inline uint32_t *alloc_l2_table(void)
{
	uint16_t idx;

	for (idx = 0; idx < CONFIG_XTENSA_MMU_NUM_L2_TABLES; idx++) {
		if (!atomic_test_and_set_bit(l2_page_tables_track, idx)) {
			return (uint32_t *)&l2_page_tables[idx];
		}
	}

	return NULL;
}

static void map_memory_range(const uint32_t start, const uint32_t end,
			     const uint32_t attrs)
{
	uint32_t page, *table;

	for (page = start; page < end; page += CONFIG_MMU_PAGE_SIZE) {
		uint32_t pte = Z_XTENSA_PTE(page, Z_XTENSA_KERNEL_RING, attrs);
		uint32_t l2_pos = Z_XTENSA_L2_POS(page);
		uint32_t l1_pos = page >> 22;

		if (l1_page_table[l1_pos] == Z_XTENSA_MMU_ILLEGAL) {
			table  = alloc_l2_table();

			__ASSERT(table != NULL, "There is no l2 page table available to "
				"map 0x%08x\n", page);

			l1_page_table[l1_pos] =
				Z_XTENSA_PTE((uint32_t)table, Z_XTENSA_KERNEL_RING,
						Z_XTENSA_MMU_CACHED_WT);
		}

		table = (uint32_t *)(l1_page_table[l1_pos] & Z_XTENSA_PTE_PPN_MASK);
		table[l2_pos] = pte;
	}
}

static void map_memory(const uint32_t start, const uint32_t end,
		       const uint32_t attrs)
{
	map_memory_range(start, end, attrs);

#ifdef CONFIG_XTENSA_MMU_DOUBLE_MAP
	if (arch_xtensa_is_ptr_uncached((void *)start)) {
		map_memory_range(POINTER_TO_UINT(z_soc_cached_ptr((void *)start)),
			POINTER_TO_UINT(z_soc_cached_ptr((void *)end)),
			attrs | Z_XTENSA_MMU_CACHED_WB);
	} else if (arch_xtensa_is_ptr_cached((void *)start)) {
		map_memory_range(POINTER_TO_UINT(z_soc_uncached_ptr((void *)start)),
			POINTER_TO_UINT(z_soc_uncached_ptr((void *)end)), attrs);
	}
#endif
}

static void xtensa_init_page_tables(void)
{
	volatile uint8_t entry;
	uint32_t page;

	for (page = 0; page < XTENSA_L1_PAGE_TABLE_ENTRIES; page++) {
		l1_page_table[page] = Z_XTENSA_MMU_ILLEGAL;
	}

	for (entry = 0; entry < ARRAY_SIZE(mmu_zephyr_ranges); entry++) {
		const struct xtensa_mmu_range *range = &mmu_zephyr_ranges[entry];

		map_memory(range->start, range->end, range->attrs);
	}

/**
 * GCC complains about usage of the SoC MMU range ARRAY_SIZE
 * (xtensa_soc_mmu_ranges) as the default weak declaration is
 * an empty array, and any access to its element is considered
 * out of bound access. However, we have a number of element
 * variable to guard against this (... if done correctly).
 * Besides, this will almost be overridden by the SoC layer.
 * So tell GCC to ignore this.
 */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
	for (entry = 0; entry < xtensa_soc_mmu_ranges_num; entry++) {
		const struct xtensa_mmu_range *range = &xtensa_soc_mmu_ranges[entry];

		map_memory(range->start, range->end, range->attrs);
	}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

	sys_cache_data_flush_all();
}

__weak void arch_xtensa_mmu_post_init(bool is_core0)
{
	ARG_UNUSED(is_core0);
}

static void xtensa_mmu_init(bool is_core0)
{
	volatile uint8_t entry;
	uint32_t ps, vecbase;

	if (is_core0) {
		/* This is normally done via arch_kernel_init() inside z_cstart().
		 * However, before that is called, we go through the sys_init of
		 * INIT_LEVEL_EARLY, which is going to result in TLB misses.
		 * So setup whatever necessary so the exception handler can work
		 * properly.
		 */
		z_xtensa_kernel_init();
		xtensa_init_page_tables();
	}

	/* Set the page table location in the virtual address */
	xtensa_ptevaddr_set((void *)Z_XTENSA_PTEVADDR);

	/* Next step is to invalidate the tlb entry that contains the top level
	 * page table. This way we don't cause a multi hit exception.
	 */
	xtensa_dtlb_entry_invalidate_sync(Z_XTENSA_TLB_ENTRY(Z_XTENSA_PAGE_TABLE_VADDR, 6));
	xtensa_itlb_entry_invalidate_sync(Z_XTENSA_TLB_ENTRY(Z_XTENSA_PAGE_TABLE_VADDR, 6));

	/* We are not using a flat table page, so we need to map
	 * only the top level page table (which maps the page table itself).
	 *
	 * Lets use one of the wired entry, so we never have tlb miss for
	 * the top level table.
	 */
	xtensa_dtlb_entry_write(Z_XTENSA_PTE((uint32_t)l1_page_table, Z_XTENSA_KERNEL_RING,
				Z_XTENSA_MMU_CACHED_WT),
			Z_XTENSA_TLB_ENTRY(Z_XTENSA_PAGE_TABLE_VADDR, MMU_PTE_WAY));

	/* Before invalidate the text region in the TLB entry 6, we need to
	 * map the exception vector into one of the wired entries to avoid
	 * a page miss for the exception.
	 */
	__asm__ volatile("rsr.vecbase %0" : "=r"(vecbase));

	xtensa_itlb_entry_write_sync(
		Z_XTENSA_PTE(vecbase, Z_XTENSA_KERNEL_RING,
			Z_XTENSA_MMU_X | Z_XTENSA_MMU_CACHED_WT),
		Z_XTENSA_TLB_ENTRY(
			Z_XTENSA_PTEVADDR + MB(4), 3));

	xtensa_dtlb_entry_write_sync(
		Z_XTENSA_PTE(vecbase, Z_XTENSA_KERNEL_RING,
			Z_XTENSA_MMU_X | Z_XTENSA_MMU_CACHED_WT),
		Z_XTENSA_TLB_ENTRY(
			Z_XTENSA_PTEVADDR + MB(4), 3));

	/* Temporarily uses KernelExceptionVector for level 1 interrupts
	 * handling. This is due to UserExceptionVector needing to jump to
	 * _Level1Vector. The jump ('j') instruction offset is incorrect
	 * when we move VECBASE below.
	 */
	__asm__ volatile("rsr.ps %0" : "=r"(ps));
	ps &= ~PS_UM;
	__asm__ volatile("wsr.ps %0; rsync" :: "a"(ps));

	__asm__ volatile("wsr.vecbase %0; rsync\n\t"
			:: "a"(Z_XTENSA_PTEVADDR + MB(4)));


	/* Finally, lets invalidate all entries in way 6 as the page tables
	 * should have already mapped the regions we care about for boot.
	 */
	for (entry = 0; entry < BIT(XCHAL_ITLB_ARF_ENTRIES_LOG2); entry++) {
		__asm__ volatile("iitlb %[idx]\n\t"
				 "isync"
				 :: [idx] "a"((entry << 29) | 6));
	}

	for (entry = 0; entry < BIT(XCHAL_DTLB_ARF_ENTRIES_LOG2); entry++) {
		__asm__ volatile("idtlb %[idx]\n\t"
				 "dsync"
				 :: [idx] "a"((entry << 29) | 6));
	}

	/* Map VECBASE to a fixed data TLB */
	xtensa_dtlb_entry_write(
			Z_XTENSA_PTE((uint32_t)vecbase,
				     Z_XTENSA_KERNEL_RING, Z_XTENSA_MMU_CACHED_WB),
			Z_XTENSA_TLB_ENTRY((uint32_t)vecbase, MMU_VECBASE_WAY));

	/*
	 * Pre-load TLB for vecbase so exception handling won't result
	 * in TLB miss during boot, and that we can handle single
	 * TLB misses.
	 */
	xtensa_itlb_entry_write_sync(
		Z_XTENSA_PTE(vecbase, Z_XTENSA_KERNEL_RING,
			Z_XTENSA_MMU_X | Z_XTENSA_MMU_CACHED_WT),
		Z_XTENSA_AUTOFILL_TLB_ENTRY(vecbase));

	/* To finish, just restore vecbase and invalidate TLB entries
	 * used to map the relocated vecbase.
	 */
	__asm__ volatile("wsr.vecbase %0; rsync\n\t"
			:: "a"(vecbase));

	/* Restore PS_UM so that level 1 interrupt handling will go to
	 * UserExceptionVector.
	 */
	__asm__ volatile("rsr.ps %0" : "=r"(ps));
	ps |= PS_UM;
	__asm__ volatile("wsr.ps %0; rsync" :: "a"(ps));

	xtensa_dtlb_entry_invalidate_sync(Z_XTENSA_TLB_ENTRY(Z_XTENSA_PTEVADDR + MB(4), 3));
	xtensa_itlb_entry_invalidate_sync(Z_XTENSA_TLB_ENTRY(Z_XTENSA_PTEVADDR + MB(4), 3));

	arch_xtensa_mmu_post_init(is_core0);
}

void z_xtensa_mmu_init(void)
{
	xtensa_mmu_init(true);
}

void z_xtensa_mmu_smp_init(void)
{
	xtensa_mmu_init(false);
}

#ifdef CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES
/* Zephyr's linker scripts for Xtensa usually puts
 * something before z_mapped_start (aka .text),
 * i.e. vecbase, so that we need to reserve those
 * space or else k_mem_map() would be mapping those,
 * resulting in faults.
 */
__weak void arch_reserved_pages_update(void)
{
	uintptr_t page;
	struct z_page_frame *pf;
	int idx;

	for (page = CONFIG_SRAM_BASE_ADDRESS, idx = 0;
	     page < (uintptr_t)z_mapped_start;
	     page += CONFIG_MMU_PAGE_SIZE, idx++) {
		pf = &z_page_frames[idx];

		pf->flags |= Z_PAGE_FRAME_RESERVED;
	}
}
#endif /* CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES */

static bool l2_page_table_map(void *vaddr, uintptr_t phys, uint32_t flags)
{
	uint32_t l1_pos = (uint32_t)vaddr >> 22;
	uint32_t pte = Z_XTENSA_PTE(phys, Z_XTENSA_KERNEL_RING, flags);
	uint32_t l2_pos = Z_XTENSA_L2_POS((uint32_t)vaddr);
	uint32_t *table;

	if (l1_page_table[l1_pos] == Z_XTENSA_MMU_ILLEGAL) {
		table  = alloc_l2_table();

		if (table == NULL) {
			return false;
		}

		l1_page_table[l1_pos] = Z_XTENSA_PTE((uint32_t)table, Z_XTENSA_KERNEL_RING,
				Z_XTENSA_MMU_CACHED_WT);
	}

	table = (uint32_t *)(l1_page_table[l1_pos] & Z_XTENSA_PTE_PPN_MASK);
	table[l2_pos] = pte;

	if ((flags & Z_XTENSA_MMU_X) == Z_XTENSA_MMU_X) {
		xtensa_itlb_vaddr_invalidate(vaddr);
	}
	xtensa_dtlb_vaddr_invalidate(vaddr);
	return true;
}

void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	uint32_t va = (uint32_t)virt;
	uint32_t pa = (uint32_t)phys;
	uint32_t rem_size = (uint32_t)size;
	uint32_t xtensa_flags = 0;
	int key;

	if (size == 0) {
		LOG_ERR("Cannot map physical memory at 0x%08X: invalid "
			"zero size", (uint32_t)phys);
		k_panic();
	}

	switch (flags & K_MEM_CACHE_MASK) {

	case K_MEM_CACHE_WB:
		xtensa_flags |= Z_XTENSA_MMU_CACHED_WB;
		break;
	case K_MEM_CACHE_WT:
		xtensa_flags |= Z_XTENSA_MMU_CACHED_WT;
		break;
	case K_MEM_CACHE_NONE:
		__fallthrough;
	default:
		break;
	}

	if ((flags & K_MEM_PERM_RW) == K_MEM_PERM_RW) {
		xtensa_flags |= Z_XTENSA_MMU_W;
	}
	if ((flags & K_MEM_PERM_EXEC) == K_MEM_PERM_EXEC) {
		xtensa_flags |= Z_XTENSA_MMU_X;
	}

	key = arch_irq_lock();

	while (rem_size > 0) {
		bool ret = l2_page_table_map((void *)va, pa, xtensa_flags);

		ARG_UNUSED(ret);
		__ASSERT(ret, "Virtual address (%u) already mapped", (uint32_t)virt);
		rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
		va += KB(4);
		pa += KB(4);
	}

	arch_irq_unlock(key);
}

static void l2_page_table_unmap(void *vaddr)
{
	uint32_t l1_pos = (uint32_t)vaddr >> 22;
	uint32_t l2_pos = Z_XTENSA_L2_POS((uint32_t)vaddr);
	uint32_t *table;
	uint32_t table_pos;
	bool exec;

	if (l1_page_table[l1_pos] == Z_XTENSA_MMU_ILLEGAL) {
		return;
	}

	exec = l1_page_table[l1_pos] & Z_XTENSA_MMU_X;

	table = (uint32_t *)(l1_page_table[l1_pos] & Z_XTENSA_PTE_PPN_MASK);
	table[l2_pos] = Z_XTENSA_MMU_ILLEGAL;

	for (l2_pos = 0; l2_pos < XTENSA_L2_PAGE_TABLE_ENTRIES; l2_pos++) {
		if (table[l2_pos] != Z_XTENSA_MMU_ILLEGAL) {
			goto end;
		}
	}

	l1_page_table[l1_pos] = Z_XTENSA_MMU_ILLEGAL;
	table_pos = (table - (uint32_t *)l2_page_tables) / (XTENSA_L2_PAGE_TABLE_ENTRIES);
	atomic_clear_bit(l2_page_tables_track, table_pos);

	/* Need to invalidate L2 page table as it is no longer valid. */
	xtensa_dtlb_vaddr_invalidate((void *)table);

end:
	if (exec) {
		xtensa_itlb_vaddr_invalidate(vaddr);
	}
	xtensa_dtlb_vaddr_invalidate(vaddr);
}

void arch_mem_unmap(void *addr, size_t size)
{
	uint32_t va = (uint32_t)addr;
	uint32_t rem_size = (uint32_t)size;
	int key;

	if (addr == NULL) {
		LOG_ERR("Cannot unmap NULL pointer");
		return;
	}

	if (size == 0) {
		LOG_ERR("Cannot unmap virtual memory with zero size");
		return;
	}

	key = arch_irq_lock();

	while (rem_size > 0) {
		l2_page_table_unmap((void *)va);
		rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
		va += KB(4);
	}

	arch_irq_unlock(key);
}
