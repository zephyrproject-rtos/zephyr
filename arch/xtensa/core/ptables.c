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
#include <xtensa_asm2_context.h>
#include <xtensa_mmu_priv.h>

#include <kernel_arch_func.h>
#include <mmu.h>

/** Mask for attributes in PTE */
#define PTE_ATTR_MASK 0x0000000FU

/** Number of bits to shift for attributes in PTE */
#define PTE_ATTR_SHIFT 0U

/** Mask for cache mode in PTE */
#define PTE_ATTR_CACHED_MASK 0x0000000CU

/** Mask for ring in PTE */
#define PTE_RING_MASK 0x00000030U

/** Number of bits to shift for ring in PTE */
#define PTE_RING_SHIFT 4U

/** Number of bits to shift for backup attributes in PTE SW field. */
#define PTE_BCKUP_ATTR_SHIFT (PTE_ATTR_SHIFT + 6U)

/** Mask for backup attributes in PTE SW field. */
#define PTE_BCKUP_ATTR_MASK (PTE_ATTR_MASK << 6U)

/** Number of bits to shift for backup ring value in PTE SW field. */
#define PTE_BCKUP_RING_SHIFT (PTE_RING_SHIFT + 6U)

/** Mask for backup ring value in PTE SW field. */
#define PTE_BCKUP_RING_MASK (PTE_RING_MASK << 6U)

/** Combined attributes and ring mask in PTE. */
#define PTE_PERM_MASK (PTE_ATTR_MASK | PTE_RING_MASK)

/** Number of bits to shift for combined attributes and ring in PTE. */
#define PTE_PERM_SHIFT 0U

/** Combined backup attributes and backup ring mask in PTE. */
#define PTE_BCKUP_PERM_MASK (PTE_BCKUP_ATTR_MASK | PTE_BCKUP_RING_MASK)

/** Number of bits to shift for combined backup attributes and backup ring mask in PTE. */
#define PTE_BCKUP_PERM_SHIFT 6U

/** Construct a page table entry (PTE) with specified backup attributes and ring. */
#define PTE_WITH_BCKUP(paddr, ring, attr, bckup_ring, bckup_attr)                                  \
	(((paddr) & XTENSA_MMU_PTE_PPN_MASK) |                                                     \
	 (((bckup_ring) << PTE_BCKUP_RING_SHIFT) & PTE_BCKUP_RING_MASK) |                          \
	 (((bckup_attr) << PTE_BCKUP_ATTR_SHIFT) & PTE_BCKUP_ATTR_MASK) |                          \
	 (((ring) << PTE_RING_SHIFT) & PTE_RING_MASK) |                                            \
	 (((attr) << PTE_ATTR_SHIFT) & PTE_ATTR_MASK))

/** Construct a page table entry (PTE) */
#define PTE(paddr, ring, attr) PTE_WITH_BCKUP(paddr, ring, attr, RING_KERNEL, PTE_ATTR_ILLEGAL)

/** Get the attributes from a PTE */
#define PTE_ATTR_GET(pte) (((pte) & PTE_ATTR_MASK) >> PTE_ATTR_SHIFT)

/** Set the attributes in a PTE */
#define PTE_ATTR_SET(pte, attr)                                                                    \
	(((pte) & ~PTE_ATTR_MASK) | (((attr) << PTE_ATTR_SHIFT) & PTE_ATTR_MASK))

/** Get the backed up attributes from the PTE SW field. */
#define PTE_BCKUP_ATTR_GET(pte) (((pte) & PTE_BCKUP_ATTR_MASK) >> PTE_BCKUP_ATTR_SHIFT)

/** Get the backed up ring value from the PTE SW field. */
#define PTE_BCKUP_RING_GET(pte) (((pte) & PTE_BCKUP_RING_MASK) >> PTE_BCKUP_RING_SHIFT)

/** Set the ring in a PTE */
#define PTE_RING_SET(pte, ring)                                                                    \
	(((pte) & ~PTE_RING_MASK) | (((ring) << PTE_RING_SHIFT) & PTE_RING_MASK))

/** Get the ring from a PTE */
#define PTE_RING_GET(pte) (((pte) & PTE_RING_MASK) >> PTE_RING_SHIFT)

/** Get the permissions from a PTE */
#define PTE_PERM_GET(pte) (((pte) & PTE_PERM_MASK) >> PTE_PERM_SHIFT)

/** Get the backup permissions from a PTE */
#define PTE_BCKUP_PERM_GET(pte) (((pte) & PTE_BCKUP_PERM_MASK) >> PTE_BCKUP_PERM_SHIFT)

/** Get the ASID from the RASID register corresponding to the ring in a PTE */
#define PTE_ASID_GET(pte, rasid)                                                                   \
	(((rasid) >> ((((pte) & PTE_RING_MASK) >> PTE_RING_SHIFT) * 8)) & 0xFF)

/** Attribute indicating PTE is illegal. */
#define PTE_ATTR_ILLEGAL (BIT(3) | BIT(2))

/** Illegal PTE entry for Level 1 page tables */
#define PTE_L1_ILLEGAL PTE(0, RING_KERNEL, PTE_ATTR_ILLEGAL)

