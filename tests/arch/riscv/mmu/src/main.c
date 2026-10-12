/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/arch/riscv/csr.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/ztest.h>

static uint32_t writable_page[1024] __aligned(4096);

ZTEST(riscv_sv32_mmu, test_kernel_identity_mappings)
{
	uintptr_t phys;
	uintptr_t satp = csr_read(satp);
	uintptr_t va = (uintptr_t)writable_page;
	uint32_t *root;
	uint32_t root_pte;
	uint32_t *leaf_table;
	uint32_t leaf_pte;

	zassert_equal(satp >> 31, SATP_MODE_SV32);
	root = (uint32_t *)((satp & SATP32_PPN) << 12);
	root_pte = root[(va >> 22) & 0x3ff];
	zassert_equal(root_pte & 0xf, 0x1); /* Non-leaf PTE */
	leaf_table = (uint32_t *)((root_pte >> 10) << 12);
	leaf_pte = leaf_table[(va >> 12) & 0x3ff];
	zassert_equal(leaf_pte & 0xf, 0x7); /* Valid, read, write */
	zassert_equal((leaf_pte >> 10) << 12, va);
	zassert_equal(arch_page_phys_get(writable_page, &phys), 0);
	zassert_equal(phys, (uintptr_t)writable_page);

	writable_page[0] = 0x12345678;
	zassert_equal(writable_page[0], 0x12345678);
}

ZTEST_SUITE(riscv_sv32_mmu, NULL, NULL, NULL, NULL, NULL);
