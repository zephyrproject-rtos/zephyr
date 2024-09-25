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

/* Skip TLB IPI when updating page tables.
 * This allows us to send IPI only after the last
 * changes of a series.
 */
#define OPTION_NO_TLB_IPI BIT(0)

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
uint32_t *xtensa_kernel_ptables = (uint32_t *)l1_page_table[0];

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
		.attrs = XTENSA_MMU_PERM_W,
#else
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
#endif
		.name = "data",
	},
#if K_HEAP_MEM_POOL_SIZE > 0
	/* System heap memory */
	{
		.start = (uint32_t)_heap_start,
		.end   = (uint32_t)_heap_end,
#ifdef CONFIG_XTENSA_RPO_CACHE
		.attrs = XTENSA_MMU_PERM_W,
#else
		.attrs = XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
#endif
		.name = "heap",
	},
#endif
	/* Mark text segment cacheable, read only and executable */
	{
		.start = (uint32_t)__text_region_start,
		.end   = (uint32_t)__text_region_end,
		.attrs = XTENSA_MMU_PERM_X | XTENSA_MMU_CACHED_WB | XTENSA_MMU_MAP_SHARED,
		.name = "text",
	},
	/* Mark rodata segment cacheable, read only and non-executable */
	{
		.start = (uint32_t)__rodata_region_start,
		.end   = (uint32_t)__rodata_region_end,
		.attrs = XTENSA_MMU_CACHED_WB | XTENSA_MMU_MAP_SHARED,
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

	return xtensa_kernel_ptables;
}

/**
 * @brief Check if the page table entry is illegal.
 *
 * @param[in] Page table entry.
 */