/** Illegal PTE entry for Level 2 page tables */
#define PTE_L2_ILLEGAL PTE(0, RING_KERNEL, PTE_ATTR_ILLEGAL)

/** Ring number in PTE for kernel specific ASID */
#define RING_KERNEL 0

/** Ring number in PTE for user specific ASID */
#define RING_USER 2

/** Ring number in PTE for shared ASID */
#define RING_SHARED 3

#if ((XTENSA_MMU_PAGE_TABLE_ATTR & PTE_ATTR_CACHED_MASK) != 0)
#define PAGE_TABLE_IS_CACHED 1
#endif

/* Skip TLB IPI when updating page tables.
 * This allows us to send IPI only after the last
 * changes of a series.
 */
#define OPTION_NO_TLB_IPI BIT(0)

/* Restore the PTE attributes if they have been
 * stored in the SW bits part in the PTE.
 */
#define OPTION_RESTORE_ATTRS BIT(1)

/* Save the PTE attributes and ring in the SW bits part in the PTE. */
#define OPTION_SAVE_ATTRS BIT(2)

/* Level 1 contains page table entries
 * necessary to map the page table itself.
 */
#define L1_PAGE_TABLE_NUM_ENTRIES 1024U

/* Size of level 1 page table.
 */
#define L1_PAGE_TABLE_SIZE (L1_PAGE_TABLE_NUM_ENTRIES * sizeof(uint32_t))

/* Level 2 contains page table entries
 * necessary to map the page table itself.
 */
#define L2_PAGE_TABLE_NUM_ENTRIES 1024U

/* Size of level 2 page table.
 */
#define L2_PAGE_TABLE_SIZE (L2_PAGE_TABLE_NUM_ENTRIES * sizeof(uint32_t))

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
static uint32_t l1_page_tables[CONFIG_XTENSA_MMU_NUM_L1_TABLES][L1_PAGE_TABLE_NUM_ENTRIES]
		__aligned(KB(4));

/*
 * That is an alias for the page tables set used by the kernel.
 */
uint32_t *xtensa_kernel_ptables = (uint32_t *)l1_page_tables[0];

/*
 * Each table in the level 2 maps a 4Mb memory range. It consists of 1024 entries each one
 * covering a 4Kb page.
 */
static uint32_t l2_page_tables[CONFIG_XTENSA_MMU_NUM_L2_TABLES][L2_PAGE_TABLE_NUM_ENTRIES]
		__aligned(KB(4));

/*
 * This additional variable tracks which l1 tables are in use. This is kept separated from
 * the tables to keep alignment easier.
 *
 * @note: The first bit is set because it is used for the kernel page tables.
 */
static ATOMIC_DEFINE(l1_page_tables_track, CONFIG_XTENSA_MMU_NUM_L1_TABLES);

/*
 * This additional variable tracks which l2 tables are in use. This is kept separated from
 * the tables to keep alignment easier.
 */
static volatile uint8_t l2_page_tables_counter[CONFIG_XTENSA_MMU_NUM_L2_TABLES];

#ifdef CONFIG_XTENSA_MMU_PAGE_TABLE_STATS
/** Maximum number of used L1 page tables. */
static uint32_t l1_page_tables_max_usage;

/** Maximum number of used L2 page tables. */
static uint32_t l2_page_tables_max_usage;
#endif /* CONFIG_XTENSA_MMU_PAGE_TABLE_STATS */

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

static void dup_l2_table_if_needed(uint32_t *l1_table, uint32_t l1_pos);
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

static inline uint32_t restore_pte(uint32_t pte);
static ALWAYS_INLINE void l2_page_tables_counter_inc(uint32_t *l2_table);
static ALWAYS_INLINE void l2_page_tables_counter_dec(uint32_t *l2_table);

/**
 * @brief Check if the page table entry is illegal.
 *
 * @param[in] Page table entry.
 */
static inline bool is_pte_illegal(uint32_t pte)
{
	uint32_t attr = pte & PTE_ATTR_MASK;

	/*
	 * The ISA manual states only 12 and 14 are illegal values.
	 * 13 and 15 are not. So we need to be specific than simply
	 * testing if bits 2 and 3 are set.
	 */
	return (attr == 12) || (attr == 14);
}

/**
 * @brief Initialize all page table entries to the same value (@a val).
 *
 * @param[in] ptable Pointer to page table.
 * @param[in] num_entries Number of page table entries in the page table.
 * @param[in] val Initialize all PTEs with this value.
 */
static void init_page_table(uint32_t *ptable, size_t num_entries, uint32_t val)
{
	int i;

	for (i = 0; i < num_entries; i++) {
		ptable[i] = val;
	}
}

/**
 * @brief Find the L2 table counter array index from L2 table pointer.
 *
 * @param[in] l2_table Pointer to L2 table.
 *
 * @note This does not check if the incoming L2 table pointer is a valid
 *       L2 table.
 *
 * @return Index to the L2 table counter array.
 */
static inline int l2_table_to_counter_pos(uint32_t *l2_table)
{
	return (l2_table - (uint32_t *)l2_page_tables) / (L2_PAGE_TABLE_NUM_ENTRIES);
}

