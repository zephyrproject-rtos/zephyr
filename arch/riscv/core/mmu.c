/*
 * Copyright (c) 2026 Alexios Lyrakis <alexios.lyrakis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <kernel_arch_func.h>
#include <kernel_arch_interface.h>
#include <kernel_internal.h>
#include <mmu.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <string.h>

#include "mmu.h"

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* Page table storage and management */
static uint64_t xlat_tables[CONFIG_MAX_XLAT_TABLES * RISCV_PGTABLE_ENTRIES] __aligned(PAGE_SIZE);
static int32_t xlat_use_count[CONFIG_MAX_XLAT_TABLES];
static struct k_spinlock xlat_lock;

/*
 * Pointer to the kernel root page table.  Allocated once during
 * arch_mem_init() and kept here so that mapping functions can access it
 * before the MMU is enabled (i.e. before SATP is written).
 */
static uint64_t *kernel_root_table;

#ifdef CONFIG_USERSPACE
/*
 * Cached kernel SATP value.  Written once by arch_mem_init() and read
 * by isr.S on every trap entry from user mode to restore the kernel
 * address space before executing any kernel code beyond the trap page.
 */
uintptr_t z_riscv_kernel_satp_val;
#endif /* CONFIG_USERSPACE */

/* ASID management */
#define ASID_INVALID    0 /* Invalid ASID (reserved) */
#define KERNEL_ASID     1 /* Kernel always uses ASID 1 */
#define ASID_FIRST_USER 2U
#define ASID_MAX        BIT(16)

#ifdef CONFIG_USERSPACE
static uint16_t next_asid = ASID_FIRST_USER;
static struct k_spinlock asid_lock;

/*
 * List of all active memory domains.  Protected by xlat_lock.
 * Used by sync_domains() to propagate new kernel mappings to all
 * domain private page tables.
 */
static sys_slist_t domain_list;

/* Forward declarations */
static int domain_privatize_kernel_text(uint64_t *domain_root);
static void sync_domains(void);

/*
 * Allocate a unique ASID for a new memory domain.  ASIDs allow the TLB to
 * cache entries for multiple address spaces simultaneously, avoiding a full
 * TLB flush on every domain switch.
 */
static uint16_t alloc_asid(void)
{
	k_spinlock_key_t key;
	uint16_t asid;

	key = k_spin_lock(&asid_lock);

	asid = next_asid++;

	/*
	 * TODO: implement ASID recycling with global TLB flush when the
	 * space is exhausted. For now just panic; 65533 domains is
	 * sufficient for any practical embedded system.
	 */
	if (next_asid >= ASID_MAX) {
		k_panic();
	}

	k_spin_unlock(&asid_lock, key);

	return asid;
}
#endif /* CONFIG_USERSPACE */

/* Usage count tracking */
#define XLAT_PTE_COUNT_MASK GENMASK(15, 0)
#define XLAT_REF_COUNT_UNIT (1 << 16)

/*
 * Allocate a free page table from the static xlat_tables pool and return a
 * pointer to it, zeroed and ready for use.  The use count is left at 0 —
 * the caller must call inc_table_ref() (or rely on set_pte() to do so) before
 * installing the table into a parent PTE.  Must be called with xlat_lock held.
 */
static uint64_t *new_table(void)
{
	uint64_t *table;
	unsigned int i;

	/* Look for a free table. */
	for (i = 0U; i < CONFIG_MAX_XLAT_TABLES; i++) {
		if (xlat_use_count[i] == 0) {
			table = &xlat_tables[i * RISCV_PGTABLE_ENTRIES];
			/*
			 * Leave the initial use count at 0.  The caller is
			 * responsible for incrementing the reference count via
			 * inc_table_ref() before the table becomes reachable
			 * from any parent PTE (done automatically by set_pte()
			 * for non-root tables, or explicitly for root tables).
			 * This prevents the double-count bug where new_table()
			 * set XLAT_REF_COUNT_UNIT and set_pte() added another
			 * XLAT_REF_COUNT_UNIT, making dec_table_ref() unable
			 * to reach 0.
			 */
			MMU_DEBUG("allocating table [%d]%p\n", i, table);

			/* Clear the new table */
			memset(table, 0, PAGE_SIZE);
			return table;
		}
	}

	printk("FATAL: CONFIG_MAX_XLAT_TABLES (%d) is too small\n",
	       CONFIG_MAX_XLAT_TABLES);
	k_panic();
	return NULL;
}

/*
 * Return the index of a page table in the xlat_tables pool.
 */
static inline unsigned int table_index(uint64_t *pte)
{
	unsigned int i = (pte - xlat_tables) / RISCV_PGTABLE_ENTRIES;

	__ASSERT(i < CONFIG_MAX_XLAT_TABLES, "table %p out of range", pte);
	return i;
}

/*
 * Adjust the combined use count of a page table and return the new value.
 * The 32-bit counter encodes two fields:
 *   bits [31:16] — reference count: number of parent PTEs pointing to this table
 *   bits [15:0]  — PTE count: number of valid entries within this table
 * XLAT_REF_COUNT_UNIT (1 << 16) adjusts the reference count field.
 * A plain ±1 adjustment modifies the PTE count field.
 * Must be called with xlat_lock held.
 */
