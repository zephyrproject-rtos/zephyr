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

#include <soc.h>
#include <cavs-mem.h>

#include <zephyr/drivers/mm/system_mm.h>
#include "mm_drv_common.h"

DEVICE_MMIO_TOPLEVEL_STATIC(tlb_regs, DT_DRV_INST(0));

#define TLB_BASE \
	((mm_reg_t)DEVICE_MMIO_TOPLEVEL_GET(tlb_regs))

/*
 * Number of significant bits in the page index (defines the size of
 * the table)
 */
#if defined(CONFIG_SOC_SERIES_INTEL_CAVS_V15)
# define TLB_PADDR_SIZE 9
#else
# define TLB_PADDR_SIZE 11
#endif

#define TLB_PADDR_MASK ((1 << TLB_PADDR_SIZE) - 1)
#define TLB_ENABLE_BIT BIT(TLB_PADDR_SIZE)

static struct k_spinlock tlb_lock;

/**
 * Calculate the index to the TLB table.
 *
 * @param vaddr Page-aligned virtual address.
 * @return Index to the TLB table.
 */
static uint32_t get_tlb_entry_idx(uintptr_t vaddr)
{
	return (POINTER_TO_UINT(vaddr) - CONFIG_KERNEL_VM_BASE) /
	       CONFIG_MM_DRV_PAGE_SIZE;
}

int sys_mm_drv_map_page(void *virt, uintptr_t phys, uint32_t flags)
{
	k_spinlock_key_t key;
	uint32_t entry_idx;
	uint16_t entry;
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	int ret = 0;

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

	/* Make sure inputs are page-aligned */
	CHECKIF(!sys_mm_drv_is_addr_aligned(pa) ||
		!sys_mm_drv_is_addr_aligned(va)) {
		ret = -EINVAL;
		goto out;
	}

	/* Check bounds of physical address space */
	CHECKIF((pa < L2_SRAM_BASE) ||
		(pa >= (L2_SRAM_BASE + L2_SRAM_SIZE))) {
		ret = -EINVAL;
		goto out;
	}

	/* Check bounds of virtual address space */
	CHECKIF((va < CONFIG_KERNEL_VM_BASE) ||
		(va >= (CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_SIZE))) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&tlb_lock);

	entry_idx = get_tlb_entry_idx(va);

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
	entry = ((pa / CONFIG_MM_DRV_PAGE_SIZE) & TLB_PADDR_MASK);

	/* Enable the translation in the TLB entry */
	entry |= TLB_ENABLE_BIT;

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
	void *va = z_soc_cached_ptr(virt);

	return sys_mm_drv_simple_map_region(va, phys, size, flags);
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
	uint32_t entry_idx;
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
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
			*phys = (ent & TLB_PADDR_MASK) * CONFIG_MM_DRV_PAGE_SIZE + L2_SRAM_BASE;
		}

		ret = 0;
	}

out:
	return ret;
}

int sys_mm_drv_page_flag_get(void *virt, uint32_t *flags)
{
	ARG_UNUSED(virt);

	/*
	 * There are no caching mode, or R/W, or eXecution (etc.) bits.
	 * So just return 0.
	 */

	*flags = 0U;

	return 0;
}

int sys_mm_drv_update_page_flags(void *virt, uint32_t flags)
{
	ARG_UNUSED(virt);
	ARG_UNUSED(flags);

	/*
	 * There are no caching mode, or R/W, or eXecution (etc.) bits.
	 * So just return 0.
	 */

	return 0;
}

int sys_mm_drv_update_region_flags(void *virt, size_t size,
				   uint32_t flags)
{
	void *va = z_soc_cached_ptr(virt);

	return sys_mm_drv_simple_update_region_flags(va, size, flags);
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
	int ret;

	void *va_new = z_soc_cached_ptr(virt_new);
	void *va_old = z_soc_cached_ptr(virt_old);

	ret = sys_mm_drv_simple_move_region(va_old, size, va_new, phys_new);

	/*
	 * Since memcpy() is done in virtual space, need to
	 * flush the cache to make sure the backing physical
	 * pages have the new data.
	 */
	z_xtensa_cache_flush(va_new, size);

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