static inline uint32_t *alloc_l2_table(void)
{
	uint16_t idx;
	uint32_t *ret = NULL;

	for (idx = 0; idx < CONFIG_XTENSA_MMU_NUM_L2_TABLES; idx++) {
		if (l2_page_tables_counter[idx] == 0) {
			l2_page_tables_counter[idx] = 1;
			ret = (uint32_t *)&l2_page_tables[idx];
			break;
		}
	}

#ifdef CONFIG_XTENSA_MMU_PAGE_TABLE_STATS
	uint32_t cur_l2_usage = 0;

	/* Calculate how many L2 page tables are being used now. */
	for (idx = 0; idx < CONFIG_XTENSA_MMU_NUM_L2_TABLES; idx++) {
		if (l2_page_tables_counter[idx] > 0) {
			cur_l2_usage++;
		}
	}

	/* Store the bigger number. */
	l2_page_tables_max_usage = MAX(l2_page_tables_max_usage, cur_l2_usage);
#endif /* CONFIG_XTENSA_MMU_PAGE_TABLE_STATS */

	return ret;
}

static void map_memory_range(const uint32_t start, const uint32_t end,
			     const uint32_t attrs, const uint32_t options)
{
	uint32_t page;
	bool shared = !!(attrs & XTENSA_MMU_MAP_SHARED);
	bool do_save_attrs = (options & OPTION_SAVE_ATTRS) == OPTION_SAVE_ATTRS;

	uint32_t ring = shared ? RING_SHARED : RING_KERNEL;
	uint32_t bckup_attrs = do_save_attrs ? attrs : PTE_ATTR_ILLEGAL;
	uint32_t bckup_ring = do_save_attrs ? ring : RING_KERNEL;

	for (page = start; page < end; page += CONFIG_MMU_PAGE_SIZE) {
		uint32_t *l2_table;
		uint32_t pte = PTE_WITH_BCKUP(page, ring, attrs, bckup_ring, bckup_attrs);
		uint32_t l2_pos = XTENSA_MMU_L2_POS(page);
		uint32_t l1_pos = XTENSA_MMU_L1_POS(page);

		if (is_pte_illegal(xtensa_kernel_ptables[l1_pos])) {
			l2_table = alloc_l2_table();

			__ASSERT(l2_table != NULL,
				 "There is no l2 page table available to map 0x%08x\n", page);

			init_page_table(l2_table, L2_PAGE_TABLE_NUM_ENTRIES, PTE_L2_ILLEGAL);

			xtensa_kernel_ptables[l1_pos] =
				PTE((uint32_t)l2_table, RING_KERNEL, XTENSA_MMU_PAGE_TABLE_ATTR);
		}

		l2_table = (uint32_t *)(xtensa_kernel_ptables[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);
		l2_table[l2_pos] = pte;
	}
}

static void map_memory(const uint32_t start, const uint32_t end,
		       const uint32_t attrs, const uint32_t options)
{
#ifdef CONFIG_XTENSA_MMU_DOUBLE_MAP
	uint32_t uc_attrs = attrs & ~PTE_ATTR_CACHED_MASK;
	uint32_t c_attrs = attrs | XTENSA_MMU_CACHED_WB;

	if (sys_cache_is_ptr_uncached((void *)start)) {
		map_memory_range(start, end, uc_attrs, options);

		map_memory_range(POINTER_TO_UINT(sys_cache_cached_ptr_get((void *)start)),
				 POINTER_TO_UINT(sys_cache_cached_ptr_get((void *)end)),
				 c_attrs, options);
	} else if (sys_cache_is_ptr_cached((void *)start)) {
		map_memory_range(start, end, c_attrs, options);

		map_memory_range(POINTER_TO_UINT(sys_cache_uncached_ptr_get((void *)start)),
				 POINTER_TO_UINT(sys_cache_uncached_ptr_get((void *)end)),
				 uc_attrs, options);
	} else
#endif
	{
		map_memory_range(start, end, attrs, options);
	}
}

static void xtensa_init_page_tables(void)
{
	volatile uint8_t entry;
	static bool already_inited;

	if (already_inited) {
		return;
	}
	already_inited = true;

	init_page_table(xtensa_kernel_ptables, L1_PAGE_TABLE_NUM_ENTRIES, PTE_L1_ILLEGAL);
	atomic_set_bit(l1_page_tables_track, 0);

	for (entry = 0; entry < ARRAY_SIZE(mmu_zephyr_ranges); entry++) {
		const struct xtensa_mmu_range *range = &mmu_zephyr_ranges[entry];

		map_memory(range->start, range->end, range->attrs, OPTION_SAVE_ATTRS);
	}

	for (entry = 0; entry < xtensa_soc_mmu_ranges_num; entry++) {
		const struct xtensa_mmu_range *range = &xtensa_soc_mmu_ranges[entry];

		map_memory(range->start, range->end, range->attrs, OPTION_SAVE_ATTRS);
	}

	/* Finally, the direct-mapped pages used in the page tables
	 * must be fixed up to use the same cache attribute (but these
	 * must be writable, obviously).  They shouldn't be left at
	 * the default.
	 */
	map_memory_range((uint32_t) &l1_page_tables[0],
			 (uint32_t) &l1_page_tables[CONFIG_XTENSA_MMU_NUM_L1_TABLES],
			 XTENSA_MMU_PAGE_TABLE_ATTR | XTENSA_MMU_PERM_W, OPTION_SAVE_ATTRS);
	map_memory_range((uint32_t) &l2_page_tables[0],
			 (uint32_t) &l2_page_tables[CONFIG_XTENSA_MMU_NUM_L2_TABLES],
			 XTENSA_MMU_PAGE_TABLE_ATTR | XTENSA_MMU_PERM_W, OPTION_SAVE_ATTRS);

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_flush_all();
	}
}