static inline bool is_pte_illegal(uint32_t pte)
{
	uint32_t attr = pte & XTENSA_MMU_PTE_ATTR_MASK;

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
		ptable[i] = XTENSA_MMU_PTE_ILLEGAL;
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

static void map_memory_range(const uint32_t start, const uint32_t end,
			     const uint32_t attrs)
{
	uint32_t page, *table;
	bool shared = !!(attrs & XTENSA_MMU_MAP_SHARED);
	uint32_t sw_attrs = (attrs & XTENSA_MMU_PTE_ATTR_ORIGINAL) == XTENSA_MMU_PTE_ATTR_ORIGINAL ?
		attrs : 0;

	for (page = start; page < end; page += CONFIG_MMU_PAGE_SIZE) {
		uint32_t pte = XTENSA_MMU_PTE(page,
					      shared ? XTENSA_MMU_SHARED_RING :
						       XTENSA_MMU_KERNEL_RING,
					      sw_attrs, attrs);
		uint32_t l2_pos = XTENSA_MMU_L2_POS(page);
		uint32_t l1_pos = XTENSA_MMU_L1_POS(page);

		if (is_pte_illegal(xtensa_kernel_ptables[l1_pos])) {
			table  = alloc_l2_table();

			__ASSERT(table != NULL, "There is no l2 page table available to "
				"map 0x%08x\n", page);

			init_page_table(table, XTENSA_L2_PAGE_TABLE_ENTRIES);

			xtensa_kernel_ptables[l1_pos] =
				XTENSA_MMU_PTE((uint32_t)table, XTENSA_MMU_KERNEL_RING,
					       sw_attrs, XTENSA_MMU_PAGE_TABLE_ATTR);
		}

		table = (uint32_t *)(xtensa_kernel_ptables[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);
		table[l2_pos] = pte;
	}
}

static void map_memory(const uint32_t start, const uint32_t end,
		       const uint32_t attrs)
{
	map_memory_range(start, end, attrs);

#ifdef CONFIG_XTENSA_MMU_DOUBLE_MAP
	if (sys_cache_is_ptr_uncached((void *)start)) {
		map_memory_range(POINTER_TO_UINT(sys_cache_cached_ptr_get((void *)start)),
			POINTER_TO_UINT(sys_cache_cached_ptr_get((void *)end)),
			attrs | XTENSA_MMU_CACHED_WB);
	} else if (sys_cache_is_ptr_cached((void *)start)) {
		map_memory_range(POINTER_TO_UINT(sys_cache_uncached_ptr_get((void *)start)),
			POINTER_TO_UINT(sys_cache_uncached_ptr_get((void *)end)), attrs);
	}
#endif
}

static void xtensa_init_page_tables(void)
{
	volatile uint8_t entry;

	init_page_table(xtensa_kernel_ptables, XTENSA_L1_PAGE_TABLE_ENTRIES);
	atomic_set_bit(l1_page_table_track, 0);

	for (entry = 0; entry < ARRAY_SIZE(mmu_zephyr_ranges); entry++) {
		const struct xtensa_mmu_range *range = &mmu_zephyr_ranges[entry];

		map_memory(range->start, range->end, range->attrs | XTENSA_MMU_PTE_ATTR_ORIGINAL);
	}

	for (entry = 0; entry < xtensa_soc_mmu_ranges_num; entry++) {
		const struct xtensa_mmu_range *range = &xtensa_soc_mmu_ranges[entry];

		map_memory(range->start, range->end, range->attrs | XTENSA_MMU_PTE_ATTR_ORIGINAL);
	}

	/* Finally, the direct-mapped pages used in the page tables
	 * must be fixed up to use the same cache attribute (but these
	 * must be writable, obviously).  They shouldn't be left at
	 * the default.
	 */
	map_memory_range((uint32_t) &l1_page_table[0],
			 (uint32_t) &l1_page_table[CONFIG_XTENSA_MMU_NUM_L1_TABLES],
			 XTENSA_MMU_PAGE_TABLE_ATTR | XTENSA_MMU_PERM_W);
	map_memory_range((uint32_t) &l2_page_tables[0],
			 (uint32_t) &l2_page_tables[CONFIG_XTENSA_MMU_NUM_L2_TABLES],
			 XTENSA_MMU_PAGE_TABLE_ATTR | XTENSA_MMU_PERM_W);

	sys_cache_data_flush_all();
}

__weak void arch_xtensa_mmu_post_init(bool is_core0)
{
	ARG_UNUSED(is_core0);
}

void xtensa_mmu_init(void)
{
	if (_current_cpu->id == 0) {
		/* This is normally done via arch_kernel_init() inside z_cstart().
		 * However, before that is called, we go through the sys_init of
		 * INIT_LEVEL_EARLY, which is going to result in TLB misses.
		 * So setup whatever necessary so the exception handler can work
		 * properly.
		 */
		xtensa_init_page_tables();
	}

	xtensa_init_paging(xtensa_kernel_ptables);

	/*
	 * This is used to determine whether we are faulting inside double
	 * exception if this is not zero. Sometimes SoC starts with this not
	 * being set to zero. So clear it during boot.
	 */
	XTENSA_WSR(ZSR_DEPC_SAVE_STR, 0);

	arch_xtensa_mmu_post_init(_current_cpu->id == 0);
}

void xtensa_mmu_reinit(void)
{
	/* First initialize the hardware */
	xtensa_init_paging(xtensa_kernel_ptables);

#ifdef CONFIG_USERSPACE
	struct k_thread *thread = _current_cpu->current;
	struct arch_mem_domain *domain =
			&(thread->mem_domain_info.mem_domain->arch);


	/* Set the page table for current context */
	xtensa_set_paging(domain->asid, domain->ptables);
#endif /* CONFIG_USERSPACE */

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
	int idx;

	for (page = CONFIG_SRAM_BASE_ADDRESS, idx = 0;
	     page < (uintptr_t)z_mapped_start;
	     page += CONFIG_MMU_PAGE_SIZE, idx++) {
		k_mem_page_frame_set(&k_mem_page_frames[idx], K_MEM_PAGE_FRAME_RESERVED);
	}
}
#endif /* CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES */

static bool l2_page_table_map(uint32_t *l1_table, void *vaddr, uintptr_t phys,
			      uint32_t flags, bool is_user)
{
	uint32_t l1_pos = XTENSA_MMU_L1_POS((uint32_t)vaddr);
	uint32_t l2_pos = XTENSA_MMU_L2_POS((uint32_t)vaddr);
	uint32_t *table;

	sys_cache_data_invd_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));

	if (is_pte_illegal(l1_table[l1_pos])) {
		table  = alloc_l2_table();

		if (table == NULL) {
			return false;
		}

		init_page_table(table, XTENSA_L2_PAGE_TABLE_ENTRIES);

		l1_table[l1_pos] = XTENSA_MMU_PTE((uint32_t)table, XTENSA_MMU_KERNEL_RING,
						  0, XTENSA_MMU_PAGE_TABLE_ATTR);

		sys_cache_data_flush_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));
	}

	table = (uint32_t *)(l1_table[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);
	table[l2_pos] = XTENSA_MMU_PTE(phys, is_user ? XTENSA_MMU_USER_RING :
						       XTENSA_MMU_KERNEL_RING,
				       0, flags);

	sys_cache_data_flush_range((void *)&table[l2_pos], sizeof(table[0]));
	xtensa_tlb_autorefill_invalidate();

	return true;
}

