/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver to utilize TLB on Intel Audio DSP
 *
 * TLB (Translation Lookup Buffer) table is used to map between
 * physical and virtual memory. This is global to all cores
 * on the DSP, as changes to the TLB table are visible to
 * all cores.
 *
 * Note that all passed in addresses should be in cached range
 * (aka cached addresses). Due to the need to calculate TLB
 * indexes, virtual addresses will be converted internally to
 * cached one via z_soc_cached_ptr(). However, physical addresses
 * are untouched.
 */

#include "mm_drv_intel_adsp.h"
#include <soc_util.h>
#include <zephyr/drivers/mm/mm_drv_intel_adsp_mtl_tlb.h>
#include <zephyr/drivers/mm/mm_drv_bank.h>
#include <zephyr/debug/sparse.h>

static struct k_spinlock tlb_lock;
extern struct k_spinlock sys_mm_drv_common_lock;

static struct mem_drv_bank hpsram_bank[L2_SRAM_BANK_NUM];

#ifdef CONFIG_SOC_INTEL_COMM_WIDGET
#include <adsp_comm_widget.h>

static uint32_t used_pages;
/* PMC uses 32 KB banks */
static uint32_t used_pmc_banks_reported;
#endif


/* Define a marker which is placed by the linker script just after
 * last explicitly defined section. All .text, .data, .bss and .heap
 * sections should be placed before this marker in the memory.
 * This driver is using the location of the marker to
 * unmap the unused L2 memory and power off corresponding memory banks.
 */
__attribute__((__section__(".unused_ram_start_marker")))
static int unused_l2_sram_start_marker = 0xba0babce;
#define UNUSED_L2_START_ALIGNED ROUND_UP(POINTER_TO_UINT(&unused_l2_sram_start_marker), \
					 CONFIG_MM_DRV_PAGE_SIZE)

/* declare L2 physical memory block */
SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(
		L2_PHYS_SRAM_REGION,
		CONFIG_MM_DRV_PAGE_SIZE,
		L2_SRAM_PAGES_NUM,
		(uint8_t *) L2_SRAM_BASE);

/**
 * Calculate the index to the TLB table.
 *
 * @param vaddr Page-aligned virutal address.
 * @return Index to the TLB table.
 */
static uint32_t get_tlb_entry_idx(uintptr_t vaddr)
{
	return (POINTER_TO_UINT(vaddr) - CONFIG_KERNEL_VM_BASE) /
	       CONFIG_MM_DRV_PAGE_SIZE;
}

/**
 * Calculate the index of the HPSRAM bank.
 *
 * @param pa physical address.
 * @return Index of the HPSRAM bank.
 */
static uint32_t get_hpsram_bank_idx(uintptr_t pa)
{
	uint32_t phys_offset = pa - L2_SRAM_BASE;

	return (phys_offset / SRAM_BANK_SIZE);
}

/**
 * Convert the SYS_MM_MEM_PERM_* flags into TLB entry permission bits.
 *
 * @param flags Access flags (SYS_MM_MEM_PERM_*)
 * @return TLB entry permission bits
 */
static uint16_t flags_to_tlb_perms(uint32_t flags)
{
#if defined(CONFIG_SOC_SERIES_INTEL_ACE)
	uint16_t perms = 0;

	if ((flags & SYS_MM_MEM_PERM_RW) == SYS_MM_MEM_PERM_RW) {
		perms |= TLB_WRITE_BIT;
	}

	if ((flags & SYS_MM_MEM_PERM_EXEC) == SYS_MM_MEM_PERM_EXEC) {
		perms |= TLB_EXEC_BIT;
	}

	return perms;
#else
	return 0;
#endif
}

#if defined(CONFIG_SOC_SERIES_INTEL_ACE)
/**
 * Convert TLB entry permission bits to the SYS_MM_MEM_PERM_* flags.
 *
 * @param perms TLB entry permission bits
 * @return Access flags (SYS_MM_MEM_PERM_*)
 */
static uint16_t tlb_perms_to_flags(uint16_t perms)
{
	uint32_t flags = 0;

	if ((perms & TLB_WRITE_BIT) == TLB_WRITE_BIT) {
		flags |= SYS_MM_MEM_PERM_RW;
	}

	if ((perms & TLB_EXEC_BIT) == TLB_EXEC_BIT) {
		flags |= SYS_MM_MEM_PERM_EXEC;
	}

	return flags;
}
#endif