__weak void arch_xtensa_mmu_post_init(bool is_core0)
{
	ARG_UNUSED(is_core0);
}

void xtensa_mmu_init(void)
{
	xtensa_init_page_tables();

	xtensa_mmu_init_paging();

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
	xtensa_mmu_init_paging();

#ifdef CONFIG_USERSPACE
	struct k_thread *thread = _current_cpu->current;
	struct arch_mem_domain *domain =
			&(thread->mem_domain_info.mem_domain->arch);


	/* Set the page table for current context */
	xtensa_mmu_set_paging(domain);
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
			      uint32_t attrs, bool is_user)
{
	uint32_t l1_pos = XTENSA_MMU_L1_POS((uint32_t)vaddr);
	uint32_t l2_pos = XTENSA_MMU_L2_POS((uint32_t)vaddr);
	uint32_t *l2_table;

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_invd_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));
	}

	if (is_pte_illegal(l1_table[l1_pos])) {
		l2_table = alloc_l2_table();

		if (l2_table == NULL) {
			return false;
		}

		init_page_table(l2_table, L2_PAGE_TABLE_NUM_ENTRIES, PTE_L2_ILLEGAL);

		l1_table[l1_pos] = PTE((uint32_t)l2_table, RING_KERNEL, XTENSA_MMU_PAGE_TABLE_ATTR);

		if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
			sys_cache_data_flush_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));
		}
	}
#ifdef CONFIG_USERSPACE
	else {
		dup_l2_table_if_needed(l1_table, l1_pos);
	}
#endif

	l2_table = (uint32_t *)(l1_table[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);
	l2_table[l2_pos] = PTE(phys, is_user ? RING_USER : RING_KERNEL, attrs);

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_flush_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));
	}

	xtensa_tlb_autorefill_invalidate();

	return true;
}

static inline void __arch_mem_map(void *va, uintptr_t pa, uint32_t new_attrs, bool is_user)
{
	bool ret;
	void *vaddr, *vaddr_uc;
	uintptr_t paddr, paddr_uc;
	uint32_t attrs, attrs_uc;

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

		attrs_uc = (new_attrs & ~PTE_ATTR_CACHED_MASK);
		attrs = attrs_uc | XTENSA_MMU_CACHED_WB;
	} else {
		vaddr = va;
		paddr = pa;
		attrs = new_attrs;
	}

	ret = l2_page_table_map(xtensa_kernel_ptables, (void *)vaddr, paddr,
				attrs, is_user);
	__ASSERT(ret, "Cannot map virtual address (%p)", va);

	if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP) && ret) {
		ret = l2_page_table_map(xtensa_kernel_ptables, (void *)vaddr_uc, paddr_uc,
					attrs_uc, is_user);
		__ASSERT(ret, "Cannot map virtual address (%p)", vaddr_uc);
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
						attrs, is_user);
			__ASSERT(ret, "Cannot map virtual address (%p) for domain %p",
				 vaddr, domain);

			if (IS_ENABLED(CONFIG_XTENSA_MMU_DOUBLE_MAP) && ret) {
				ret = l2_page_table_map(domain->ptables,
							(void *)vaddr_uc, paddr_uc,
							attrs_uc, is_user);
				__ASSERT(ret, "Cannot map virtual address (%p) for domain %p",
					 vaddr_uc, domain);
			}

			/* We may have made a copy of L2 table containing VECBASE.
			 * So we need to re-calculate the static TLBs so the correct ones
			 * will be placed in the TLB cache when swapping page tables.
			 */
			xtensa_mmu_compute_domain_regs(domain);
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
	uint32_t attrs = 0;
	k_spinlock_key_t key;
	bool is_user;

	if (size == 0) {
		LOG_ERR("Cannot map physical memory at 0x%08X: invalid "
			"zero size", (uint32_t)phys);
		k_panic();
	}

	switch (flags & K_MEM_CACHE_MASK) {

	case K_MEM_CACHE_WB:
		attrs |= XTENSA_MMU_CACHED_WB;
		break;
	case K_MEM_CACHE_WT:
		attrs |= XTENSA_MMU_CACHED_WT;
		break;
	case K_MEM_CACHE_NONE:
		__fallthrough;
	default:
		break;
	}

	if ((flags & K_MEM_PERM_RW) == K_MEM_PERM_RW) {
		attrs |= XTENSA_MMU_PERM_W;
	}
	if ((flags & K_MEM_PERM_EXEC) == K_MEM_PERM_EXEC) {
		attrs |= XTENSA_MMU_PERM_X;
	}

	is_user = (flags & K_MEM_PERM_USER) == K_MEM_PERM_USER;

	key = k_spin_lock(&xtensa_mmu_lock);

	while (rem_size > 0) {
		__arch_mem_map((void *)va, pa, attrs, is_user);

		rem_size -= (rem_size >= KB(4)) ? KB(4) : rem_size;
		va += KB(4);
		pa += KB(4);
	}

