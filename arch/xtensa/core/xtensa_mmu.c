/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/arch/xtensa/arch.h>
#include <zephyr/arch/xtensa/xtensa_mmu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/toolchain.h>
#include <xtensa/corebits.h>
#include <xtensa_mmu_priv.h>

#include <kernel_arch_func.h>
#include <mmu.h>

/* Level 1 contains page table entries
 * necessary to map the page table itself.
 */
#define XTENSA_L1_PAGE_TABLE_ENTRIES 1024U

/* Size of level 1 page table.
 */
#define XTENSA_L1_PAGE_TABLE_SIZE (XTENSA_L1_PAGE_TABLE_ENTRIES * sizeof(uint32_t))

/* Level 2 contains page table entries
 * necessary to map the page table itself.
 */
#define XTENSA_L2_PAGE_TABLE_ENTRIES 1024U

/* Size of level 2 page table.
 */
#define XTENSA_L2_PAGE_TABLE_SIZE (XTENSA_L2_PAGE_TABLE_ENTRIES * sizeof(uint32_t))

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MMU_PAGE_SIZE == 0x1000,
	     "MMU_PAGE_SIZE value is invalid, only 4 kB pages are supported\n");

/*
 * Level 1 page table has to be 4Kb to fit into one of the wired entries.
 * All entries are initialized as INVALID, so an attempt to read an unmapped
 * area will cause a double exception.
 *
 * Each memory domain contains its own l1 page table. The kernel l1 page table is
 * located at the index 0.
 */
static uint32_t l1_page_table[CONFIG_XTENSA_MMU_NUM_L1_TABLES][XTENSA_L1_PAGE_TABLE_ENTRIES]
				__aligned(KB(4));


/*
 * That is an alias for the page tables set used by the kernel.
 */
uint32_t *z_xtensa_kernel_ptables = (uint32_t *)l1_page_table[0];

/*
 * Each table in the level 2 maps a 4Mb memory range. It consists of 1024 entries each one
 * covering a 4Kb page.
 */
static uint32_t l2_page_tables[CONFIG_XTENSA_MMU_NUM_L2_TABLES][XTENSA_L2_PAGE_TABLE_ENTRIES]
				__aligned(KB(4));

/*
 * This additional variable tracks which l1 tables are in use. This is kept separated from
 * the tables to keep alignment easier.
 *
 * @note: The first bit is set because it is used for the kernel page tables.
 */
static ATOMIC_DEFINE(l1_page_table_track, CONFIG_XTENSA_MMU_NUM_L1_TABLES);

/*
 * This additional variable tracks which l2 tables are in use. This is kept separated from
 * the tables to keep alignment easier.
 */
static ATOMIC_DEFINE(l2_page_tables_track, CONFIG_XTENSA_MMU_NUM_L2_TABLES);

/*
 * Protects xtensa_domain_list and serializes access to page tables.
 */
static struct k_spinlock xtensa_mmu_lock;

#ifdef CONFIG_USERSPACE

/*
 * Each domain has its own ASID. ASID can go through 1 (kernel) to 255.
 * When a TLB entry matches, the hw will check the ASID in the entry and finds
 * the correspondent position in the RASID register. This position will then be
 * compared with the current ring (CRING) to check the permission.
 */
static uint8_t asid_count = 3;

/*
 * List with all active and initialized memory domains.
 */
static sys_slist_t xtensa_domain_list;
#endif /* CONFIG_USERSPACE */

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
		.attrs = Z_XTENSA_MMU_X | Z_XTENSA_MMU_CACHED_WB | Z_XTENSA_MMU_MAP_SHARED,
		.name = "text",
	},
	/* Mark rodata segment cacheable, read only and non-executable */
	{
		.start = (uint32_t)__rodata_region_start,
		.end   = (uint32_t)__rodata_region_end,
		.attrs = Z_XTENSA_MMU_CACHED_WB | Z_XTENSA_MMU_MAP_SHARED,
		.name = "rodata",
	},
};