static int sys_mm_drv_hpsram_pwr(uint32_t bank_idx, bool enable, bool non_blocking)
{
#if defined(CONFIG_SOC_SERIES_INTEL_ACE)
	if (bank_idx > ace_hpsram_get_bank_count()) {
		return -1;
	}

	HPSRAM_REGS(bank_idx)->HSxPGCTL = !enable;

	if (!non_blocking) {
		while (HPSRAM_REGS(bank_idx)->HSxPGISTS == enable) {
			k_busy_wait(1);
		}
	}
#endif
	return 0;
}

#ifdef CONFIG_SOC_INTEL_COMM_WIDGET
static void sys_mm_drv_report_page_usage(void)
{
	/* PMC uses 32 KB banks */
	uint32_t pmc_banks = DIV_ROUND_UP(used_pages, KB(32) / CONFIG_MM_DRV_PAGE_SIZE);

	if (used_pmc_banks_reported != pmc_banks) {
		if (!adsp_comm_widget_pmc_send_ipc(pmc_banks)) {
			/* Store reported value if message was sent successfully. */
			used_pmc_banks_reported = pmc_banks;
		}
	}
}
#endif

int sys_mm_drv_map_page(void *virt, uintptr_t phys, uint32_t flags)
{
	k_spinlock_key_t key;
	uint32_t entry_idx, bank_idx;
	uint16_t entry;
	volatile uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	int ret = 0;
	void *phys_block_ptr;

	/*
	 * Cached addresses for both physical and virtual.
	 *
	 * As the main memory is in cached address ranges,
	 * the cached physical address is needed to perform
	 * bound check.
	 */
	uintptr_t pa = POINTER_TO_UINT(z_soc_cached_ptr(UINT_TO_POINTER(phys)));
	uintptr_t va = POINTER_TO_UINT(z_soc_cached_ptr(virt));

	ARG_UNUSED(flags);

	/* Make sure VA is page-aligned */
	CHECKIF(!sys_mm_drv_is_addr_aligned(va)) {
		ret = -EINVAL;
		goto out;
	}

	/* Check bounds of virtual address space */
	CHECKIF((va < UNUSED_L2_START_ALIGNED) ||
		(va >= (CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE))) {
		ret = -EINVAL;
		goto out;
	}

	/*
	 * When the provided physical address is NULL
	 * then it is a signal to the Intel ADSP TLB driver to
	 * select the first available free physical address
	 * autonomously within the driver.
	 */
	if (UINT_TO_POINTER(phys) == NULL) {
		ret = sys_mem_blocks_alloc_contiguous(&L2_PHYS_SRAM_REGION, 1,
						      &phys_block_ptr);
		if (ret != 0) {
			__ASSERT(false,
				 "unable to assign free phys page %d\n", ret);
			goto out;
		}
		pa = POINTER_TO_UINT(z_soc_cached_ptr(phys_block_ptr));
	}

	/* Check bounds of physical address space */
	CHECKIF((pa < L2_SRAM_BASE) ||
		(pa >= (L2_SRAM_BASE + L2_SRAM_SIZE))) {
		ret = -EINVAL;
		goto out;
	}

	/* Make sure PA is page-aligned */
	CHECKIF(!sys_mm_drv_is_addr_aligned(pa)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&tlb_lock);

	entry_idx = get_tlb_entry_idx(va);

#ifdef CONFIG_SOC_INTEL_COMM_WIDGET
	used_pages++;
	sys_mm_drv_report_page_usage();
#endif

	bank_idx = get_hpsram_bank_idx(pa);
	if (sys_mm_drv_bank_page_mapped(&hpsram_bank[bank_idx]) == 1) {
		sys_mm_drv_hpsram_pwr(bank_idx, true, false);
	}

	/*
	 * The address part of the TLB entry takes the lowest
	 * TLB_PADDR_SIZE bits of the physical page number,
	 * and discards the highest bits.  This is due to the
	 * architecture design where the same physical page
	 * can be accessed via two addresses. One address goes
	 * through the cache, and the other one accesses
	 * memory directly (without cache). The difference
	 * between these two addresses are in the higher bits,
	 * and the lower bits are the same.  And this is why
	 * TLB only cares about the lower part of the physical
	 * address.
	 */
	entry = pa_to_tlb_entry(pa);

	/* Enable the translation in the TLB entry */
	entry |= TLB_ENABLE_BIT;

	/* Set permissions for this entry */
	entry |= flags_to_tlb_perms(flags);

	tlb_entries[entry_idx] = entry;

	/*
	 * Invalid the cache of the newly mapped virtual page to
	 * avoid stale data.
	 */
	z_xtensa_cache_inv(virt, CONFIG_MM_DRV_PAGE_SIZE);

	k_spin_unlock(&tlb_lock, key);

out:
	return ret;
}