#if CONFIG_MP_MAX_NUM_CPUS > 1
	xtensa_mmu_tlb_ipi();
#endif

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_flush_and_invd_all();
	}

	k_spin_unlock(&xtensa_mmu_lock, key);
}

/**
 * @brief Unmap an entry from L2 table.
 *
 * @param[in] l1_table Pointer to L1 page table.
 * @param[in] vaddr Address to be unmapped.
 *
 * @note If all L2 PTEs in the L2 table are illegal, the L2 table will be
 *       unmapped from L1 and is returned to the pool.
 */
static void l2_page_table_unmap(uint32_t *l1_table, void *vaddr)
{
	uint32_t l1_pos = XTENSA_MMU_L1_POS((uint32_t)vaddr);
	uint32_t l2_pos = XTENSA_MMU_L2_POS((uint32_t)vaddr);
	uint32_t *l2_table;
	bool exec = false;

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_invd_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));
	}

	if (is_pte_illegal(l1_table[l1_pos])) {
		/* We shouldn't be unmapping an illegal entry.
		 * Return true so that we can invalidate ITLB too.
		 */
		return;
	}

#ifdef CONFIG_USERSPACE
	dup_l2_table_if_needed(l1_table, l1_pos);
#endif

	l2_table = (uint32_t *)(l1_table[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_invd_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));
	}

	exec = (l2_table[l2_pos] & XTENSA_MMU_PERM_X) == XTENSA_MMU_PERM_X;

	/* Restore the PTE to previous ring and attributes. */
	l2_table[l2_pos] = restore_pte(l2_table[l2_pos]);

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_flush_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));
	}

	for (l2_pos = 0; l2_pos < L2_PAGE_TABLE_NUM_ENTRIES; l2_pos++) {
		if (!is_pte_illegal(l2_table[l2_pos])) {
			/* If any PTE is mapped (== not illegal), we need to
			 * keep this L2 table.
			 */
			goto end;
		}
	}

	/* All L2 PTE are illegal (== nothing mapped), we can safely remove
	 * the L2 table mapping in L1 table and return the L2 table to the pool.
	 */
	l1_table[l1_pos] = PTE_L1_ILLEGAL;

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_flush_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));
	}

	l2_page_tables_counter_dec(l2_table);

end:
	/* Need to invalidate TLB associated with the unmapped address. */
	xtensa_dtlb_vaddr_invalidate(vaddr);
	if (exec) {
		xtensa_itlb_vaddr_invalidate(vaddr);
	}
}

static inline void __arch_mem_unmap(void *va)
{
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

	l2_page_table_unmap(xtensa_kernel_ptables, (void *)vaddr);

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

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_flush_and_invd_all();
	}

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

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		K_SPINLOCK(&xtensa_mmu_lock) {
			/* We don't have information on which page tables have changed,
			 * so we just invalidate the cache for all L1 page tables.
			 */
			sys_cache_data_invd_range((void *)l1_page_tables, sizeof(l1_page_tables));
			sys_cache_data_invd_range((void *)l2_page_tables, sizeof(l2_page_tables));
		}
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
			xtensa_mmu_set_paging(domain);
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

/**
 * @brief Restore PTE ring and attributes from those stashed in SW bits.
 *
 * @param[in] pte Page table entry to be restored.
 *
 * @note This does not check if the SW bits contain ring and attributes to be
 *       restored.
 *
 * @return PTE with restored ring and attributes. Illegal entry if original is
 *         illegal.
 */
static inline uint32_t restore_pte(uint32_t pte)
{
	uint32_t restored_pte;

	uint32_t bckup_attr = PTE_BCKUP_ATTR_GET(pte);
	uint32_t bckup_ring = PTE_BCKUP_RING_GET(pte);

	restored_pte = pte;
	restored_pte = PTE_ATTR_SET(restored_pte, bckup_attr);
	restored_pte = PTE_RING_SET(restored_pte, bckup_ring);

	return restored_pte;
}

/**
 * @brief Test if the L2 table is inside the L2 page table array.
 *
 * @param[in] l2_table Pointer to L2 table.
 *
 * @return True if within array, false otherwise.
 */
static bool is_l2_table_inside_array(uint32_t *l2_table)
{
	uintptr_t l2_table_begin = (uintptr_t)l2_page_tables;
	uintptr_t l2_table_end = l2_table_begin + sizeof(l2_page_tables);
	uintptr_t addr = (uintptr_t)l2_table;

	return (addr >= l2_table_begin) && (addr < l2_table_end);
}

static ALWAYS_INLINE void l2_page_tables_counter_inc(uint32_t *l2_table)
{
	if (is_l2_table_inside_array(l2_table)) {
		l2_page_tables_counter[l2_table_to_counter_pos(l2_table)]++;
	}
}

static ALWAYS_INLINE void l2_page_tables_counter_dec(uint32_t *l2_table)
{
	if (is_l2_table_inside_array(l2_table)) {
		l2_page_tables_counter[l2_table_to_counter_pos(l2_table)]--;
	}
}

#ifdef CONFIG_USERSPACE