static inline uint32_t *thread_page_tables_get(const struct k_thread *thread)
{
#ifdef CONFIG_USERSPACE
	if ((thread->base.user_options & K_USER) != 0U) {
		return thread->arch.ptables;
	}
#endif

	return z_xtensa_kernel_ptables;
}

/**
 * @brief Check if the page table entry is illegal.
 *
 * @param[in] Page table entry.
 */
static inline bool is_pte_illegal(uint32_t pte)
{
	uint32_t attr = pte & Z_XTENSA_PTE_ATTR_MASK;

	/*
	 * The ISA manual states only 12 and 14 are illegal values.
	 * 13 and 15 are not. So we need to be specific than simply
	 * testing if bits 2 and 3 are set.
	 */
	return (attr == 12) || (attr == 14);
}

/*
 * @brief Initialize all page table entries to be illegal.
 *
 * @param[in] Pointer to page table.
 * @param[in] Number of page table entries in the page table.
 */
static void init_page_table(uint32_t *ptable, size_t num_entries)
{
	int i;

	for (i = 0; i < num_entries; i++) {
		ptable[i] = Z_XTENSA_MMU_ILLEGAL;
	}
}

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

/**
 * @brief Switch page tables
 *
 * This switches the page tables to the incoming ones (@a ptables).
 * Since data TLBs to L2 page tables are auto-filled, @a dtlb_inv
 * can be used to invalidate these data TLBs. @a cache_inv can be
 * set to true to invalidate cache to the page tables.
 *
 * @param[in] ptables Page tables to be switched to.
 * @param[in] dtlb_inv True if to invalidate auto-fill data TLBs.
 * @param[in] cache_inv True if to invalidate cache to page tables.
 */
static ALWAYS_INLINE void switch_page_tables(uint32_t *ptables, bool dtlb_inv, bool cache_inv)
{
	if (cache_inv) {
		sys_cache_data_invd_range((void *)ptables, XTENSA_L1_PAGE_TABLE_SIZE);
		sys_cache_data_invd_range((void *)l2_page_tables, sizeof(l2_page_tables));
	}

	/* Invalidate data TLB to L1 page table */
	xtensa_dtlb_vaddr_invalidate((void *)Z_XTENSA_PAGE_TABLE_VADDR);

	/* Now map the pagetable itself with KERNEL asid to avoid user thread
	 * from tampering with it.
	 */
	xtensa_dtlb_entry_write_sync(
		Z_XTENSA_PTE((uint32_t)ptables, Z_XTENSA_KERNEL_RING, Z_XTENSA_PAGE_TABLE_ATTR),
		Z_XTENSA_TLB_ENTRY(Z_XTENSA_PAGE_TABLE_VADDR, Z_XTENSA_MMU_PTE_WAY));

	if (dtlb_inv) {
		/* Since L2 page tables are auto-refilled,
		 * invalidate all of them to flush the old entries out.
		 */
		xtensa_tlb_autorefill_invalidate();
	}
}

static void map_memory_range(const uint32_t start, const uint32_t end,
			     const uint32_t attrs, bool shared)
{
	uint32_t page, *table;

	for (page = start; page < end; page += CONFIG_MMU_PAGE_SIZE) {
		uint32_t pte = Z_XTENSA_PTE(page,
					    shared ? Z_XTENSA_SHARED_RING : Z_XTENSA_KERNEL_RING,
					    attrs);
		uint32_t l2_pos = Z_XTENSA_L2_POS(page);
		uint32_t l1_pos = Z_XTENSA_L1_POS(page);

		if (is_pte_illegal(z_xtensa_kernel_ptables[l1_pos])) {
			table  = alloc_l2_table();

			__ASSERT(table != NULL, "There is no l2 page table available to "
				"map 0x%08x\n", page);

			init_page_table(table, XTENSA_L2_PAGE_TABLE_ENTRIES);

			z_xtensa_kernel_ptables[l1_pos] =
				Z_XTENSA_PTE((uint32_t)table, Z_XTENSA_KERNEL_RING,
					     Z_XTENSA_PAGE_TABLE_ATTR);
		}

		table = (uint32_t *)(z_xtensa_kernel_ptables[l1_pos] & Z_XTENSA_PTE_PPN_MASK);
		table[l2_pos] = pte;
	}
}