int sys_mm_drv_map_region(void *virt, uintptr_t phys,
			  size_t size, uint32_t flags)
{
	k_spinlock_key_t key;
	int ret = 0;
	size_t offset;
	uintptr_t pa;
	uint8_t *va;

	CHECKIF(!sys_mm_drv_is_addr_aligned(phys) ||
		!sys_mm_drv_is_virt_addr_aligned(virt) ||
		!sys_mm_drv_is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	va = (__sparse_force uint8_t *)z_soc_cached_ptr(virt);
	pa = phys;

	key = k_spin_lock(&sys_mm_drv_common_lock);

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		int ret2 = sys_mm_drv_map_page(va, pa, flags);

		if (ret2 != 0) {
			__ASSERT(false, "cannot map 0x%lx to %p\n", pa, va);

			ret = ret2;
		}
		va += CONFIG_MM_DRV_PAGE_SIZE;
		if (phys != 0) {
			pa += CONFIG_MM_DRV_PAGE_SIZE;
		}
	}

	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	return ret;
}

int sys_mm_drv_map_array(void *virt, uintptr_t *phys,
			 size_t cnt, uint32_t flags)
{
	void *va = (__sparse_force void *)z_soc_cached_ptr(virt);

	return sys_mm_drv_simple_map_array(va, phys, cnt, flags);
}

int sys_mm_drv_unmap_page(void *virt)
{
	k_spinlock_key_t key;
	uint32_t entry_idx, bank_idx;
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	uintptr_t pa;
	int ret = 0;

	/* Use cached virtual address */
	uintptr_t va = POINTER_TO_UINT(z_soc_cached_ptr(virt));

	/* Check bounds of virtual address space */
	CHECKIF((va < UNUSED_L2_START_ALIGNED) ||
		(va >= (CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE))) {
		ret = -EINVAL;
		goto out;
	}

	/* Make sure inputs are page-aligned */
	CHECKIF(!sys_mm_drv_is_addr_aligned(va)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&tlb_lock);

	/*
	 * Flush the cache to make sure the backing physical page
	 * has the latest data.
	 */
	z_xtensa_cache_flush(virt, CONFIG_MM_DRV_PAGE_SIZE);

	entry_idx = get_tlb_entry_idx(va);

	/* Simply clear the enable bit */
	tlb_entries[entry_idx] &= ~TLB_ENABLE_BIT;

	pa = tlb_entry_to_pa(tlb_entries[entry_idx]);

	/* Check bounds of physical address space. */
	/* Initial TLB mappings could point to non existing physical pages. */
	if ((pa >= L2_SRAM_BASE) && (pa < (L2_SRAM_BASE + L2_SRAM_SIZE))) {
		sys_mem_blocks_free_contiguous(&L2_PHYS_SRAM_REGION,
					       UINT_TO_POINTER(pa), 1);

		bank_idx = get_hpsram_bank_idx(pa);
#ifdef CONFIG_SOC_INTEL_COMM_WIDGET
		used_pages--;
		sys_mm_drv_report_page_usage();
#endif

		if (sys_mm_drv_bank_page_unmapped(&hpsram_bank[bank_idx]) == 0) {
			sys_mm_drv_hpsram_pwr(bank_idx, false, false);
		}
	}

	k_spin_unlock(&tlb_lock, key);

out:
	return ret;
}

int sys_mm_drv_unmap_region(void *virt, size_t size)
{
	void *va = (__sparse_force void *)z_soc_cached_ptr(virt);

	return sys_mm_drv_simple_unmap_region(va, size);
}

int sys_mm_drv_page_phys_get(void *virt, uintptr_t *phys)
{
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	uintptr_t ent;
	int ret = 0;

	/* Use cached address */
	uintptr_t va = POINTER_TO_UINT(z_soc_cached_ptr(virt));

	CHECKIF(!sys_mm_drv_is_addr_aligned(va)) {
		ret = -EINVAL;
		goto out;
	}

	/* Check bounds of virtual address space */
	CHECKIF((va < CONFIG_KERNEL_VM_BASE) ||
		(va >= (CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE))) {
		ret = -EINVAL;
		goto out;
	}

	ent = tlb_entries[get_tlb_entry_idx(va)];

	if ((ent & TLB_ENABLE_BIT) != TLB_ENABLE_BIT) {
		ret = -EFAULT;
	} else {
		if (phys != NULL) {
			*phys = (ent & TLB_PADDR_MASK) *
				CONFIG_MM_DRV_PAGE_SIZE + TLB_PHYS_BASE;
		}

		ret = 0;
	}

out:
	return ret;
}