static inline void __arch_mem_map(void *va, uintptr_t pa, uint32_t xtensa_flags, bool is_user)
{
	bool ret;
	void *vaddr, *vaddr_uc;
	uintptr_t paddr, paddr_uc;
	uint32_t flags, flags_uc;

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
		if (sys_cache_is_ptr_cached(va)) {
			vaddr = va;
			vaddr_uc = sys_cache_uncached_ptr_get(va);
		} else {
			vaddr = sys_cache_cached_ptr_get(va);
			vaddr_uc = va;
		}

		if (sys_cache_is_ptr_cached((void *)pa)) {
			paddr = pa;
			paddr_uc = (uintptr_t)sys_cache_uncached_ptr_get((void *)pa);
		} else {
			paddr = (uintptr_t)sys_cache_cached_ptr_get((void *)pa);
			paddr_uc = pa;
		}

		flags_uc = (xtensa_flags & ~XTENSA_MMU_PTE_ATTR_CACHED_MASK);
		flags = flags_uc | XTENSA_MMU_CACHED_WB;
	} else {
		vaddr = va;
		paddr = pa;
		flags = xtensa_flags;
	}

	ret = l2_page_table_map(xtensa_kernel_ptables, (void *)vaddr, paddr,
				flags, is_user);
	__ASSERT(ret, "Virtual address (%p) already mapped", va);

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP) && ret) {
		ret = l2_page_table_map(xtensa_kernel_ptables, (void *)vaddr_uc, paddr_uc,
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
		xtensa_flags |= XTENSA_MMU_CACHED_WB;
		break;
	case K_MEM_CACHE_WT:
		xtensa_flags |= XTENSA_MMU_CACHED_WT;
		break;
	case K_MEM_CACHE_NONE:
		__fallthrough;
	default:
		break;
	}

	if ((flags & K_MEM_PERM_RW) == K_MEM_PERM_RW) {
		xtensa_flags |= XTENSA_MMU_PERM_W;
	}
	if ((flags & K_MEM_PERM_EXEC) == K_MEM_PERM_EXEC) {
		xtensa_flags |= XTENSA_MMU_PERM_X;
	}

	is_user = (flags & K_MEM_PERM_USER) == K_MEM_PERM_USER;

	key = k_spin_lock(&xtensa_mmu_lock);

	while (rem_size > 0) {
		__arch_mem_map((void *)va, pa, xtensa_flags, is_user);

		rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
		va += KB(4);
		pa += KB(4);
	}

#if CONFIG_MP_MAX_NUM_CPUS > 1
	xtensa_mmu_tlb_ipi();
#endif

	sys_cache_data_flush_and_invd_all();
	k_spin_unlock(&xtensa_mmu_lock, key);
}