static void map_memory(const uint32_t start, const uint32_t end,
		       const uint32_t attrs, bool shared)
{
	map_memory_range(start, end, attrs, shared);

#ifdef CONFIG_XTENSA_MMU_DOUBLE_MAP
	if (arch_xtensa_is_ptr_uncached((void *)start)) {
		map_memory_range(POINTER_TO_UINT(z_soc_cached_ptr((void *)start)),
			POINTER_TO_UINT(z_soc_cached_ptr((void *)end)),
			attrs | Z_XTENSA_MMU_CACHED_WB, shared);
	} else if (arch_xtensa_is_ptr_cached((void *)start)) {
		map_memory_range(POINTER_TO_UINT(z_soc_uncached_ptr((void *)start)),
			POINTER_TO_UINT(z_soc_uncached_ptr((void *)end)), attrs, shared);
	}
#endif
}

static void xtensa_init_page_tables(void)
{
	volatile uint8_t entry;

	init_page_table(z_xtensa_kernel_ptables, XTENSA_L1_PAGE_TABLE_ENTRIES);
	atomic_set_bit(l1_page_table_track, 0);

	for (entry = 0; entry < ARRAY_SIZE(mmu_zephyr_ranges); entry++) {
		const struct xtensa_mmu_range *range = &mmu_zephyr_ranges[entry];
		bool shared;
		uint32_t attrs;

		shared = !!(range->attrs & Z_XTENSA_MMU_MAP_SHARED);
		attrs = range->attrs & ~Z_XTENSA_MMU_MAP_SHARED;

		map_memory(range->start, range->end, attrs, shared);
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
		bool shared;
		uint32_t attrs;

		shared = !!(range->attrs & Z_XTENSA_MMU_MAP_SHARED);
		attrs = range->attrs & ~Z_XTENSA_MMU_MAP_SHARED;

		map_memory(range->start, range->end, attrs, shared);
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

void z_xtensa_mmu_init(void)
{
	volatile uint8_t entry;
	uint32_t ps, vecbase;

	if (_current_cpu->id == 0) {
		/* This is normally done via arch_kernel_init() inside z_cstart().
		 * However, before that is called, we go through the sys_init of
		 * INIT_LEVEL_EARLY, which is going to result in TLB misses.
		 * So setup whatever necessary so the exception handler can work
		 * properly.
		 */
		xtensa_init_page_tables();
	}

	/* Set the page table location in the virtual address */
	xtensa_ptevaddr_set((void *)Z_XTENSA_PTEVADDR);

	/* Set rasid */
	xtensa_rasid_asid_set(Z_XTENSA_MMU_SHARED_ASID, Z_XTENSA_SHARED_RING);

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
	xtensa_dtlb_entry_write(Z_XTENSA_PTE((uint32_t)z_xtensa_kernel_ptables,
					     Z_XTENSA_KERNEL_RING, Z_XTENSA_PAGE_TABLE_ATTR),
			Z_XTENSA_TLB_ENTRY(Z_XTENSA_PAGE_TABLE_VADDR, Z_XTENSA_MMU_PTE_WAY));

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
			Z_XTENSA_TLB_ENTRY((uint32_t)vecbase, Z_XTENSA_MMU_VECBASE_WAY));

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

	/*
	 * Clear out THREADPTR as we use it to indicate
	 * whether we are in user mode or not.
	 */
	XTENSA_WUR("THREADPTR", 0);

	arch_xtensa_mmu_post_init(_current_cpu->id == 0);
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

static bool l2_page_table_map(uint32_t *l1_table, void *vaddr, uintptr_t phys,
			      uint32_t flags, bool is_user)
{
	uint32_t l1_pos = (uint32_t)vaddr >> 22;
	uint32_t l2_pos = Z_XTENSA_L2_POS((uint32_t)vaddr);
	uint32_t *table;

	sys_cache_data_invd_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));

	if (is_pte_illegal(l1_table[l1_pos])) {
		table  = alloc_l2_table();

		if (table == NULL) {
			return false;
		}

		init_page_table(table, XTENSA_L2_PAGE_TABLE_ENTRIES);

		l1_table[l1_pos] = Z_XTENSA_PTE((uint32_t)table, Z_XTENSA_KERNEL_RING,
						Z_XTENSA_PAGE_TABLE_ATTR);

		sys_cache_data_flush_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));
	}

	table = (uint32_t *)(l1_table[l1_pos] & Z_XTENSA_PTE_PPN_MASK);
	table[l2_pos] = Z_XTENSA_PTE(phys, is_user ? Z_XTENSA_USER_RING : Z_XTENSA_KERNEL_RING,
				     flags);

	sys_cache_data_flush_range((void *)&table[l2_pos], sizeof(table[0]));

	return true;
}

