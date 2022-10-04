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

#define DT_DRV_COMPAT intel_adsp_tlb

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mm/system_mm.h>
#include <zephyr/sys/mem_blocks.h>

#include <soc.h>
#include <cavs-mem.h>

#include "mm_drv_common.h"

DEVICE_MMIO_TOPLEVEL_STATIC(tlb_regs, DT_DRV_INST(0));

#define TLB_BASE \
	((mm_reg_t)DEVICE_MMIO_TOPLEVEL_GET(tlb_regs))

/*
 * Number of significant bits in the page index (defines the size of
 * the table)
 */
#if defined(CONFIG_SOC_SERIES_INTEL_ACE1X)
# include <ace_v1x-regs.h>
# define TLB_PADDR_SIZE 12
# define TLB_EXEC_BIT   BIT(14)
# define TLB_WRITE_BIT  BIT(15)
#endif

#define TLB_ENTRY_NUM (1 << TLB_PADDR_SIZE)
#define TLB_PADDR_MASK ((1 << TLB_PADDR_SIZE) - 1)
#define TLB_ENABLE_BIT BIT(TLB_PADDR_SIZE)

/* This is used to translate from TLB entry back to physical address. */
#define TLB_PHYS_BASE  \
	(((L2_SRAM_BASE / CONFIG_MM_DRV_PAGE_SIZE) & ~TLB_PADDR_MASK) * CONFIG_MM_DRV_PAGE_SIZE)
#define HPSRAM_SEGMENTS(hpsram_ebb_quantity) \
	((ROUND_DOWN((hpsram_ebb_quantity) + 31u, 32u) / 32u) - 1u)

#define L2_SRAM_PAGES_NUM			(L2_SRAM_SIZE / CONFIG_MM_DRV_PAGE_SIZE)
#define MAX_EBB_BANKS_IN_SEGMENT	32
#define SRAM_BANK_SIZE				(128 * 1024)
#define L2_SRAM_BANK_NUM			(L2_SRAM_SIZE / SRAM_BANK_SIZE)
#define IS_BIT_SET(value, idx)		((value) & (1 << (idx)))

static struct k_spinlock tlb_lock;
extern struct k_spinlock sys_mm_drv_common_lock;

static int hpsram_ref[L2_SRAM_BANK_NUM];

/* declare L2 physical memory block */
SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(
		L2_PHYS_SRAM_REGION,
		CONFIG_MM_DRV_PAGE_SIZE,
		L2_SRAM_PAGES_NUM,
		(uint8_t *) L2_SRAM_BASE);

/* Define a marker which is placed by the linker script just after
 * last explicitly defined section. All .text, .data, .bss and .heap
 * sections should be placed before this marker in the memory.
 * This driver is using the location of the marker to
 * unmap the unused L2 memory and power off corresponding memory banks.
 */
__attribute__((__section__(".unused_ram_start_marker")))
static int unused_l2_sram_start_marker = 0xba0babce;

/**
 * Calculate TLB entry based on physical address.
 *
 * @param pa Page-aligned virutal address.
 * @return TLB entry value.
 */
static inline uint16_t pa_to_tlb_entry(uintptr_t pa)
{
	return (((pa) / CONFIG_MM_DRV_PAGE_SIZE) & TLB_PADDR_MASK);
}

/**
 * Calculate physical address based on TLB entry.
 *
 * @param tlb_entry TLB entry value.
 * @return physcial address pointer.
 */
static inline uintptr_t tlb_entry_to_pa(uint16_t tlb_entry)
{
	return ((((tlb_entry) & TLB_PADDR_MASK) *
		CONFIG_MM_DRV_PAGE_SIZE) + TLB_PHYS_BASE);
}

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
#if defined(CONFIG_SOC_SERIES_INTEL_ACE1X)
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

#if defined(CONFIG_SOC_SERIES_INTEL_ACE1X)
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
#if defined(CONFIG_SOC_SERIES_INTEL_ACE1X)
	if (bank_idx > mtl_hpsram_get_bank_count()) {
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

int sys_mm_drv_map_page(void *virt, uintptr_t phys, uint32_t flags)
{
	k_spinlock_key_t key;
	uint32_t entry_idx, bank_idx;
	uint16_t entry;
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
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
	CHECKIF((va < CONFIG_KERNEL_VM_BASE) ||
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

	bank_idx = get_hpsram_bank_idx(pa);

	if (!hpsram_ref[bank_idx]++) {
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

	va = (uint8_t *)z_soc_cached_ptr(virt);
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
	void *va = z_soc_cached_ptr(virt);

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
	CHECKIF((va < CONFIG_KERNEL_VM_BASE) ||
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

	sys_mem_blocks_free_contiguous(&L2_PHYS_SRAM_REGION,
				       UINT_TO_POINTER(pa), 1);

	bank_idx = get_hpsram_bank_idx(pa);
	if (--hpsram_ref[bank_idx] == 0) {
		sys_mm_drv_hpsram_pwr(bank_idx, false, false);
	}

	k_spin_unlock(&tlb_lock, key);

out:
	return ret;
}

int sys_mm_drv_unmap_region(void *virt, size_t size)
{
	void *va = z_soc_cached_ptr(virt);

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

#if defined(CONFIG_SOC_SERIES_INTEL_ACE1X)
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
	void *va_new = z_soc_cached_ptr(virt_new);
	void *va_old = z_soc_cached_ptr(virt_old);

	return sys_mm_drv_simple_remap_region(va_old, size, va_new);
}

int sys_mm_drv_move_region(void *virt_old, size_t size, void *virt_new,
			   uintptr_t phys_new)
{
	k_spinlock_key_t key;
	size_t offset;
	int ret = 0;

	virt_new = z_soc_cached_ptr(virt_new);
	virt_old = z_soc_cached_ptr(virt_old);

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

	void *va_new = z_soc_cached_ptr(virt_new);
	void *va_old = z_soc_cached_ptr(virt_old);

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
		hpsram_ref[i] = SRAM_BANK_SIZE / CONFIG_MM_DRV_PAGE_SIZE;
	}

	/*
	 * find virtual address range which are unused
	 * in the system
	 */
	uintptr_t unused_l2_start_aligned =
		ROUND_UP(POINTER_TO_UINT(&unused_l2_sram_start_marker),
					 CONFIG_MM_DRV_PAGE_SIZE);

	if (unused_l2_start_aligned < L2_SRAM_BASE ||
	    unused_l2_start_aligned > L2_SRAM_BASE + L2_SRAM_SIZE) {
		__ASSERT(false,
			 "unused l2 pointer is outside of l2 sram range %p\n",
			 unused_l2_start_aligned);
		return -EFAULT;
	}

	/*
	 * Unmap all unused physical pages from the entire
	 * virtual address space to save power
	 */
	size_t unused_size = CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE -
			     unused_l2_start_aligned;

	ret = sys_mm_drv_unmap_region(UINT_TO_POINTER(unused_l2_start_aligned),
				      unused_size);

	return 0;
}

SYS_INIT(sys_mm_drv_mm_init, POST_KERNEL, 0);