/**
 * @return True if page is executable (thus need to invalidate ITLB),
 *         false if not.
 */
static bool l2_page_table_unmap(uint32_t *l1_table, void *vaddr)
{
	uint32_t l1_pos = XTENSA_MMU_L1_POS((uint32_t)vaddr);
	uint32_t l2_pos = XTENSA_MMU_L2_POS((uint32_t)vaddr);
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

	exec = l1_table[l1_pos] & XTENSA_MMU_PERM_X;

	l2_table = (uint32_t *)(l1_table[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);

	sys_cache_data_invd_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));

	l2_table[l2_pos] = XTENSA_MMU_PTE_ILLEGAL;

	sys_cache_data_flush_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));

	for (l2_pos = 0; l2_pos < XTENSA_L2_PAGE_TABLE_ENTRIES; l2_pos++) {
		if (!is_pte_illegal(l2_table[l2_pos])) {
			goto end;
		}
	}

	l1_table[l1_pos] = XTENSA_MMU_PTE_ILLEGAL;
	sys_cache_data_flush_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));

	table_pos = (l2_table - (uint32_t *)l2_page_tables) / (XTENSA_L2_PAGE_TABLE_ENTRIES);
	atomic_clear_bit(l2_page_tables_track, table_pos);

end:
	/* Need to invalidate L2 page table as it is no longer valid. */
	xtensa_tlb_autorefill_invalidate();
	return exec;
}

static inline void __arch_mem_unmap(void *va)
{
	bool is_exec;
	void *vaddr, *vaddr_uc;

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
		if (sys_cache_is_ptr_cached(va)) {
			vaddr = va;
			vaddr_uc = sys_cache_uncached_ptr_get(va);
		} else {
			vaddr = sys_cache_cached_ptr_get(va);
			vaddr_uc = va;
		}
	} else {
		vaddr = va;
	}

	is_exec = l2_page_table_unmap(xtensa_kernel_ptables, (void *)vaddr);

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP)) {
		(void)l2_page_table_unmap(xtensa_kernel_ptables, (void *)vaddr_uc);
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

#if CONFIG_MP_MAX_NUM_CPUS > 1
	xtensa_mmu_tlb_ipi();
#endif

	sys_cache_data_flush_and_invd_all();
	k_spin_unlock(&xtensa_mmu_lock, key);
}

/* This should be implemented in the SoC layer.
 * This weak version is here to avoid build errors.
 */
void __weak xtensa_mmu_tlb_ipi(void)
{
}