static inline void __arch_mem_map(void *va, uintptr_t pa, uint32_t xtensa_flags, bool is_user)
{
	bool ret;
	void *vaddr, *vaddr_uc;
	uintptr_t paddr, paddr_uc;
	uint32_t flags, flags_uc;

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
		if (arch_xtensa_is_ptr_cached(va)) {
			vaddr = va;
			vaddr_uc = arch_xtensa_uncached_ptr(va);
		} else {
			vaddr = arch_xtensa_cached_ptr(va);
			vaddr_uc = va;
		}

		if (arch_xtensa_is_ptr_cached((void *)pa)) {
			paddr = pa;
			paddr_uc = (uintptr_t)arch_xtensa_uncached_ptr((void *)pa);
		} else {
			paddr = (uintptr_t)arch_xtensa_cached_ptr((void *)pa);
			paddr_uc = pa;
		}

		flags_uc = (xtensa_flags & ~Z_XTENSA_PTE_ATTR_CACHED_MASK);
		flags = flags_uc | Z_XTENSA_MMU_CACHED_WB;
	} else {
		vaddr = va;
		paddr = pa;
		flags = xtensa_flags;
	}

	ret = l2_page_table_map(z_xtensa_kernel_ptables, (void *)vaddr, paddr,
				flags, is_user);
	__ASSERT(ret, "Virtual address (%p) already mapped", va);

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP) && ret) {
		ret = l2_page_table_map(z_xtensa_kernel_ptables, (void *)vaddr_uc, paddr_uc,
					flags_uc, is_user);
		__ASSERT(ret, "Virtual address (%p) already mapped", vaddr_uc);
	}

#ifndef CONFIG_USERSPACE
	ARG_UNUSED(ret);
#else
	if (ret) {
		sys_snode_t *node;
		struct arch_mem_domain *domain;
		k_spinlock_key_t key;

		key = k_spin_lock(&z_mem_domain_lock);
		SYS_SLIST_FOR_EACH_NODE(&xtensa_domain_list, node) {
			domain = CONTAINER_OF(node, struct arch_mem_domain, node);

			ret = l2_page_table_map(domain->ptables, (void *)vaddr, paddr,
						flags, is_user);
			__ASSERT(ret, "Virtual address (%p) already mapped for domain %p",
				 vaddr, domain);

			if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP) && ret) {
				ret = l2_page_table_map(domain->ptables,
							(void *)vaddr_uc, paddr_uc,
							flags_uc, is_user);
				__ASSERT(ret, "Virtual address (%p) already mapped for domain %p",
					 vaddr_uc, domain);
			}
		}
		k_spin_unlock(&z_mem_domain_lock, key);
	}