int sys_mm_drv_page_flag_get(void *virt, uint32_t *flags)
{
	ARG_UNUSED(virt);
	int ret = 0;

#if defined(CONFIG_SOC_SERIES_INTEL_ACE)
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	uint16_t ent;

	/* Use cached address */
	uintptr_t va = POINTER_TO_UINT(z_soc_cached_ptr(virt));

	CHECKIF(!sys_mm_drv_is_addr_aligned(va)) {
		ret = -EINVAL;
		goto out;
	}

	/* Check bounds of virtual address space */
	CHECKIF((va < CONFIG_KERNEL_VM_BASE) ||
		(va >= (CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE))) {
		ret = -EINVAL;
		goto out;
	}

	ent = tlb_entries[get_tlb_entry_idx(va)];

	if ((ent & TLB_ENABLE_BIT) != TLB_ENABLE_BIT) {
		ret = -EFAULT;
	} else {
		*flags = tlb_perms_to_flags(ent);
	}

out:
#else
	/*
	 * There are no caching mode, or R/W, or eXecution (etc.) bits.
	 * So just return 0.
	 */

	*flags = 0U;
#endif

	return ret;
}

int sys_mm_drv_remap_region(void *virt_old, size_t size,
			    void *virt_new)
{
	void *va_new = (__sparse_force void *)z_soc_cached_ptr(virt_new);
	void *va_old = (__sparse_force void *)z_soc_cached_ptr(virt_old);

	return sys_mm_drv_simple_remap_region(va_old, size, va_new);
}

int sys_mm_drv_move_region(void *virt_old, size_t size, void *virt_new,
			   uintptr_t phys_new)
{
	k_spinlock_key_t key;
	size_t offset;
	int ret = 0;

	virt_new = (__sparse_force void *)z_soc_cached_ptr(virt_new);
	virt_old = (__sparse_force void *)z_soc_cached_ptr(virt_old);

	CHECKIF(!sys_mm_drv_is_virt_addr_aligned(virt_old) ||
		!sys_mm_drv_is_virt_addr_aligned(virt_new) ||
		!sys_mm_drv_is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	if ((POINTER_TO_UINT(virt_new) >= POINTER_TO_UINT(virt_old)) &&
	    (POINTER_TO_UINT(virt_new) < (POINTER_TO_UINT(virt_old) + size))) {
		ret = -EINVAL; /* overlaps */
		goto out;
	}

	/*
	 * The function's behavior has been updated to accept
	 * phys_new == NULL and get the physical addresses from
	 * the actual TLB instead of from the caller.
	 */
	if (phys_new != POINTER_TO_UINT(NULL) &&
	    !sys_mm_drv_is_addr_aligned(phys_new)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&sys_mm_drv_common_lock);

	if (!sys_mm_drv_is_virt_region_mapped(virt_old, size) ||
	    !sys_mm_drv_is_virt_region_unmapped(virt_new, size)) {
		ret = -EINVAL;
		goto unlock_out;
	}

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		uint8_t *va_old = (uint8_t *)virt_old + offset;
		uint8_t *va_new = (uint8_t *)virt_new + offset;
		uintptr_t pa;
		uint32_t flags;
		int ret2;

		ret2 = sys_mm_drv_page_flag_get(va_old, &flags);
		if (ret2 != 0) {
			__ASSERT(false, "cannot query page flags %p\n", va_old);

			ret = ret2;
			goto unlock_out;
		}

		ret2 = sys_mm_drv_page_phys_get(va_old, &pa);
		if (ret2 != 0) {
			__ASSERT(false, "cannot query page paddr %p\n", va_old);

			ret = ret2;
			goto unlock_out;
		}

		/*
		 * Only map the new page when we can retrieve
		 * flags and phys addr of the old mapped page as We don't
		 * want to map with unknown random flags.
		 */
		ret2 = sys_mm_drv_map_page(va_new, pa, flags);
		if (ret2 != 0) {
			__ASSERT(false, "cannot map 0x%lx to %p\n", pa, va_new);

			ret = ret2;
		}

		ret2 = sys_mm_drv_unmap_page(va_old);
		if (ret2 != 0) {
			__ASSERT(false, "cannot unmap %p\n", va_old);

			ret = ret2;
		}
	}

unlock_out:
	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	/*
	 * Since move is done in virtual space, need to
	 * flush the cache to make sure the backing physical
	 * pages have the new data.
	 */
	z_xtensa_cache_flush(virt_new, size);
	z_xtensa_cache_flush_inv(virt_old, size);

	return ret;
}