static int32_t table_usage(uint64_t *table, int32_t adjustment)
{
	unsigned int i = table_index(table);
	int32_t prev_count = xlat_use_count[i];
	int32_t new_count = prev_count + adjustment;

	MMU_DEBUG("table [%d]%p: usage %#x -> %#x\n", i, table, prev_count, new_count);

	__ASSERT(new_count >= 0, "table use count underflow");
	__ASSERT(new_count == 0 || new_count >= XLAT_REF_COUNT_UNIT,
		 "table in use with no reference to it");
	__ASSERT((new_count & XLAT_PTE_COUNT_MASK) <= RISCV_PGTABLE_ENTRIES,
		 "table PTE count overflow");

	xlat_use_count[i] = new_count;
	return new_count;
}

/* Increment the reference count of a table (one more parent PTE points to it). */
static inline void inc_table_ref(uint64_t *table)
{
	table_usage(table, XLAT_REF_COUNT_UNIT);
}

/* Decrement the reference count of a table (one parent PTE removed). */
static inline void dec_table_ref(uint64_t *table)
{
	table_usage(table, -XLAT_REF_COUNT_UNIT);
}

/* Increment the count of valid PTEs within a table. */
static inline void inc_pte_count(uint64_t *table)
{
	table_usage(table, 1);
}

/* Decrement the count of valid PTEs within a table. */
static inline void dec_pte_count(uint64_t *table)
{
	table_usage(table, -1);
}

/*
 * Write a PTE into a page table at the given index, maintaining reference and
 * PTE counts.  Installing a table PTE increments the child table's reference
 * count; removing one decrements it.  A PTE value of 0 clears the entry.
 * Must be called with xlat_lock held.
 */
static void set_pte(uint64_t *table, unsigned int index, pte_t pte)
{

	__ASSERT(index < RISCV_PGTABLE_ENTRIES, "PTE index %u out of range", index);

	pte_t old_pte = table[index];

	table[index] = pte;

	/* Update reference counts */
	if (!pte_is_valid(old_pte) && pte_is_valid(pte)) {
		/* Adding new PTE */
		inc_pte_count(table);
		if (pte_is_table(pte)) {
			/* new PTE points to another table - increment its reference */
			uint64_t *child_table = (uint64_t *)pte_to_phys(pte);

			inc_table_ref(child_table);
		}
	} else if (pte_is_valid(old_pte) && !pte_is_valid(pte)) {
		/* Removing PTE */
		if (pte_is_table(old_pte)) {
			/* Old PTE pointed to another table - decrement its reference */
			uint64_t *child_table = (uint64_t *)pte_to_phys(old_pte);

			dec_table_ref(child_table);
		}
		dec_pte_count(table);
	} else if (pte_is_valid(old_pte) && pte_is_valid(pte)) {
		/* Updating existing PTE */
		if (pte_is_table(old_pte) && pte_is_table(pte)) {
			/* Both are table PTEs - update references */
			uint64_t *old_table = (uint64_t *)pte_to_phys(old_pte);
			uint64_t *new_table = (uint64_t *)pte_to_phys(pte);

			if (old_table != new_table) {
				dec_table_ref(old_table);
				inc_table_ref(new_table);
			}
		}
		/* Note: Leaf PTE updates don't affect reference counts */
	}

	MMU_DEBUG("%s: table[%u] = %#llx (was %#llx)\n", __func__, index, pte, old_pte);
}

/*
 * Convert Zephyr generic memory permission flags (K_MEM_PERM_*) to the
 * corresponding RISC-V leaf PTE permission bits (PTE_READ/WRITE/EXEC/USER).
 */
static pte_t flags_to_pte(uint32_t flags)
{
	pte_t pte_flags = PTE_VALID | PTE_ACCESSED;

	/* Permission flags */
	if (flags & K_MEM_PERM_RW) {
		pte_flags |= PTE_READ | PTE_WRITE | PTE_DIRTY;
	} else {
		pte_flags |= PTE_READ; /* Default */
	}

	if (flags & K_MEM_PERM_EXEC) {
		pte_flags |= PTE_EXEC;
	}

	if (flags & K_MEM_PERM_USER) {
		pte_flags |= PTE_USER;
	}

	return pte_flags;
}

/*
 * Return a pointer to the kernel root page table.
 */
static uint64_t *get_root_page_table(void)
{
	return kernel_root_table;
}

uintptr_t z_riscv_kernel_satp(void)
{
	return ((uintptr_t)SATP_MODE << SATP_MODE_SHIFT) |
	       ((uintptr_t)KERNEL_ASID << SATP_ASID_SHIFT) |
	       ((uintptr_t)kernel_root_table >> PAGE_SIZE_SHIFT);
}

/*
 * After unmapping a page at @va, walk the page table hierarchy from the root
 * and free any intermediate tables that have become empty (PTE count == 0).
 * Operates bottom-up: checks the deepest table first, then its parent.
 * Must be called with xlat_lock held.
 */
static void cleanup_unused_tables(uintptr_t va)
{
	uint64_t *table = get_root_page_table();
	uint64_t *tables[PGTABLE_LEVELS];
	unsigned int indices[PGTABLE_LEVELS];
	int level;

	for (level = 0; level < PGTABLE_LEVELS - 1; level++) {
		unsigned int index = VPN(va, level);
		pte_t pte = table[index];

		tables[level] = table;
		indices[level] = index;

		if (!pte_is_valid(pte) || pte_is_leaf(pte)) {
			return;
		}

		table = (uint64_t *)pte_to_phys(pte);
	}

	for (level = PGTABLE_LEVELS - 2; level >= 0; level--) {
		unsigned int index = indices[level];
		uint64_t *current_table = (uint64_t *)pte_to_phys(tables[level][index]);
		unsigned int table_idx = table_index(current_table);

		if ((xlat_use_count[table_idx] & XLAT_PTE_COUNT_MASK) == 0) {
			set_pte(tables[level], index, 0);
			MMU_DEBUG("freed empty table [%d]%p at level %d\n", table_idx,
				  current_table, level + 1);
		} else {
			break;
		}
	}
}