#endif /* CONFIG_USERSPACE */

	if ((xtensa_flags & Z_XTENSA_MMU_X) == Z_XTENSA_MMU_X) {
		xtensa_itlb_vaddr_invalidate(vaddr);
	}
	xtensa_dtlb_vaddr_invalidate(vaddr);

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
		if (xtensa_flags & Z_XTENSA_MMU_X) {
			xtensa_itlb_vaddr_invalidate(vaddr_uc);
		}
		xtensa_dtlb_vaddr_invalidate(vaddr_uc);
	}
}

void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	uint32_t va = (uint32_t)virt;
	uint32_t pa = (uint32_t)phys;
	uint32_t rem_size = (uint32_t)size;
	uint32_t xtensa_flags = 0;
	k_spinlock_key_t key;
	bool is_user;

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

	is_user = (flags & K_MEM_PERM_USER) == K_MEM_PERM_USER;

	key = k_spin_lock(&xtensa_mmu_lock);

	while (rem_size > 0) {
		__arch_mem_map((void *)va, pa, xtensa_flags, is_user);

		rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
		va += KB(4);
		pa += KB(4);
	}

	k_spin_unlock(&xtensa_mmu_lock, key);
}

/**
 * @return True if page is executable (thus need to invalidate ITLB),
 *         false if not.
 */
static bool l2_page_table_unmap(uint32_t *l1_table, void *vaddr)
{
	uint32_t l1_pos = (uint32_t)vaddr >> 22;
	uint32_t l2_pos = Z_XTENSA_L2_POS((uint32_t)vaddr);
	uint32_t *l2_table;
	uint32_t table_pos;
	bool exec;

	sys_cache_data_invd_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));

	if (is_pte_illegal(l1_table[l1_pos])) {
		/* We shouldn't be unmapping an illegal entry.
		 * Return true so that we can invalidate ITLB too.
		 */
		return true;
	}

	exec = l1_table[l1_pos] & Z_XTENSA_MMU_X;

	l2_table = (uint32_t *)(l1_table[l1_pos] & Z_XTENSA_PTE_PPN_MASK);

	sys_cache_data_invd_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));

	l2_table[l2_pos] = Z_XTENSA_MMU_ILLEGAL;

	sys_cache_data_flush_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));

	for (l2_pos = 0; l2_pos < XTENSA_L2_PAGE_TABLE_ENTRIES; l2_pos++) {
		if (!is_pte_illegal(l2_table[l2_pos])) {
			goto end;
		}
	}

	l1_table[l1_pos] = Z_XTENSA_MMU_ILLEGAL;
	sys_cache_data_flush_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));

	table_pos = (l2_table - (uint32_t *)l2_page_tables) / (XTENSA_L2_PAGE_TABLE_ENTRIES);
	atomic_clear_bit(l2_page_tables_track, table_pos);

	/* Need to invalidate L2 page table as it is no longer valid. */
	xtensa_dtlb_vaddr_invalidate((void *)l2_table);

end:
	return exec;
}

static inline void __arch_mem_unmap(void *va)
{
	bool is_exec;
	void *vaddr, *vaddr_uc;

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
		if (arch_xtensa_is_ptr_cached(va)) {
			vaddr = va;
			vaddr_uc = arch_xtensa_uncached_ptr(va);
		} else {
			vaddr = arch_xtensa_cached_ptr(va);
			vaddr_uc = va;
		}
	} else {
		vaddr = va;
	}

	is_exec = l2_page_table_unmap(z_xtensa_kernel_ptables, (void *)vaddr);

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
		(void)l2_page_table_unmap(z_xtensa_kernel_ptables, (void *)vaddr_uc);
	}