void xtensa_mmu_tlb_shootdown(void)
{
	unsigned int key;

	/* Need to lock interrupts to prevent any context
	 * switching until all the page tables are updated.
	 * Or else we would be switching to another thread
	 * and running that with incorrect page tables
	 * which would result in permission issues.
	 */
	key = arch_irq_lock();

	K_SPINLOCK(&xtensa_mmu_lock) {
		/* We don't have information on which page tables have changed,
		 * so we just invalidate the cache for all L1 page tables.
		 */
		sys_cache_data_invd_range((void *)l1_page_table, sizeof(l1_page_table));
		sys_cache_data_invd_range((void *)l2_page_tables, sizeof(l2_page_tables));
	}

#ifdef CONFIG_USERSPACE
	struct k_thread *thread = _current_cpu->current;

	/* If current thread is a user thread, we need to see if it has
	 * been migrated to another memory domain as the L1 page table
	 * is different from the currently used one.
	 */
	if ((thread->base.user_options & K_USER) == K_USER) {
		uint32_t ptevaddr_entry, ptevaddr,
			thread_ptables, current_ptables;

		/* Need to read the currently used L1 page table.
		 * We know that L1 page table is always mapped at way
		 * MMU_PTE_WAY, so we can skip the probing step by
		 * generating the query entry directly.
		 */
		ptevaddr = (uint32_t)xtensa_ptevaddr_get();
		ptevaddr_entry = XTENSA_MMU_PTE_ENTRY_VADDR(ptevaddr, ptevaddr)
				 | XTENSA_MMU_PTE_WAY;
		current_ptables = xtensa_dtlb_paddr_read(ptevaddr_entry);
		thread_ptables = (uint32_t)thread->arch.ptables;

		if (thread_ptables != current_ptables) {
			/* Need to remap the thread page tables if the ones
			 * indicated by the current thread are different
			 * than the current mapped page table.
			 */
			struct arch_mem_domain *domain =
				&(thread->mem_domain_info.mem_domain->arch);
			xtensa_set_paging(domain->asid, (uint32_t *)thread_ptables);
		}

	}
#endif /* CONFIG_USERSPACE */

	/* L2 are done via autofill, so invalidate autofill TLBs
	 * would refresh the L2 page tables.
	 *
	 * L1 will be refreshed during context switch so no need
	 * to do anything here.
	 */
	xtensa_tlb_autorefill_invalidate();

	arch_irq_unlock(key);
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

static uint32_t *dup_table(void)
{
	uint16_t i, j;
	uint32_t *dst_table = alloc_l1_table();

	if (!dst_table) {
		return NULL;
	}

	for (i = 0; i < XTENSA_L1_PAGE_TABLE_ENTRIES; i++) {
		uint32_t *l2_table, *src_l2_table;

		if (is_pte_illegal(xtensa_kernel_ptables[i]) ||
			(i == XTENSA_MMU_L1_POS(XTENSA_MMU_PTEVADDR))) {
			dst_table[i] = XTENSA_MMU_PTE_ILLEGAL;
			continue;
		}

		src_l2_table = (uint32_t *)(xtensa_kernel_ptables[i] & XTENSA_MMU_PTE_PPN_MASK);
		l2_table = alloc_l2_table();
		if (l2_table == NULL) {
			goto err;
		}

		for (j = 0; j < XTENSA_L2_PAGE_TABLE_ENTRIES; j++) {
			uint32_t original_attr =  XTENSA_MMU_PTE_SW_GET(src_l2_table[j]);

			l2_table[j] =  src_l2_table[j];
			if (original_attr != 0x0) {
				uint8_t ring;

				ring = XTENSA_MMU_PTE_RING_GET(l2_table[j]);
				l2_table[j] =  XTENSA_MMU_PTE_ATTR_SET(l2_table[j], original_attr);
				l2_table[j] =  XTENSA_MMU_PTE_RING_SET(l2_table[j],
						ring == XTENSA_MMU_SHARED_RING ?
						XTENSA_MMU_SHARED_RING : XTENSA_MMU_KERNEL_RING);
			}
		}

		/* The page table is using kernel ASID because we don't
		 * user thread manipulate it.
		 */
		dst_table[i] = XTENSA_MMU_PTE((uint32_t)l2_table, XTENSA_MMU_KERNEL_RING,
					      0, XTENSA_MMU_PAGE_TABLE_ATTR);

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
	__ASSERT(asid_count < (XTENSA_MMU_SHARED_ASID), "Reached maximum of ASID available");

	key = k_spin_lock(&xtensa_mmu_lock);
	/* If this is the default domain, we don't need
	 * to create a new set of page tables. We can just
	 * use the kernel page tables and save memory.
	 */

	if (domain == &k_mem_domain_default) {
		domain->arch.ptables = xtensa_kernel_ptables;
		domain->arch.asid = asid_count;
		goto end;
	}


	ptables = dup_table();

	if (ptables == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	domain->arch.ptables = ptables;
	domain->arch.asid = ++asid_count;

	sys_slist_append(&xtensa_domain_list, &domain->arch.node);

end:
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
		uint32_t l1_pos = XTENSA_MMU_L1_POS(page);
		uint32_t l2_pos = XTENSA_MMU_L2_POS(page);
		/* Make sure we grab a fresh copy of L1 page table */
		sys_cache_data_invd_range((void *)&ptables[l1_pos], sizeof(ptables[0]));

		l2_table = (uint32_t *)(ptables[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);

		sys_cache_data_invd_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));

		pte = XTENSA_MMU_PTE_RING_SET(l2_table[l2_pos], ring);
		pte = XTENSA_MMU_PTE_ATTR_SET(pte, flags);

		l2_table[l2_pos] = pte;

		sys_cache_data_flush_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));

		xtensa_dtlb_vaddr_invalidate((void *)page);
	}

	return ret;
}