/*
 * Map a contiguous physical range [@phys, @phys+@size) to virtual address @va
 * in the page table rooted at @root, using leaf PTE flags @pte_flags.
 * Intermediate tables are allocated from the pool as needed.
 * @size and addresses must be PAGE_SIZE-aligned.
 * Must be called with xlat_lock held.
 */
static void map_pages_into(uint64_t *root, uintptr_t va, uintptr_t phys, size_t size,
			   pte_t pte_flags)
{
	size_t mapped = 0;

	while (mapped < size) {
		uint64_t *table = root;
		int level;

		for (level = 0; level < PGTABLE_LEVELS - 1; level++) {
			unsigned int index = VPN(va, level);
			pte_t pte = table[index];

			if (!pte_is_valid(pte)) {
				uint64_t *new_child_table = new_table();

				pte = pte_create((uintptr_t)new_child_table, PTE_TYPE_TABLE);
				set_pte(table, index, pte);
			}

			__ASSERT(pte_is_table(pte), "Expected table PTE at level %d", level);
			table = (uint64_t *)pte_to_phys(pte);
		}

		unsigned int leaf_index = VPN(va, PGTABLE_LEVELS - 1);
		pte_t leaf_pte = pte_create(phys, pte_flags);

		set_pte(table, leaf_index, leaf_pte);
		SFENCE_VMA_ADDR((void *)va);

		va += PAGE_SIZE;
		phys += PAGE_SIZE;
		mapped += PAGE_SIZE;
	}
}

/*
 * Unmap the virtual range [@va, @va+@size) from the page table rooted at
 * @root by zeroing the corresponding leaf PTEs and issuing sfence.vma for
 * each unmapped page.  Empty intermediate tables are freed via
 * cleanup_unused_tables().  @size and @va must be PAGE_SIZE-aligned.
 * Must be called with xlat_lock held.
 */
static void unmap_pages_from(uint64_t *root, uintptr_t va, size_t size)
{
	size_t unmapped = 0;

	while (unmapped < size) {
		uint64_t *table = root;
		int level;

		for (level = 0; level < PGTABLE_LEVELS - 1; level++) {
			unsigned int index = VPN(va, level);
			pte_t pte = table[index];

			if (!pte_is_valid(pte)) {
				MMU_DEBUG("No mapping at level %d for VA:%p\n", level, (void *)va);
				goto next_page;
			}

			if (pte_is_leaf(pte)) {
				MMU_DEBUG("Hugepage found at level %d - skipping\n", level);
				goto next_page;
			}

			table = (uint64_t *)pte_to_phys(pte);
		}

		{
			unsigned int leaf_index = VPN(va, PGTABLE_LEVELS - 1);
			pte_t leaf_pte = table[leaf_index];

			if (pte_is_valid(leaf_pte)) {
				set_pte(table, leaf_index, 0);
				MMU_DEBUG("unmapped VA:%p -> PA:%p\n", (void *)va,
					  (void *)pte_to_phys(leaf_pte));
				SFENCE_VMA_ADDR((void *)va);
				cleanup_unused_tables(va);
			}
		}

next_page:
		va += PAGE_SIZE;
		unmapped += PAGE_SIZE;
	}
}

/**
 * Map physical pages into virtual address space
 *
 * @param virt Page-aligned virtual address
 * @param phys Page-aligned physical address
 * @param size Size in bytes (page-aligned)
 * @param flags Permission and caching flags
 */
void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	k_spinlock_key_t key;

	__ASSERT(IS_ALIGNED((uintptr_t)virt, PAGE_SIZE), "Virtual address not page aligned");
	__ASSERT(IS_ALIGNED(phys, PAGE_SIZE), "Physical address not page aligned");
	__ASSERT(IS_ALIGNED(size, PAGE_SIZE), "Size not page aligned");

	key = k_spin_lock(&xlat_lock);

	MMU_DEBUG("mapping VA:%p -> PA:%p size:%zu flags:%#x\n", virt, (void *)phys, size, flags);

	map_pages_into(kernel_root_table, (uintptr_t)virt, phys, size, flags_to_pte(flags));

#ifdef CONFIG_USERSPACE
	/*
	 * Propagate any new kernel L0/L1 entries to all active domain
	 * page tables that carry a private copy of the relevant L1.
	 */
	sync_domains();
#endif

	k_spin_unlock(&xlat_lock, key);

	/*
	 * TODO: SMP TLB shootdown — broadcast an IPI to all other CPUs so
	 * they flush their local TLBs for the affected address range.
	 * sfence.vma only flushes the local TLB.
	 */
}

/**
 * Remove mappings for a provided virtual address range
 *
 * @param addr Virtual address to unmap
 * @param size Size in bytes to unmap
 */
