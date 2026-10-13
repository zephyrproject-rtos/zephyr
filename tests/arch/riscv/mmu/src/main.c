/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/arch/riscv/csr.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <zephyr/ztest.h>

static uint32_t writable_page[1024] __aligned(4096);

ZTEST(riscv_mmu, test_kernel_identity_mappings)
{
	uintptr_t phys;
	uintptr_t satp = csr_read(satp);
	uintptr_t va = (uintptr_t)writable_page;
	uintptr_t *table;
	uintptr_t pte;
	unsigned int levels;
	unsigned int index_bits;

#if defined(CONFIG_RISCV_MMU_SV32)
	zassert_equal(satp >> 31, SATP_MODE_SV32);
	table = (uintptr_t *)((satp & SATP32_PPN) << 12);
	levels = 2;
	index_bits = 10;
#else
	table = (uintptr_t *)((satp & SATP64_PPN) << 12);
	index_bits = 9;
#if defined(CONFIG_RISCV_MMU_SV39)
	zassert_equal(satp >> 60, SATP_MODE_SV39);
	levels = 3;
#else
	zassert_equal(satp >> 60, SATP_MODE_SV48);
	levels = 4;
#endif
#endif
	for (unsigned int level = 0; level < levels; level++) {
		unsigned int shift = 12 + index_bits * (levels - 1 - level);

		pte = table[(va >> shift) & ((1U << index_bits) - 1)];
		if (level != levels - 1) {
			zassert_equal(pte & 0xf, 0x1); /* Non-leaf PTE */
			table = (uintptr_t *)((pte >> 10) << 12);
		}
	}
	zassert_equal(pte & 0xf, 0x7); /* Valid, read, write */
	zassert_equal((pte >> 10) << 12, va);
	zassert_equal(arch_page_phys_get(writable_page, &phys), 0);
	zassert_equal(phys, (uintptr_t)writable_page);

	writable_page[0] = 0x12345678;
	zassert_equal(writable_page[0], 0x12345678);
}

#ifdef CONFIG_DEMAND_PAGING
ZTEST(riscv_mmu, test_page_state_round_trip)
{
	void *addr = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	uintptr_t phys;
	uintptr_t location;
	uintptr_t info;

	zassert_not_null(addr);
	zassert_equal(arch_page_phys_get(addr, &phys), 0);
	*(volatile uint32_t *)addr = 0x12345678;

	/* Keep the frame: this tests PTE transitions without a backing store. */
	arch_mem_page_out(addr, 0);
	zassert_equal(arch_page_location_get(addr, &location), ARCH_PAGE_LOCATION_PAGED_OUT);
	zassert_equal(location, 0);
	zassert_equal(arch_page_info_get(addr, NULL, false), 0);

	arch_mem_page_in(addr, phys);
	zassert_equal(arch_page_location_get(addr, &location), ARCH_PAGE_LOCATION_PAGED_IN);
	zassert_equal(location, phys);
	info = arch_page_info_get(addr, NULL, false);
	zassert_true(info & ARCH_DATA_PAGE_LOADED);
	zassert_false(info & ARCH_DATA_PAGE_DIRTY);
	zassert_equal(*(volatile uint32_t *)addr, 0x12345678);
	/* Anonymous-page markers must survive conversion to and from PTE PPNs. */
	const uintptr_t tags[] = {ARCH_UNPAGED_ANON_ZERO, ARCH_UNPAGED_ANON_UNINIT};

	for (size_t i = 0; i < ARRAY_SIZE(tags); i++) {
		arch_mem_page_out(addr, tags[i]);
		zassert_equal(arch_page_location_get(addr, &location),
			      ARCH_PAGE_LOCATION_PAGED_OUT);
		zassert_equal(location, tags[i]);
		arch_mem_page_in(addr, phys);
		zassert_equal(*(volatile uint32_t *)addr, 0x12345678);
	}
	*(volatile uint32_t *)addr = 0x87654321;
	zassert_true(arch_page_info_get(addr, NULL, false) & ARCH_DATA_PAGE_DIRTY);
	k_mem_unmap(addr, CONFIG_MMU_PAGE_SIZE);
}
#endif

ZTEST_SUITE(riscv_mmu, NULL, NULL, NULL, NULL, NULL);