static inline uint32_t *thread_page_tables_get(const struct k_thread *thread)
{
	if ((thread->base.user_options & K_USER) != 0U) {
		return thread->arch.ptables;
	}

	return xtensa_kernel_ptables;
}

static inline uint32_t *alloc_l1_table(void)
{
	uint16_t idx;
	uint32_t *ret = NULL;

	for (idx = 0; idx < CONFIG_XTENSA_MMU_NUM_L1_TABLES; idx++) {
		if (!atomic_test_and_set_bit(l1_page_tables_track, idx)) {
			ret = (uint32_t *)&l1_page_tables[idx];
			break;
		}
	}

#ifdef CONFIG_XTENSA_MMU_PAGE_TABLE_STATS
	uint32_t cur_l1_usage = 0;

	/* Calculate how many L1 page tables are being used now. */
	for (idx = 0; idx < CONFIG_XTENSA_MMU_NUM_L1_TABLES; idx++) {
		if (atomic_test_bit(l1_page_tables_track, idx)) {
			cur_l1_usage++;
		}
	}

	/* Store the bigger number. */
	l1_page_tables_max_usage = MAX(l1_page_tables_max_usage, cur_l1_usage);
#endif /* CONFIG_XTENSA_MMU_PAGE_TABLE_STATS */

	return ret;
}

/**
 * Given page table position, calculate the corresponding virtual address.
 *
 * @param l1_pos Position in L1 page table.
 * @param l2_pos Position in L2 page table.
 * @return Virtual address.
 */
static ALWAYS_INLINE uint32_t vaddr_from_pt_pos(uint32_t l1_pos, uint32_t l2_pos)
{
	return (l1_pos << 22U) | (l2_pos << 12U);
}

static uint32_t *dup_l2_table(uint32_t *src_l2_table)
{
	uint32_t *l2_table;

	l2_table = alloc_l2_table();
	if (l2_table == NULL) {
		return NULL;
	}

	for (int j = 0; j < L2_PAGE_TABLE_NUM_ENTRIES; j++) {
		uint32_t bckup_attr = PTE_BCKUP_ATTR_GET(src_l2_table[j]);

		if (bckup_attr != PTE_ATTR_ILLEGAL) {
			l2_table[j] = restore_pte(src_l2_table[j]);
		} else {
			l2_table[j] = PTE_L2_ILLEGAL;
		}
	}

	return l2_table;
}

static uint32_t *dup_l1_table(void)
{
	uint32_t *l1_table = alloc_l1_table();

	if (!l1_table) {
		return NULL;
	}

	for (uint32_t l1_pos = 0; l1_pos < L1_PAGE_TABLE_NUM_ENTRIES; l1_pos++) {
		if (is_pte_illegal(xtensa_kernel_ptables[l1_pos]) ||
			(l1_pos == XTENSA_MMU_L1_POS(XTENSA_MMU_PTEVADDR))) {
			l1_table[l1_pos] = PTE_L1_ILLEGAL;
		} else {
			uint32_t *l2_table, *src_l2_table;
			bool l2_need_dup = false;

			src_l2_table = (uint32_t *)(xtensa_kernel_ptables[l1_pos] &
						    XTENSA_MMU_PTE_PPN_MASK);

			/* Need to check if the L2 table has been modified between boot and
			 * this function call. We do not want to inherit any changes in
			 * between (e.g. arch_mem_map done to kernel page tables).
			 * If no modifications have been done, we can re-use this L2 table.
			 * Otherwise, we need to duplicate it.
			 */
			for (uint32_t l2_pos = 0; l2_pos < L2_PAGE_TABLE_NUM_ENTRIES; l2_pos++) {
				uint32_t vaddr;
				uint32_t perm = PTE_PERM_GET(src_l2_table[l2_pos]);
				uint32_t bckup_perm = PTE_BCKUP_PERM_GET(src_l2_table[l2_pos]);

				/* Current and backup permissions do not match. Must duplicate. */
				if (perm != bckup_perm) {
					l2_need_dup = true;
					break;
				}

				/* At boot, everything are identity mapped. So if physical and
				 * virtual addresses do not match in the PTE, we need to
				 * duplicate the L2 table.
				 */
				vaddr = vaddr_from_pt_pos(l1_pos, l2_pos);
				if ((src_l2_table[l2_pos] & XTENSA_MMU_PTE_PPN_MASK) != vaddr) {
					l2_need_dup = true;
					break;
				}
			}

			if (l2_need_dup) {
				l2_table = dup_l2_table(src_l2_table);
			} else {
				l2_table = src_l2_table;
				l2_page_tables_counter_inc(src_l2_table);
			}

			/* The page table is using kernel ASID because we don't want
			 * user threads to manipulate it.
			 */
			l1_table[l1_pos] =
				PTE((uint32_t)l2_table, RING_KERNEL, XTENSA_MMU_PAGE_TABLE_ATTR);
		}
	}

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_flush_range((void *)l1_table, L1_PAGE_TABLE_SIZE);
	}

	return l1_table;
}