void arch_mem_unmap(void *addr, size_t size)
{
	k_spinlock_key_t key;

	__ASSERT(IS_ALIGNED((uintptr_t)addr, PAGE_SIZE), "Virtual address not page aligned");
	__ASSERT(IS_ALIGNED(size, PAGE_SIZE), "Size not page aligned");

	key = k_spin_lock(&xlat_lock);

	MMU_DEBUG("unmapping VA:%p size:%zu\n", addr, size);

	unmap_pages_from(kernel_root_table, (uintptr_t)addr, size);

	k_spin_unlock(&xlat_lock, key);

	/*
	 * TODO: SMP TLB shootdown — broadcast an IPI to all other CPUs so
	 * they flush their local TLBs for the affected address range.
	 * sfence.vma only flushes the local TLB.
	 */
}

/**
 * Get the mapped physical memory address from virtual address.
 *
 * The function only needs to query the current set of page tables as
 * the information it reports must be common to all of them if multiple
 * page tables are in use. If multiple page tables are active it is unnecessary
 * to iterate over all of them.
 *
 * Unless otherwise specified, virtual pages have the same mappings
 * across all page tables. Calling this function on data pages that are
 * exceptions to this rule (such as the scratch page) is undefined behavior.
 * Just check the currently installed page tables and return the information
 * in that.
 *
 * @param virt Page-aligned virtual address
 * @param[out] phys Mapped physical address (can be NULL if only checking
 *                  if virtual address is mapped)
 *
 * @retval 0 if mapping is found and valid
 * @retval -EFAULT if virtual address is not mapped
 */
int arch_page_phys_get(void *virt, uintptr_t *phys)
{
	uintptr_t va = (uintptr_t)virt;
	uint64_t *table;
	k_spinlock_key_t key;
	uintptr_t result;
	int level;

	/* Don't need page alignment - can lookup any address */

	key = k_spin_lock(&xlat_lock);

	table = get_root_page_table();
	if (!table) {
		MMU_DEBUG("No root page table for VA:%p\n", virt);
		result = -EFAULT;
		goto unlock_and_return;
	}

	/* Walk page table hierarchy */
	for (level = 0; level < PGTABLE_LEVELS; level++) {
		unsigned int index = VPN(va, level);
		pte_t pte = table[index];

		if (!pte_is_valid(pte)) {
			/* No mapping exists */
			MMU_DEBUG("No mapping at level %d for VA:%p\n", level, virt);
			result = -EFAULT;
			goto unlock_and_return;
		}

		if (pte_is_leaf(pte)) {
			/* Found the page - extract physical address */
			uintptr_t page_phys = pte_to_phys(pte);
			unsigned int page_offset_bits = RISCV_PGLEVEL_SHIFT(level);
			uintptr_t page_offset = va & ((1UL << page_offset_bits) - 1);
			uintptr_t final_phys = page_phys + page_offset;

			/* Store result if caller provided output pointer */
			if (phys != NULL) {
				*phys = final_phys;
			}

			MMU_DEBUG("VA:%p -> PA:%p (level %d page)\n", virt, (void *)final_phys,
				  level);
			result = 0; /* SUCCESS */
			goto unlock_and_return;
		}

		/* Continue to next level */
		table = (uint64_t *)pte_to_phys(pte);
	}

	/* Should never reach here if page table levels are correct */
	MMU_DEBUG("Page table walk completed without finding leaf for VA:%p\n", virt);
	result = -EFAULT;

unlock_and_return:
	k_spin_unlock(&xlat_lock, key);
	return result;
}

/**
 * Create platform-specific memory mappings
 *
 * This function provides a hook for platform-specific code to add
 * device register mappings that the kernel needs access to.
 * Platform code can override this function to add their mappings.
 *
 * @return 0 on success, negative error code on failure
 */
__weak int create_platform_mappings(void)
{
#if DT_HAS_CHOSEN(zephyr_console)
	/*
	 * Map the console UART so that printk works after the MMU is
	 * enabled.  Platforms that need additional device mappings should
	 * override this weak function.
	 */
	uintptr_t uart_base = DT_REG_ADDR(DT_CHOSEN(zephyr_console));
	size_t uart_size = ROUND_UP(DT_REG_SIZE(DT_CHOSEN(zephyr_console)), PAGE_SIZE);

	MMU_DEBUG("Mapping console UART: %p size:%zu\n", (void *)uart_base, uart_size);
	arch_mem_map((void *)uart_base, uart_base, uart_size, K_MEM_PERM_RW);
#endif
	return 0;
}

/**
 * Create essential kernel memory mappings
 *
 * Sets up identity mappings for kernel code, data, and other essential regions
 * that must be accessible when the MMU is enabled.
 *
 * @return 0 on success, negative error code on failure
 */