int sys_mm_drv_move_array(void *virt_old, size_t size, void *virt_new,
			  uintptr_t *phys_new, size_t phys_cnt)
{
	int ret;

	void *va_new = (__sparse_force void *)z_soc_cached_ptr(virt_new);
	void *va_old = (__sparse_force void *)z_soc_cached_ptr(virt_old);

	ret = sys_mm_drv_simple_move_array(va_old, size, va_new,
					    phys_new, phys_cnt);

	/*
	 * Since memcpy() is done in virtual space, need to
	 * flush the cache to make sure the backing physical
	 * pages have the new data.
	 */
	z_xtensa_cache_flush(va_new, size);

	return ret;
}

static int sys_mm_drv_mm_init(const struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	/*
	 * Change size of avalible physical memory according to fw register information
	 * in runtime.
	 */

	uint32_t avalible_memory_size = ace_hpsram_get_bank_count() * SRAM_BANK_SIZE;

	L2_PHYS_SRAM_REGION.num_blocks = avalible_memory_size / CONFIG_MM_DRV_PAGE_SIZE;

	ret = calculate_memory_regions(UNUSED_L2_START_ALIGNED);
	CHECKIF(ret != 0) {
		return ret;
	}
	/*
	 * Initialize memblocks that will store physical
	 * page usage. Initially all physical pages are
	 * mapped in linear way to virtual address space
	 * so mark all pages as allocated.
	 */

	ret = sys_mem_blocks_get(&L2_PHYS_SRAM_REGION,
				 (void *) L2_SRAM_BASE, L2_SRAM_PAGES_NUM);
	CHECKIF(ret != 0) {
		return ret;
	}

	/*
	 * Initialize refcounts for all HPSRAM banks
	 * as fully used because entire HPSRAM is powered on
	 * at system boot. Set reference count to a number
	 * of pages within single memory bank.
	 */
	for (int i = 0; i < L2_SRAM_BANK_NUM; i++) {
		sys_mm_drv_bank_init(&hpsram_bank[i],
				     SRAM_BANK_SIZE / CONFIG_MM_DRV_PAGE_SIZE);
	}
#ifdef CONFIG_SOC_INTEL_COMM_WIDGET
	used_pages = L2_SRAM_BANK_NUM * SRAM_BANK_SIZE / CONFIG_MM_DRV_PAGE_SIZE;
#endif

#ifdef CONFIG_MM_DRV_INTEL_ADSP_TLB_REMAP_UNUSED_RAM
	/*
	 * find virtual address range which are unused
	 * in the system
	 */
	if (L2_SRAM_BASE + L2_SRAM_SIZE < UNUSED_L2_START_ALIGNED ||
	    L2_SRAM_BASE > UNUSED_L2_START_ALIGNED) {

		__ASSERT(false,
			 "unused l2 pointer is outside of l2 sram range %p\n",
			 UNUSED_L2_START_ALIGNED);
		return -EFAULT;
	}

	/*
	 * Unmap all unused physical pages from the entire
	 * virtual address space to save power
	 */
	size_t unused_size = CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE -
			     UNUSED_L2_START_ALIGNED;

	ret = sys_mm_drv_unmap_region(UINT_TO_POINTER(UNUSED_L2_START_ALIGNED),
				      unused_size);
#endif

	/*
	 * Notify PMC about used HP-SRAM pages.
	 */
#ifdef CONFIG_SOC_INTEL_COMM_WIDGET
	sys_mm_drv_report_page_usage();
#endif

	return 0;
}