static inline int update_region(uint32_t *ptables, uintptr_t start,
				size_t size, uint32_t ring, uint32_t flags,
				uint32_t option)
{
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&xtensa_mmu_lock);

#ifdef CONFIG_XTENSA_MMU_DOUBLE_MAP
	uintptr_t va, va_uc;
	uint32_t new_flags, new_flags_uc;

	if (sys_cache_is_ptr_cached((void *)start)) {
		va = start;
		va_uc = (uintptr_t)sys_cache_uncached_ptr_get((void *)start);
	} else {
		va = (uintptr_t)sys_cache_cached_ptr_get((void *)start);
		va_uc = start;
	}

	new_flags_uc = (flags & ~XTENSA_MMU_PTE_ATTR_CACHED_MASK);
	new_flags = new_flags_uc | XTENSA_MMU_CACHED_WB;

	ret = region_map_update(ptables, va, size, ring, new_flags);

	if (ret == 0) {
		ret = region_map_update(ptables, va_uc, size, ring, new_flags_uc);
	}
#else
	ret = region_map_update(ptables, start, size, ring, flags);
#endif /* CONFIG_XTENSA_MMU_DOUBLE_MAP */

#if CONFIG_MP_MAX_NUM_CPUS > 1
	if ((option & OPTION_NO_TLB_IPI) != OPTION_NO_TLB_IPI) {
		xtensa_mmu_tlb_ipi();
	}
#endif

	sys_cache_data_flush_and_invd_all();
	k_spin_unlock(&xtensa_mmu_lock, key);

	return ret;
}

static inline int reset_region(uint32_t *ptables, uintptr_t start, size_t size, uint32_t option)
{
	return update_region(ptables, start, size,
			     XTENSA_MMU_KERNEL_RING, XTENSA_MMU_PERM_W, option);
}

void xtensa_user_stack_perms(struct k_thread *thread)
{
	(void)memset((void *)thread->stack_info.start,
		     (IS_ENABLED(CONFIG_INIT_STACKS)) ? 0xAA : 0x00,
		     thread->stack_info.size - thread->stack_info.delta);

	update_region(thread_page_tables_get(thread),
		      thread->stack_info.start, thread->stack_info.size,
		      XTENSA_MMU_USER_RING, XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB, 0);
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
			    partition->size, 0);
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				uint32_t partition_id)
{
	struct k_mem_partition *partition = &domain->partitions[partition_id];
	uint32_t ring = K_MEM_PARTITION_IS_USER(partition->attr) ? XTENSA_MMU_USER_RING :
			XTENSA_MMU_KERNEL_RING;

	return update_region(domain->arch.ptables, partition->start,
			     partition->size, ring, partition->attr, 0);
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

	if (is_migration) {
		/* Give access to the thread's stack in its new
		 * memory domain if it is migrating.
		 */
		update_region(thread_page_tables_get(thread),
			      thread->stack_info.start, thread->stack_info.size,
			      XTENSA_MMU_USER_RING,
			      XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
			      OPTION_NO_TLB_IPI);
		/* and reset thread's stack permission in
		 * the old page tables.
		 */
		ret = reset_region(old_ptables,
			thread->stack_info.start,
			thread->stack_info.size, 0);
	}

	/* Need to switch to new page tables if this is
	 * the current thread running.
	 */
	if (thread == _current_cpu->current) {
		xtensa_set_paging(domain->arch.asid, thread->arch.ptables);
	}

#if CONFIG_MP_MAX_NUM_CPUS > 1
	/* Need to tell other CPUs to switch to the new page table
	 * in case the thread is running on one of them.
	 *
	 * Note that there is no need to send TLB IPI if this is
	 * migration as it was sent above during reset_region().
	 */
	if ((thread != _current_cpu->current) && !is_migration) {
		xtensa_mmu_tlb_ipi();
	}
#endif

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
	 *
	 * Note that, since every thread must have an associated memory
	 * domain, removing a thread from domain will be followed by
	 * adding it back to another. So there is no need to send TLB IPI
	 * at this point.
	 */
	return reset_region(domain->arch.ptables,
			    thread->stack_info.start,
			    thread->stack_info.size, OPTION_NO_TLB_IPI);
}