static int create_kernel_mappings(void)
{
	int ret;

	/*
	 * Map the ROM region (exception vectors + text + rodata) as RX.
	 * For non-XIP builds this is a range within RAM; the MMU enforces
	 * the read-only + executable permission on top of the physical RAM.
	 */
	uintptr_t rom_start = ROUND_DOWN((uintptr_t)__rom_region_start, PAGE_SIZE);
	uintptr_t rom_end = ROUND_UP((uintptr_t)__rom_region_end, PAGE_SIZE);
	size_t rom_size = rom_end - rom_start;

	MMU_DEBUG("Mapping kernel ROM region: %p-%p (%zu bytes)\n", (void *)rom_start,
		  (void *)rom_end, rom_size);

	/*
	 * Map the ROM region (exception vectors + text + rodata) as RX
	 * for supervisor mode.  Domain page tables add PTE_USER to this
	 * region (except the ISR trap-entry page) so that user threads can
	 * execute kernel text (z_thread_entry, syscall wrappers, etc.).
	 */
	arch_mem_map((void *)rom_start, rom_start, rom_size, K_MEM_PERM_EXEC | K_MEM_CACHE_WB);

	/*
	 * Map the RAM region (app_smem partitions + data + BSS + noinit,
	 * including the page table pool) as RW.  With CONFIG_USERSPACE the
	 * app_smem section immediately follows the ROM region and precedes
	 * __kernel_ram_start, so use _app_smem_start (== rom_end) as the
	 * actual RAM region start to avoid leaving an unmapped gap.
	 */
#ifdef CONFIG_USERSPACE
	extern char _app_smem_start[];
	uintptr_t ram_start = ROUND_DOWN((uintptr_t)_app_smem_start, PAGE_SIZE);
#else
	uintptr_t ram_start = ROUND_DOWN((uintptr_t)__kernel_ram_start, PAGE_SIZE);
#endif
	uintptr_t ram_end = ROUND_UP((uintptr_t)__kernel_ram_end, PAGE_SIZE);
	size_t ram_size = ram_end - ram_start;

	MMU_DEBUG("Mapping kernel RAM region: %p-%p (%zu bytes)\n", (void *)ram_start,
		  (void *)ram_end, ram_size);

	arch_mem_map((void *)ram_start, ram_start, ram_size, K_MEM_PERM_RW | K_MEM_CACHE_WB);

	ret = create_platform_mappings();

	if (ret != 0) {
		return ret;
	}

	return 0;
}

/**
 * Initialize the RISC-V MMU subsystem
 *
 * This function sets up the initial memory mappings required for the kernel
 * to operate with the MMU enabled. It creates identity mappings for essential
 * kernel sections and initializes the page table management structures.
 *
 * @return 0 on success, negative error code on failure
 */
static int arch_mem_init(void)
{
	uint64_t satp_val;
	uint64_t mode;
	uint64_t new_satp;
	int ret;

	MMU_DEBUG("Initializing RISC-V MMU\n");

	/* Initialize page table pool - all tables start unused */
	memset(xlat_tables, 0, sizeof(xlat_tables));
	memset(xlat_use_count, 0, sizeof(xlat_use_count));

	/*
	 * If the MMU was already enabled by earlier boot stages (e.g. a
	 * bootloader or OpenSBI), recover the root table pointer from the
	 * current SATP value and leave the existing mappings in place.
	 */
	satp_val = csr_read(satp);
	mode = (satp_val >> SATP_MODE_SHIFT) & 0xF;

	if (mode != SATP_MODE_OFF) {
		kernel_root_table = (uint64_t *)((satp_val & SATP_PPN_MASK) << PAGE_SIZE_SHIFT);
		MMU_DEBUG("MMU already enabled (mode=%llu), root table at %p\n", mode,
			  kernel_root_table);
#ifdef CONFIG_USERSPACE
		z_riscv_kernel_satp_val = satp_val;
#endif
		return 0;
	}

	/*
	 * Allocate the root page table first so that create_kernel_mappings()
	 * can populate it while the MMU is still disabled.  The MMU is only
	 * turned on once all mappings are ready.
	 *
	 * Root tables have no parent PTE so set_pte() will never call
	 * inc_table_ref() for them.  Increment manually to mark the table
	 * as alive; arch_mem_domain_destroy() undoes this with a direct
	 * zero-out for domain roots.
	 */
	kernel_root_table = new_table();
	inc_table_ref(kernel_root_table);

	ret = create_kernel_mappings();
	if (ret != 0) {
		LOG_ERR("Failed to create kernel mappings: %d", ret);
		return ret;
	}

	/*
	 * Enable the MMU.  PPN = physical address of root table / PAGE_SIZE,
	 * stored in SATP bits 43:0 (Sv39/Sv48) or 21:0 (Sv32).
	 */
	new_satp = ((uint64_t)SATP_MODE << SATP_MODE_SHIFT) |
		   ((uint64_t)KERNEL_ASID << SATP_ASID_SHIFT) |
		   ((uintptr_t)kernel_root_table >> PAGE_SIZE_SHIFT);

	MMU_DEBUG("Enabling MMU: SATP=0x%llx, root_table=%p, ASID=%d\n", new_satp,
		  kernel_root_table, KERNEL_ASID);

	csr_write(satp, new_satp);
	SFENCE_VMA_ALL();

#ifdef CONFIG_USERSPACE
	z_riscv_kernel_satp_val = new_satp;
#endif

	/*
	 * Set sstatus.SUM so that S-mode code can access pages that have the
	 * U-bit set.  This is required because domain partitions are mapped
	 * with PTE_USER in shared intermediate tables, and kernel threads
	 * running in S-mode must still be able to read/write those pages.
	 */
	csr_set(sstatus, SSTATUS_SUM);

	MMU_DEBUG("RISC-V MMU initialized successfully\n");

#ifdef CONFIG_ARCH_MEM_DOMAIN_DATA
	/*
	 * The default memory domain (k_mem_domain_default) is created by
	 * init_mem_domain_module() which runs at the same PRE_KERNEL_1 level
	 * as us.  If it happened to run first, arch_mem_domain_init() found
	 * kernel_root_table == NULL and left the domain's page table empty.
	 * Patch it now that kernel_root_table is set.
	 */
	extern struct k_mem_domain k_mem_domain_default;

	if (k_mem_domain_default.arch.ptables != NULL) {
		memcpy(k_mem_domain_default.arch.ptables, kernel_root_table, PAGE_SIZE);
		domain_privatize_kernel_text(k_mem_domain_default.arch.ptables);
	}
#endif

	return 0;
}