#ifdef CONFIG_USERSPACE
	sys_snode_t *node;
	struct arch_mem_domain *domain;
	k_spinlock_key_t key;

	key = k_spin_lock(&z_mem_domain_lock);
	SYS_SLIST_FOR_EACH_NODE(&xtensa_domain_list, node) {
		domain = CONTAINER_OF(node, struct arch_mem_domain, node);

		(void)l2_page_table_unmap(domain->ptables, (void *)vaddr);

		if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
			(void)l2_page_table_unmap(domain->ptables, (void *)vaddr_uc);
		}
	}
	k_spin_unlock(&z_mem_domain_lock, key);
#endif /* CONFIG_USERSPACE */

	if (is_exec) {
		xtensa_itlb_vaddr_invalidate(vaddr);
	}
	xtensa_dtlb_vaddr_invalidate(vaddr);

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
		if (is_exec) {
			xtensa_itlb_vaddr_invalidate(vaddr_uc);
		}
		xtensa_dtlb_vaddr_invalidate(vaddr_uc);
	}
}

void arch_mem_unmap(void *addr, size_t size)
{
	uint32_t va = (uint32_t)addr;
	uint32_t rem_size = (uint32_t)size;
	k_spinlock_key_t key;

	if (addr == NULL) {
		LOG_ERR("Cannot unmap NULL pointer");
		return;
	}

	if (size == 0) {
		LOG_ERR("Cannot unmap virtual memory with zero size");
		return;
	}

	key = k_spin_lock(&xtensa_mmu_lock);

	while (rem_size > 0) {
		__arch_mem_unmap((void *)va);

		rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
		va += KB(4);
	}

	k_spin_unlock(&xtensa_mmu_lock, key);
}

#ifdef CONFIG_USERSPACE

static inline uint32_t *alloc_l1_table(void)
{
	uint16_t idx;

	for (idx = 0; idx < CONFIG_XTENSA_MMU_NUM_L1_TABLES; idx++) {
		if (!atomic_test_and_set_bit(l1_page_table_track, idx)) {
			return (uint32_t *)&l1_page_table[idx];
		}
	}

	return NULL;
}

static uint32_t *dup_table(uint32_t *source_table)
{
	uint16_t i, j;
	uint32_t *dst_table = alloc_l1_table();

	if (!dst_table) {
		return NULL;
	}

	for (i = 0; i < XTENSA_L1_PAGE_TABLE_ENTRIES; i++) {
		uint32_t *l2_table, *src_l2_table;

		if (is_pte_illegal(source_table[i])) {
			dst_table[i] = Z_XTENSA_MMU_ILLEGAL;
			continue;
		}

		src_l2_table = (uint32_t *)(source_table[i] & Z_XTENSA_PTE_PPN_MASK);
		l2_table = alloc_l2_table();
		if (l2_table == NULL) {
			goto err;
		}

		for (j = 0; j < XTENSA_L2_PAGE_TABLE_ENTRIES; j++) {
			l2_table[j] =  src_l2_table[j];
		}

		/* The page table is using kernel ASID because we don't
		 * user thread manipulate it.
		 */
		dst_table[i] = Z_XTENSA_PTE((uint32_t)l2_table, Z_XTENSA_KERNEL_RING,
					    Z_XTENSA_PAGE_TABLE_ATTR);

		sys_cache_data_flush_range((void *)l2_table, XTENSA_L2_PAGE_TABLE_SIZE);
	}

	sys_cache_data_flush_range((void *)dst_table, XTENSA_L1_PAGE_TABLE_SIZE);

	return dst_table;

err:
	/* TODO: Cleanup failed allocation*/
	return NULL;
}