static void adsp_mm_save_context(void *storage_buffer)
{
	uint16_t entry;
	uint32_t entry_idx;
	int page_idx;
	uint32_t phys_addr;
	volatile uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	uint8_t *location = (uint8_t *) storage_buffer;

	/* first, store the existing TLB */
	memcpy(location, UINT_TO_POINTER(TLB_BASE), TLB_SIZE);
	location += TLB_SIZE;

	/* save context of all the pages */
	for (page_idx = 0; page_idx < L2_SRAM_PAGES_NUM; page_idx++) {
		phys_addr = POINTER_TO_UINT(L2_SRAM_BASE) +
				CONFIG_MM_DRV_PAGE_SIZE * page_idx;
		if (sys_mem_blocks_is_region_free(
				&L2_PHYS_SRAM_REGION,
				UINT_TO_POINTER(phys_addr), 1)) {
			/* skip a free page */
			continue;
		}

		/* map the physical addr 1:1 to virtual address */
		entry_idx = get_tlb_entry_idx(phys_addr);
		entry = pa_to_tlb_entry(phys_addr);

		if ((tlb_entries[entry_idx] & TLB_PADDR_MASK) != entry) {
			/* this page needs remapping, invalidate cache to avoid stalled data
			 * all cache data has been flushed before
			 * do this for pages to remap only
			 */
			z_xtensa_cache_inv(UINT_TO_POINTER(phys_addr), CONFIG_MM_DRV_PAGE_SIZE);

			/* Enable the translation in the TLB entry */
			entry |= TLB_ENABLE_BIT;

			/* map the page 1:1 virtual to physical */
			tlb_entries[entry_idx] = entry;
		}

		/* save physical address */
		*((uint32_t *) location) = phys_addr;
		location += sizeof(uint32_t);

		/* save the page */
		memcpy(location,
			UINT_TO_POINTER(phys_addr),
			CONFIG_MM_DRV_PAGE_SIZE);
		location += CONFIG_MM_DRV_PAGE_SIZE;
	}

	/* write end marker - a null address */
	*((uint32_t *) location) = 0;
	location += sizeof(uint32_t);

	z_xtensa_cache_flush(
		storage_buffer,
		(uint32_t)location - (uint32_t)storage_buffer);


	/* system state is frozen, ready to poweroff, no further changes will be stored */
}

__imr void adsp_mm_restore_context(void *storage_buffer)
{
	/* at this point system must be in a startup state
	 * TLB must be set to initial state
	 * Note! the stack must NOT be in the area being restored
	 */
	uint32_t phys_addr;
	uint8_t *location;

	/* restore context of all the pages */
	location = (uint8_t *) storage_buffer + TLB_SIZE;

	phys_addr = *((uint32_t *) location);

	while (phys_addr != 0) {
		uint32_t phys_addr_uncached =
				POINTER_TO_UINT(z_soc_uncached_ptr(
					(void __sparse_cache *)UINT_TO_POINTER(phys_addr)));
		uint32_t phys_offset = phys_addr - L2_SRAM_BASE;
		uint32_t bank_idx = (phys_offset / SRAM_BANK_SIZE);

		location += sizeof(uint32_t);

		/* turn on memory bank power, wait till the power is on */
		__ASSERT_NO_MSG(bank_idx <= ace_hpsram_get_bank_count());
		HPSRAM_REGS(bank_idx)->HSxPGCTL = 0;
		while (HPSRAM_REGS(bank_idx)->HSxPGISTS == 1) {
			/* k_busy_wait cannot be used here - not available */
		}

		/* copy data to uncached alias and invalidate cache */
		bmemcpy(UINT_TO_POINTER(phys_addr_uncached),
			location,
			CONFIG_MM_DRV_PAGE_SIZE);
		z_xtensa_cache_inv(UINT_TO_POINTER(phys_addr), CONFIG_MM_DRV_PAGE_SIZE);

		location += CONFIG_MM_DRV_PAGE_SIZE;
		phys_addr = *((uint32_t *) location);
	}

	/* restore original TLB table */
	bmemcpy(UINT_TO_POINTER(TLB_BASE), storage_buffer, TLB_SIZE);

	/* HPSRAM memory is restored */
}

static uint32_t adsp_mm_get_storage_size(void)
{
	/*
	 * FIXME - currently the function returns a maximum possible size of the buffer
	 * as L3 memory is generally a huge area its OK (and fast)
	 * in future the function may go through the mapping and calculate a required size
	 */
	return	L2_SRAM_SIZE + TLB_SIZE + (L2_SRAM_PAGES_NUM * sizeof(void *))
		+ sizeof(void *);
}

static const struct intel_adsp_tlb_api adsp_tlb_api_func = {
	.save_context = adsp_mm_save_context,
	.get_storage_size = adsp_mm_get_storage_size
};

DEVICE_DT_DEFINE(DT_INST(0, intel_adsp_mtl_tlb),
		sys_mm_drv_mm_init,
		NULL,
		NULL,
		NULL,
		POST_KERNEL,
		0,
		&adsp_tlb_api_func);