static void dup_l2_table_if_needed(uint32_t *l1_table, uint32_t l1_pos)
{
	uint32_t *l2_table, *src_l2_table;

	src_l2_table = (uint32_t *)(l1_table[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);

	if (l2_page_tables_counter[l2_table_to_counter_pos(src_l2_table)] == 1) {
		/* Only one user of L2 table, no need to duplicate. */
		return;
	}

	l2_table = dup_l2_table(src_l2_table);

	/* The page table is using kernel ASID because we don't
	 * user thread manipulate it.
	 */
	l1_table[l1_pos] = PTE((uint32_t)l2_table, RING_KERNEL, XTENSA_MMU_PAGE_TABLE_ATTR);

	l2_page_tables_counter_dec(src_l2_table);

	sys_cache_data_flush_range((void *)l2_table, L2_PAGE_TABLE_SIZE);
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


	ptables = dup_l1_table();

	if (ptables == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	domain->arch.ptables = ptables;
	domain->arch.asid = ++asid_count;

	sys_slist_append(&xtensa_domain_list, &domain->arch.node);

end:
	xtensa_mmu_compute_domain_regs(&domain->arch);
	ret = 0;

err:
	k_spin_unlock(&xtensa_mmu_lock, key);

	return ret;
}

static void region_map_update(uint32_t *l1_table, uintptr_t start,
			      size_t size, uint32_t ring, uint32_t flags, uint32_t option)
{
	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		uint32_t *l2_table, pte;
		uint32_t new_ring, new_attrs;
		uint32_t page = start + offset;
		uint32_t l1_pos = XTENSA_MMU_L1_POS(page);
		uint32_t l2_pos = XTENSA_MMU_L2_POS(page);

		if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
			/* Make sure we grab a fresh copy of L1 page table */
			sys_cache_data_invd_range((void *)&l1_table[l1_pos], sizeof(l1_table[0]));
		}

#ifdef CONFIG_USERSPACE
		dup_l2_table_if_needed(l1_table, l1_pos);
#endif

		l2_table = (uint32_t *)(l1_table[l1_pos] & XTENSA_MMU_PTE_PPN_MASK);

		if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
			sys_cache_data_invd_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));
		}

		pte = l2_table[l2_pos];

		if ((option & OPTION_RESTORE_ATTRS) == OPTION_RESTORE_ATTRS) {
			new_attrs = PTE_BCKUP_ATTR_GET(pte);
			new_ring = PTE_BCKUP_RING_GET(pte);
		} else {
			new_attrs = flags;
			new_ring = ring;
		}

		pte = PTE_RING_SET(pte, new_ring);
		pte = PTE_ATTR_SET(pte, new_attrs);

		l2_table[l2_pos] = pte;

		if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
			sys_cache_data_flush_range((void *)&l2_table[l2_pos], sizeof(l2_table[0]));
		}

		xtensa_dtlb_vaddr_invalidate((void *)page);
	}
}

static void update_region(uint32_t *ptables, uintptr_t start, size_t size,
			  uint32_t ring, uint32_t flags, uint32_t option)
{
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

	new_flags_uc = (flags & ~PTE_ATTR_CACHED_MASK);
	new_flags = new_flags_uc | XTENSA_MMU_CACHED_WB;

	region_map_update(ptables, va, size, ring, new_flags, option);
	region_map_update(ptables, va_uc, size, ring, new_flags_uc, option);
#else
	region_map_update(ptables, start, size, ring, flags, option);
#endif /* CONFIG_XTENSA_MMU_DOUBLE_MAP */

#if CONFIG_MP_MAX_NUM_CPUS > 1
	if ((option & OPTION_NO_TLB_IPI) != OPTION_NO_TLB_IPI) {
		xtensa_mmu_tlb_ipi();
	}
#endif

	if (IS_ENABLED(PAGE_TABLE_IS_CACHED)) {
		sys_cache_data_flush_and_invd_all();
	}
	k_spin_unlock(&xtensa_mmu_lock, key);
}

static inline void reset_region(uint32_t *ptables, uintptr_t start, size_t size, uint32_t option)
{
	update_region(ptables, start, size, RING_KERNEL, XTENSA_MMU_PERM_W,
		      option | OPTION_RESTORE_ATTRS);
}

void xtensa_user_stack_perms(struct k_thread *thread)
{
	(void)memset((void *)thread->stack_info.start,
		     (IS_ENABLED(CONFIG_INIT_STACKS)) ? 0xAA : 0x00,
		     thread->stack_info.size - thread->stack_info.delta);

	update_region(thread_page_tables_get(thread),
		      thread->stack_info.start, thread->stack_info.size,
		      RING_USER, XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB, 0);
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
	reset_region(domain->arch.ptables, partition->start, partition->size, 0);

	return 0;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				uint32_t partition_id)
{
	struct k_mem_partition *partition = &domain->partitions[partition_id];
	uint32_t ring = K_MEM_PARTITION_IS_USER(partition->attr) ? RING_USER : RING_KERNEL;

	update_region(domain->arch.ptables, partition->start, partition->size, ring,
		      partition->attr, 0);

	/* We may have made a copy of L2 table containing VECBASE.
	 * So we need to re-calculate the static TLBs so the correct ones
	 * will be placed in the TLB cache when swapping page tables.
	 */
	xtensa_mmu_compute_domain_regs(&domain->arch);

	return 0;
}