#ifdef CONFIG_RISCV_MMU_STACK_GUARD
/*
 * Map the guard page at the bottom of a thread's stack object as inaccessible
 * (V=0) in the kernel page tables.  Any access to this page will cause a page
 * fault which is then reported as a stack overflow.
 */
void z_riscv_mmu_map_guard_page(const struct k_thread *thread)
{
	uintptr_t guard = (uintptr_t)thread->stack_obj;
	k_spinlock_key_t key = k_spin_lock(&xlat_lock);

	/* unmap_pages_from sets PTEs to 0 (V=0) and issues sfence.vma */
	unmap_pages_from(kernel_root_table, guard, CONFIG_MMU_PAGE_SIZE);

	k_spin_unlock(&xlat_lock, key);
}
#endif /* CONFIG_RISCV_MMU_STACK_GUARD */

#ifdef CONFIG_USERSPACE

/*
 * Translate a k_mem_partition_attr_t (PMP-style bits) into PTE flags.
 * PMP_R=0x01, PMP_W=0x02, PMP_X=0x04 map directly to PTE_READ/WRITE/EXEC.
 * If the user bits are set, add PTE_USER so the page is accessible from
 * user mode.
 */
static pte_t partition_attr_to_pte(k_mem_partition_attr_t attr)
{
	pte_t pte = PTE_VALID | PTE_ACCESSED;

	if (attr.pmp_attr & PMP_R) {
		pte |= PTE_READ | PTE_USER;
	}
	if (attr.pmp_attr & PMP_W) {
		pte |= PTE_WRITE | PTE_DIRTY;
	}
	if (attr.pmp_attr & PMP_X) {
		pte |= PTE_EXEC;
	}

	return pte;
}

/* Each L1 table entry maps 2^RISCV_PGLEVEL_SHIFT(1) bytes */
#define L1_MAP_SIZE BIT(RISCV_PGLEVEL_SHIFT(1))

/*
 * Privatize the L1/L2 page tables that cover the kernel .text/.rodata
 * region for a domain page table, then add PTE_USER to pages in
 * [__text_region_start, __rom_region_end) so that user mode can execute
 * kernel text (z_thread_entry, syscall stubs, etc.).
 *
 * Pages below __text_region_start (reset + exception.entry sections) keep
 * PTE_USER=0 so that S-mode can fetch instructions from them while the
 * domain SATP is active — this is required during SATP switch at trap
 * exit / arch_user_mode_enter just before SRET.
 */
static int domain_privatize_kernel_text(uint64_t *domain_root)
{
	extern char __rom_region_start[];
	extern char __text_region_start[];
	extern char __rom_region_end[];

	const uintptr_t rom_start = ROUND_DOWN((uintptr_t)__rom_region_start, PAGE_SIZE);
	const uintptr_t text_start = (uintptr_t)__text_region_start;
	const uintptr_t rom_end = ROUND_UP((uintptr_t)__rom_region_end, PAGE_SIZE);
	const unsigned int l0_idx = VPN(rom_start, 0);
	uint64_t *private_l1;
	pte_t l0_pte;
	uintptr_t l1_base = 0;

	l0_pte = domain_root[l0_idx];
	if (!pte_is_valid(l0_pte) || pte_is_leaf(l0_pte)) {
		/* No intermediate table for kernel ROM — nothing to privatize. */
		return 0;
	}

	/*
	 * Create a private copy of the L1 table so modifications to domain
	 * leaf PTEs do not affect the kernel page tables (which share the
	 * same L1 via the root-table memcpy in arch_mem_domain_init).
	 */
	private_l1 = new_table();
	if (!private_l1) {
		return -ENOMEM;
	}
	inc_table_ref(private_l1);
	memcpy(private_l1, (void *)pte_to_phys(l0_pte), PAGE_SIZE);
	domain_root[l0_idx] = pte_create((uintptr_t)private_l1, PTE_TYPE_TABLE);

	/*
	 * Walk ALL valid intermediate L1 entries and privatize their L2
	 * tables.  This prevents partition mappings from one domain from
	 * leaking into another domain that shares the same L2 table.
	 *
	 * For L1 entries that intersect [text_start, rom_end), also stamp
	 * PTE_USER on the leaf entries so that user threads can execute
	 * kernel text (z_thread_entry, syscall stubs, etc.) while the
	 * domain SATP is active.
	 */
	for (unsigned int l1_idx = 0; l1_idx < RISCV_PGTABLE_ENTRIES; l1_idx++) {
		pte_t l1_pte = private_l1[l1_idx];
		uint64_t *private_l2;
		unsigned int i;

		if (!pte_is_valid(l1_pte) || pte_is_leaf(l1_pte)) {
			continue;
		}

		private_l2 = new_table();
		if (!private_l2) {
			return -ENOMEM;
		}
		inc_table_ref(private_l2);
		memcpy(private_l2, (void *)pte_to_phys(l1_pte), PAGE_SIZE);
		private_l1[l1_idx] = pte_create((uintptr_t)private_l2, PTE_TYPE_TABLE);

		/*
		 * Initialize the PTE count to match the entries we copied
		 * so that dec_pte_count() stays balanced when partitions
		 * are unmapped later.
		 */
		for (i = 0; i < RISCV_PGTABLE_ENTRIES; i++) {
			if (pte_is_valid(private_l2[i])) {
				table_usage(private_l2, 1);
			}
		}

		l1_base = ((uintptr_t)l0_idx << RISCV_PGLEVEL_SHIFT(0)) |
			  ((uintptr_t)l1_idx << RISCV_PGLEVEL_SHIFT(1));

		/* Stamp PTE_USER on text/rodata pages in this L2 */
		for (i = 0; i < RISCV_PGTABLE_ENTRIES; i++) {
			uintptr_t page_va = l1_base + ((uintptr_t)i << PAGE_SIZE_SHIFT);
			pte_t pte = private_l2[i];

			if (!pte_is_valid(pte) || !pte_is_leaf(pte)) {
				continue;
			}
			if (page_va >= text_start && page_va < rom_end) {
				private_l2[i] = pte | PTE_USER;
			}
		}
	}

	return 0;
}

