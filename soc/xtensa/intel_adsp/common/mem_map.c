/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <spinlock.h>
#include <sys/__assert.h>
#include <sys/mem_manage.h>
#include <sys/util.h>

#include <soc.h>
#include <soc/memory.h>

#define TLB_BASE 0x3000

/* Number of significant bits in the page index (defines the size of
 * the table)
 */
#ifdef SOC_SERIES_INTEL_CAVS_V15
# define TLB_PADDR_SIZE 9
#else
# define TLB_PADDR_SIZE 11
#endif

#define PAGE_SIZE      4096
#define TLB_PADDR_MASK ((1 << TLB_PADDR_SIZE) - 1)
#define TLB_ENABLE_BIT BIT(TLB_PADDR_SIZE)

static struct k_spinlock soc_mem_map_lock;

/**
 * Calculate the index to the TLB table.
 *
 * @param vaddr Page-aligned virutal address.
 * @return Index to the TLB table.
 */
static uint32_t get_tlb_entry_idx(uintptr_t vaddr)
{
	return ((POINTER_TO_UINT(vaddr) - HP_SRAM_BASE) / PAGE_SIZE);
}

void z_soc_mem_map(void *vaddr, uintptr_t paddr, size_t size, uint32_t flags)
{
	k_spinlock_key_t key;
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	uint32_t entry_idx;
	uint16_t entry;
	size_t offset;

	ARG_UNUSED(flags);

	/* TODO: add assertions on bounds of virtual address space */
	__ASSERT_NO_MSG(paddr >= HP_SRAM_BASE);
	__ASSERT_NO_MSG((paddr + size) <= (HP_SRAM_BASE + HP_SRAM_SIZE));

	/* Make sure inputs are page-aligned */
	__ASSERT_NO_MSG((paddr % PAGE_SIZE) == 0U);
	__ASSERT_NO_MSG((POINTER_TO_UINT(vaddr) % PAGE_SIZE) == 0U);
	__ASSERT_NO_MSG((size % PAGE_SIZE) == 0U);

	key = k_spin_lock(&soc_mem_map_lock);

	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		entry_idx = get_tlb_entry_idx(POINTER_TO_UINT(vaddr) + offset);

		/* The address part of the TLB entry takes the lowest
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
		entry = ((((paddr + offset) / PAGE_SIZE) & TLB_PADDR_MASK)
			 | TLB_ENABLE_BIT);

		tlb_entries[entry_idx] = entry;
	}

	k_spin_unlock(&soc_mem_map_lock, key);
}

void z_soc_mem_unmap(void *vaddr, size_t size)
{
	k_spinlock_key_t key;
	uint32_t entry_idx;
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	size_t offset;

	/* TODO: add assertions on bounds of virtual address space */

	/* Make sure inputs are page-aligned */
	__ASSERT_NO_MSG((POINTER_TO_UINT(vaddr) % PAGE_SIZE) == 0U);
	__ASSERT_NO_MSG((size % PAGE_SIZE) == 0U);

	key = k_spin_lock(&soc_mem_map_lock);

	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		entry_idx = get_tlb_entry_idx(POINTER_TO_UINT(vaddr) + offset);

		/* Simply clear the enable bit */
		tlb_entries[entry_idx] &= ~TLB_ENABLE_BIT;
	}

	k_spin_unlock(&soc_mem_map_lock, key);
}

void *z_soc_mem_phys_addr(void *vaddr)
{
	uint16_t *tlb_entries = UINT_TO_POINTER(TLB_BASE);
	uint16_t ent = tlb_entries[get_tlb_entry_idx(POINTER_TO_UINT(vaddr))];

	return (void *)((ent & TLB_PADDR_MASK) * PAGE_SIZE + HP_SRAM_BASE);
}