int arch_mem_domain_init(struct k_mem_domain *domain)
{
	uint32_t *ptables;
	k_spinlock_key_t key;
	int ret;

	/*
	 * For now, lets just assert if we have reached the maximum number
	 * of asid we assert.
	 */
	__ASSERT(asid_count < (Z_XTENSA_MMU_SHARED_ASID), "Reached maximum of ASID available");

	key = k_spin_lock(&xtensa_mmu_lock);
	ptables = dup_table(z_xtensa_kernel_ptables);

	if (ptables == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	domain->arch.ptables = ptables;
	domain->arch.asid = ++asid_count;

	sys_slist_append(&xtensa_domain_list, &domain->arch.node);

	ret = 0;

err:
	k_spin_unlock(&xtensa_mmu_lock, key);

	return ret;
}

static int region_map_update(uint32_t *ptables, uintptr_t start,
			      size_t size, uint32_t ring, uint32_t flags)
{
	int ret = 0;

	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		uint32_t *l2_table, pte;
		uint32_t page = start + offset;
		uint32_t l1_pos = page >> 22;
		uint32_t l2_pos = Z_XTENSA_L2_POS(page);

		/* Make sure we grab a fresh copy of L1 page table */
		sys_cache_data_invd_range((void *)&ptables[l1_pos], sizeof(ptables[0]));

		l2_table = (uint32_t *)(ptables[l1_pos] & Z_XTENSA_PTE_PPN_MASK);

		sys_cache_data_invd_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));

		pte = Z_XTENSA_PTE_RING_SET(l2_table[l2_pos], ring);
		pte = Z_XTENSA_PTE_ATTR_SET(pte, flags);

		l2_table[l2_pos] = pte;

		sys_cache_data_flush_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));

		xtensa_dtlb_vaddr_invalidate(
			(void *)(pte & Z_XTENSA_PTE_PPN_MASK));
	}

	return ret;
}

static inline int update_region(uint32_t *ptables, uintptr_t start,
				size_t size, uint32_t ring, uint32_t flags)
{
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&xtensa_mmu_lock);

#ifdef CONFIG_XTENSA_MMU_DOUBLE_MAP
	uintptr_t va, va_uc;
	uint32_t new_flags, new_flags_uc;

	if (arch_xtensa_is_ptr_cached((void *)start)) {
		va = start;
		va_uc = (uintptr_t)arch_xtensa_uncached_ptr((void *)start);
	} else {
		va = (uintptr_t)arch_xtensa_cached_ptr((void *)start);
		va_uc = start;
	}

	new_flags_uc = (flags & ~Z_XTENSA_PTE_ATTR_CACHED_MASK);
	new_flags = new_flags_uc | Z_XTENSA_MMU_CACHED_WB;

	ret = region_map_update(ptables, va, size, ring, new_flags);

	if (ret == 0) {
		ret = region_map_update(ptables, va_uc, size, ring, new_flags_uc);
	}
#else
	ret = region_map_update(ptables, start, size, ring, flags);
#endif /* CONFIG_XTENSA_MMU_DOUBLE_MAP */

	k_spin_unlock(&xtensa_mmu_lock, key);

	return ret;
}

static inline int reset_region(uint32_t *ptables, uintptr_t start, size_t size)
{
	return update_region(ptables, start, size, Z_XTENSA_KERNEL_RING, Z_XTENSA_MMU_W);
}

void xtensa_set_stack_perms(struct k_thread *thread)
{
	if ((thread->base.user_options & K_USER) == 0) {
		return;
	}

	update_region(thread_page_tables_get(thread),
		      thread->stack_info.start, thread->stack_info.size,
		      Z_XTENSA_USER_RING, Z_XTENSA_MMU_W | Z_XTENSA_MMU_CACHED_WB);
}

void xtensa_user_stack_perms(struct k_thread *thread)
{
	(void)memset((void *)thread->stack_info.start, 0xAA,
		thread->stack_info.size - thread->stack_info.delta);

	update_region(thread_page_tables_get(thread),
		      thread->stack_info.start, thread->stack_info.size,
		      Z_XTENSA_USER_RING, Z_XTENSA_MMU_W | Z_XTENSA_MMU_CACHED_WB);
}

int arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				uint32_t partition_id)
{
	struct k_mem_partition *partition = &domain->partitions[partition_id];

	/* Reset the partition's region back to defaults */
	return reset_region(domain->arch.ptables, partition->start,
			    partition->size);
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				uint32_t partition_id)
{
	uint32_t ring = domain->arch.asid == 0 ? Z_XTENSA_KERNEL_RING : Z_XTENSA_USER_RING;
	struct k_mem_partition *partition = &domain->partitions[partition_id];

	return update_region(domain->arch.ptables, partition->start,
			     partition->size, ring, partition->attr);
}

/* These APIs don't need to do anything */
int arch_mem_domain_thread_add(struct k_thread *thread)
{
	int ret = 0;
	bool is_user, is_migration;
	uint32_t *old_ptables;
	struct k_mem_domain *domain;

	old_ptables = thread->arch.ptables;
	domain = thread->mem_domain_info.mem_domain;
	thread->arch.ptables = domain->arch.ptables;

	is_user = (thread->base.user_options & K_USER) != 0;
	is_migration = (old_ptables != NULL) && is_user;

	/* Give access to the thread's stack in its new
	 * memory domain if it is migrating.
	 */
	if (is_migration) {
		xtensa_set_stack_perms(thread);
	}

	if (is_migration) {
		ret = reset_region(old_ptables,
			thread->stack_info.start,
			thread->stack_info.size);
	}

	return ret;
}

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
			    thread->stack_info.start,
			    thread->stack_info.size);
}

static bool page_validate(uint32_t *ptables, uint32_t page, uint8_t ring, bool write)
{
	uint8_t asid_ring;
	uint32_t rasid, pte, *l2_table;
	uint32_t l1_pos = page >> 22;
	uint32_t l2_pos = Z_XTENSA_L2_POS(page);

	if (is_pte_illegal(ptables[l1_pos])) {
		return false;
	}

	l2_table = (uint32_t *)(ptables[l1_pos] & Z_XTENSA_PTE_PPN_MASK);
	pte = l2_table[l2_pos];

	if (is_pte_illegal(pte)) {
		return false;
	}

	asid_ring = 0;
	rasid = xtensa_rasid_get();
	for (uint32_t i = 0; i < 4; i++) {
		if (Z_XTENSA_PTE_ASID_GET(pte, rasid) ==
				Z_XTENSA_RASID_ASID_GET(rasid, i)) {
			asid_ring = i;
			break;
		}
	}

	if (ring > asid_ring) {
		return false;
	}

	if (write) {
		return (Z_XTENSA_PTE_ATTR_GET((pte)) & Z_XTENSA_MMU_W) != 0;
	}

	return true;
}

int arch_buffer_validate(void *addr, size_t size, int write)
{
	int ret = 0;
	uint8_t *virt;
	size_t aligned_size;
	const struct k_thread *thread = _current;
	uint32_t *ptables = thread_page_tables_get(thread);
	uint8_t ring = ((thread->base.user_options & K_USER) != 0) ?
		Z_XTENSA_USER_RING : Z_XTENSA_KERNEL_RING;

	/* addr/size arbitrary, fix this up into an aligned region */
	k_mem_region_align((uintptr_t *)&virt, &aligned_size,
			   (uintptr_t)addr, size, CONFIG_MMU_PAGE_SIZE);

	for (size_t offset = 0; offset < aligned_size;
	     offset += CONFIG_MMU_PAGE_SIZE) {
		if (!page_validate(ptables, (uint32_t)(virt + offset), ring, write)) {
			ret = -1;
			break;
		}
	}

	return ret;
}

void z_xtensa_swap_update_page_tables(struct k_thread *incoming)
{
	uint32_t *ptables = incoming->arch.ptables;
	struct arch_mem_domain *domain =
		&(incoming->mem_domain_info.mem_domain->arch);

	/* Lets set the asid for the incoming thread */
	if ((incoming->base.user_options & K_USER) != 0) {
		xtensa_rasid_asid_set(domain->asid, Z_XTENSA_USER_RING);
	}

	switch_page_tables(ptables, true, false);
}

#endif /* CONFIG_USERSPACE */