/*
 * Propagate new kernel root-table entries to all active domain page tables.
 *
 * When arch_mem_map() adds a new mapping to the kernel root table it may
 * create a new L1 entry in an L1 table that a domain has already privatised
 * (via domain_privatize_kernel_text()).  The private copy was taken at domain
 * creation time and will be missing the new entry.  Walk every domain in
 * domain_list and copy any kernel L0/L1 entries that the domain does not yet
 * have.
 *
 * Must be called with xlat_lock held.
 */
static void sync_domains(void)
{
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&domain_list, node) {
		struct arch_mem_domain *arch_domain =
			CONTAINER_OF(node, struct arch_mem_domain, node);
		uint64_t *domain_root = arch_domain->ptables;
		unsigned int i;

		for (i = 0; i < RISCV_PGTABLE_ENTRIES; i++) {
			pte_t k_pte = kernel_root_table[i];
			pte_t d_pte = domain_root[i];

			if (!pte_is_valid(k_pte)) {
				continue;
			}

			if (!pte_is_valid(d_pte)) {
				/* Domain missing this L0 entry entirely. */
				domain_root[i] = k_pte;
				continue;
			}

			if (!pte_is_table(k_pte) || !pte_is_table(d_pte)) {
				/* Leaf (huge page) mapping — no sub-table to sync. */
				continue;
			}

			{
				uint64_t *k_l1 = (uint64_t *)pte_to_phys(k_pte);
				uint64_t *d_l1 = (uint64_t *)pte_to_phys(d_pte);
				unsigned int j;

				if (k_l1 == d_l1) {
					/* Shared L1 — already in sync. */
					continue;
				}

				/*
				 * Domain has a private L1 copy.  Copy any
				 * kernel L1 entries that are absent in the
				 * private copy.
				 */
				for (j = 0; j < RISCV_PGTABLE_ENTRIES; j++) {
					if (pte_is_valid(k_l1[j]) &&
					    !pte_is_valid(d_l1[j])) {
						d_l1[j] = k_l1[j];
					}
				}
			}
		}
	}
}

int arch_mem_domain_max_partitions_get(void)
{
	return CONFIG_MAX_DOMAIN_PARTITIONS;
}

int arch_mem_domain_init(struct k_mem_domain *domain)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&xlat_lock);

	domain->arch.ptables = new_table();
	if (!domain->arch.ptables) {
		k_spin_unlock(&xlat_lock, key);
		return -ENOMEM;
	}
	/* Root table: no parent PTE, so add the initial reference manually. */
	inc_table_ref(domain->arch.ptables);

	/*
	 * Copy kernel root table entries so the domain shares all
	 * current kernel mappings. Partitions are mapped on top of
	 * this snapshot when added.
	 *
	 * kernel_root_table is NULL if arch_mem_init() has not yet run.
	 * Guard defensively; arch_mem_init() will patch the table afterwards.
	 */
	if (kernel_root_table != NULL) {
		int ret;

		memcpy(domain->arch.ptables, kernel_root_table, PAGE_SIZE);

		/*
		 * Create private copies of the L1/L2 tables for the kernel
		 * ROM region and add PTE_USER to .text/.rodata pages.
		 * This lets user threads execute kernel text (z_thread_entry,
		 * syscall stubs) while the domain SATP is active, without
		 * affecting the kernel SATP (PTE_USER=0 everywhere).
		 */
		ret = domain_privatize_kernel_text(domain->arch.ptables);
		if (ret != 0) {
			k_spin_unlock(&xlat_lock, key);
			return ret;
		}
	}

	domain->arch.asid = alloc_asid();

	sys_slist_append(&domain_list, &domain->arch.node);

	k_spin_unlock(&xlat_lock, key);

	return 0;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain, uint32_t partition_id)
{
	struct k_mem_partition *part = &domain->partitions[partition_id];
	pte_t pte_flags = partition_attr_to_pte(part->attr);
	k_spinlock_key_t key;

	key = k_spin_lock(&xlat_lock);

	map_pages_into(domain->arch.ptables, part->start, part->start, part->size, pte_flags);

	SFENCE_VMA_ASID(domain->arch.asid);

	k_spin_unlock(&xlat_lock, key);

	return 0;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain, uint32_t partition_id)
{
	struct k_mem_partition *part = &domain->partitions[partition_id];
	k_spinlock_key_t key;

	key = k_spin_lock(&xlat_lock);

	unmap_pages_from(domain->arch.ptables, part->start, part->size);

	SFENCE_VMA_ASID(domain->arch.asid);

	k_spin_unlock(&xlat_lock, key);

	return 0;
}