static bool page_validate(uint32_t *ptables, uint32_t page, uint8_t ring, bool write)
{
	uint8_t asid_ring;
	uint32_t rasid, pte, *l2_table;
	uint32_t l1_pos = XTENSA_MMU_L1_POS(page);
	uint32_t l2_pos = XTENSA_MMU_L2_POS(page);

	if (is_pte_illegal(ptables[l1_pos])) {
		return false;
	}

	l2_table = (uint32_t *)(ptables[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);
	pte = l2_table[l2_pos];

	if (is_pte_illegal(pte)) {
		return false;
	}

	asid_ring = 0;
	rasid = xtensa_rasid_get();
	for (uint32_t i = 0; i < 4; i++) {
		if (XTENSA_MMU_PTE_ASID_GET(pte, rasid) == XTENSA_MMU_RASID_ASID_GET(rasid, i)) {
			asid_ring = i;
			break;
		}
	}

	if (ring > asid_ring) {
		return false;
	}

	if (write) {
		return (XTENSA_MMU_PTE_ATTR_GET((pte)) & XTENSA_MMU_PERM_W) != 0;
	}

	return true;
}

static int mem_buffer_validate(const void *addr, size_t size, int write, int ring)
{
	int ret = 0;
	uint8_t *virt;
	size_t aligned_size;
	const struct k_thread *thread = _current;
	uint32_t *ptables = thread_page_tables_get(thread);

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

bool xtensa_mem_kernel_has_access(void *addr, size_t size, int write)
{
	return mem_buffer_validate(addr, size, write, XTENSA_MMU_KERNEL_RING) == 0;
}

int arch_buffer_validate(const void *addr, size_t size, int write)
{
	return mem_buffer_validate(addr, size, write, XTENSA_MMU_USER_RING);
}

void xtensa_swap_update_page_tables(struct k_thread *incoming)
{
	uint32_t *ptables = incoming->arch.ptables;
	struct arch_mem_domain *domain =
		&(incoming->mem_domain_info.mem_domain->arch);

	xtensa_set_paging(domain->asid, ptables);

#ifdef CONFIG_XTENSA_INVALIDATE_MEM_DOMAIN_TLB_ON_SWAP
	struct k_mem_domain *mem_domain = incoming->mem_domain_info.mem_domain;

	for (int idx = 0; idx < mem_domain->num_partitions; idx++) {
		struct k_mem_partition *part = &mem_domain->partitions[idx];
		uintptr_t end = part->start + part->size;

		for (uintptr_t addr = part->start; addr < end; addr += CONFIG_MMU_PAGE_SIZE) {
			xtensa_dtlb_vaddr_invalidate((void *)addr);
		}
	}
#endif
}

#endif /* CONFIG_USERSPACE */