/* These APIs don't need to do anything */
int arch_mem_domain_thread_add(struct k_thread *thread)
{
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
			      RING_USER,
			      XTENSA_MMU_PERM_W | XTENSA_MMU_CACHED_WB,
			      OPTION_NO_TLB_IPI);
		/* and reset thread's stack permission in
		 * the old page tables.
		 */
		reset_region(old_ptables, thread->stack_info.start, thread->stack_info.size, 0);
	}

	/* Need to switch to new page tables if this is
	 * the current thread running.
	 */
	if (thread == _current_cpu->current) {
		struct arch_mem_domain *arch_domain = &(domain->arch);

		xtensa_mmu_set_paging(arch_domain);
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

	return 0;
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
	reset_region(domain->arch.ptables,
		     thread->stack_info.start,
		     thread->stack_info.size, OPTION_NO_TLB_IPI);

	return 0;
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
		if (PTE_ASID_GET(pte, rasid) == XTENSA_MMU_RASID_ASID_GET(rasid, i)) {
			asid_ring = i;
			break;
		}
	}

	if (ring > asid_ring) {
		return false;
	}

	if (write) {
		return (PTE_ATTR_GET((pte)) & XTENSA_MMU_PERM_W) != 0;
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

bool xtensa_mem_kernel_has_access(const void *addr, size_t size, int write)
{
	return mem_buffer_validate(addr, size, write, RING_KERNEL) == 0;
}

int arch_buffer_validate(const void *addr, size_t size, int write)
{
	return mem_buffer_validate(addr, size, write, RING_USER);
}

void xtensa_exc_dtlb_multihit_handle(void)
{
	/* For some unknown reasons, using xtensa_dtlb_probe() would result in
	 * QEMU raising privileged instruction exception. So for now, just
	 * invalidate all auto-refilled DTLBs.
	 */

	xtensa_dtlb_autorefill_invalidate();
}

bool xtensa_exc_load_store_ring_error_check(void *bsa_p)
{
	uintptr_t ring, vaddr;
	_xtensa_irq_bsa_t *bsa = (_xtensa_irq_bsa_t *)bsa_p;

	ring = (bsa->ps & XCHAL_PS_RING_MASK) >> XCHAL_PS_RING_SHIFT;

	if (ring != RING_USER) {
		return true;
	}

	vaddr = bsa->excvaddr;

	if (arch_buffer_validate((void *)vaddr, sizeof(uint32_t), false) != 0) {
		/* User thread DO NOT have access to this memory according to
		 * page table. so this is a true access violation.
		 */
		return true;
	}

	/* User thread has access to this memory according to
	 * page table. so this is not a true access violation.
	 *
	 * Now we need to find all associated auto-refilled DTLBs
	 * and invalidate them. So that hardware can reload
	 * from page table with correct permission for user
	 * thread.
	 */
	while (true) {
		uint32_t dtlb_entry = xtensa_dtlb_probe((void *)vaddr);

		if ((dtlb_entry & XTENSA_MMU_PDTLB_HIT) != XTENSA_MMU_PDTLB_HIT) {
			/* No more DTLB entry found. */
			return false;
		}

		if ((dtlb_entry & XTENSA_MMU_PDTLB_WAY_MASK) >=
				XTENSA_MMU_NUM_TLB_AUTOREFILL_WAYS) {
			return false;
		}

		xtensa_dtlb_entry_invalidate_sync(dtlb_entry);
	}

	return false;
}

#ifdef CONFIG_XTENSA_MMU_FLUSH_AUTOREFILL_DTLBS_ON_SWAP
/* This is only used when swapping page tables and auto-refill DTLBs
 * needing to be invalidated. Otherwise, SWAP_PAGE_TABLE assembly
 * is used to avoid a function call.
 */
void xtensa_swap_update_page_tables(struct k_thread *incoming)
{
	struct arch_mem_domain *domain =
		&(incoming->mem_domain_info.mem_domain->arch);

	xtensa_mmu_set_paging(domain);

	xtensa_dtlb_autorefill_invalidate();
}
#endif

#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_XTENSA_MMU_PAGE_TABLE_STATS
void xtensa_mmu_page_table_stats_get(struct xtensa_mmu_page_table_stats *stats)
{
	int idx;
	uint32_t cur_l1_usage = 0;
	uint32_t cur_l2_usage = 0;

	__ASSERT_NO_MSG(stats != NULL);

	/* Calculate how many L1 page tables are being used now. */
	for (idx = 0; idx < CONFIG_XTENSA_MMU_NUM_L1_TABLES; idx++) {
		if (atomic_test_bit(l1_page_tables_track, idx)) {
			cur_l1_usage++;
		}
	}

	/* Calculate how many L2 page tables are being used now. */
	for (idx = 0; idx < CONFIG_XTENSA_MMU_NUM_L2_TABLES; idx++) {
		if (l2_page_tables_counter[idx] > 0) {
			cur_l2_usage++;
		}
	}

	/* Store the statistics into the output. */
	stats->cur_num_l1_alloced = cur_l1_usage;
	stats->cur_num_l2_alloced = cur_l2_usage;
	stats->max_num_l1_alloced = l1_page_tables_max_usage;
	stats->max_num_l2_alloced = l2_page_tables_max_usage;
}
#endif /* CONFIG_XTENSA_MMU_PAGE_TABLE_STATS */