int arch_mem_domain_thread_add(struct k_thread *thread)
{
	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;

	thread->arch.satp = ((uintptr_t)SATP_MODE << SATP_MODE_SHIFT) |
			    ((uintptr_t)domain->arch.asid << SATP_ASID_SHIFT) |
			    ((uintptr_t)domain->arch.ptables >> PAGE_SIZE_SHIFT);

	return 0;
}

int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	/* Revert to kernel page tables */
	thread->arch.satp = z_riscv_kernel_satp();

	return 0;
}

int arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	k_spinlock_key_t key;
	uint64_t *ptables = domain->arch.ptables;
	int i;

	if (!ptables) {
		return 0;
	}

	key = k_spin_lock(&xlat_lock);

	sys_slist_find_and_remove(&domain_list, &domain->arch.node);

	/*
	 * Release intermediate tables that are exclusively owned by this
	 * domain (ref_count == XLAT_REF_COUNT_UNIT means only the domain
	 * root points to them).  Tables shared with the kernel page table
	 * have a higher ref count and must not be freed.
	 *
	 * TODO: recurse into deeper levels for partitions that required
	 * multi-level exclusive sub-tables.
	 */
	for (i = 0; i < RISCV_PGTABLE_ENTRIES; i++) {
		pte_t pte = ptables[i];

		if (pte_is_valid(pte) && pte_is_table(pte)) {
			uint64_t *child = (uint64_t *)pte_to_phys(pte);
			unsigned int idx = table_index(child);

			if (xlat_use_count[idx] == XLAT_REF_COUNT_UNIT) {
				xlat_use_count[idx] = 0;
			}
		}
	}

	/* Free the domain root table */
	xlat_use_count[table_index(ptables)] = 0;
	domain->arch.ptables = NULL;

	k_spin_unlock(&xlat_lock, key);

	return 0;
}

void z_riscv_mmu_map_user_stack(struct k_thread *thread)
{
	struct k_mem_domain *domain = thread->mem_domain_info.mem_domain;
	k_spinlock_key_t key;

	if (!domain || !domain->arch.ptables) {
		return;
	}

	uintptr_t stack_start = thread->stack_info.start;
	size_t stack_size = ROUND_UP(thread->stack_info.size, PAGE_SIZE);

	key = k_spin_lock(&xlat_lock);
	map_pages_into(domain->arch.ptables, stack_start, stack_start, stack_size,
		       PTE_VALID | PTE_READ | PTE_WRITE | PTE_USER |
		       PTE_ACCESSED | PTE_DIRTY);
	SFENCE_VMA_ASID(domain->arch.asid);
	k_spin_unlock(&xlat_lock, key);
}

#ifdef CONFIG_MEM_DOMAIN_ISOLATED_STACKS
/*
 * Called at every context switch when MEM_DOMAIN_ISOLATED_STACKS is active.
 * Clears PTE_USER from the outgoing thread's stack pages in its domain page
 * tables and sets PTE_USER on the incoming thread's stack pages in its domain
 * page tables.  This ensures that within a shared domain, each thread can only
 * access its own stack from U-mode.
 *
 * This relies on PTE_USER toggling which only affects U-mode accesses, so it
 * requires CONFIG_USERSPACE to have any effect.
 */
void z_riscv_mmu_switch_stack_perms(struct k_thread *old_thread,
				    struct k_thread *new_thread)
{
	struct k_mem_domain *old_domain = old_thread->mem_domain_info.mem_domain;
	struct k_mem_domain *new_domain = new_thread->mem_domain_info.mem_domain;
	k_spinlock_key_t key;

	key = k_spin_lock(&xlat_lock);

	/* Remove PTE_USER from the outgoing thread's stack */
	if ((old_thread->base.user_options & K_USER) &&
	    old_thread->stack_info.start != 0 &&
	    old_domain && old_domain->arch.ptables) {
		uintptr_t start = old_thread->stack_info.start;
		size_t size = ROUND_UP(old_thread->stack_info.size, PAGE_SIZE);

		map_pages_into(old_domain->arch.ptables, start, start, size,
			       PTE_VALID | PTE_READ | PTE_WRITE |
			       PTE_ACCESSED | PTE_DIRTY);
		SFENCE_VMA_ASID(old_domain->arch.asid);
	}

	/* Grant PTE_USER on the incoming thread's stack */
	if ((new_thread->base.user_options & K_USER) &&
	    new_thread->stack_info.start != 0 &&
	    new_domain && new_domain->arch.ptables) {
		uintptr_t start = new_thread->stack_info.start;
		size_t size = ROUND_UP(new_thread->stack_info.size, PAGE_SIZE);

		map_pages_into(new_domain->arch.ptables, start, start, size,
			       PTE_VALID | PTE_READ | PTE_WRITE | PTE_USER |
			       PTE_ACCESSED | PTE_DIRTY);
		SFENCE_VMA_ASID(new_domain->arch.asid);
	}

	k_spin_unlock(&xlat_lock, key);
}
#endif /* CONFIG_MEM_DOMAIN_ISOLATED_STACKS */

#endif /* CONFIG_USERSPACE */

void arch_reserved_pages_update(void)
{
	/*
	 * xlat_tables is a static array in BSS — it is already within the
	 * kernel image range [z_mapped_start, z_mapped_end] and will be
	 * automatically pinned by z_mem_manage_init().  No additional pages
	 * need to be marked reserved on this platform.
	 */
}

SYS_INIT(arch_mem_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
