/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

/* Checks if the Machine Privilege Register Virtualization (MPRV) bit in mstatus is 1 (enabled). */
static bool riscv_mprv_is_enabled(void)
{
	return csr_read(mstatus) & MSTATUS_MPRV;
}

/* Checks if the Machine Previous Privilege (MPP) field in mstatus is set to M-Mode (0b11). */
static bool riscv_mpp_is_m_mode(void)
{
	return (csr_read(mstatus) & MSTATUS_MPP) == MSTATUS_MPP;
}

/* Helper structure to define the expected PMP regions derived from the Device Tree. */
struct expected_region {
	uintptr_t base;
	size_t size;
	uint8_t perm;
	bool found;
};

/*
 * Extract base address, size, and permission for the memory regions
 * defined in the Device Tree under the 'memattr' nodes.
 */
static struct expected_region dt_regions[] = {
	{.base = DT_REG_ADDR(DT_NODELABEL(memattr_region1)),
	 .size = DT_REG_SIZE(DT_NODELABEL(memattr_region1)),
	 .perm = DT_MEM_RISCV_TO_PMP_PERM(
		 DT_PROP(DT_NODELABEL(memattr_region1), zephyr_memory_attr)),
	 .found = false},
	{.base = DT_REG_ADDR(DT_NODELABEL(memattr_region2)),
	 .size = DT_REG_SIZE(DT_NODELABEL(memattr_region2)),
	 .perm = DT_MEM_RISCV_TO_PMP_PERM(
		 DT_PROP(DT_NODELABEL(memattr_region2), zephyr_memory_attr)),
	 .found = false}};

ZTEST(riscv_pmp_memattr_entries, test_pmp_devicetree_memattr_config)
{
	const size_t num_pmpcfg_regs = CONFIG_PMP_SLOTS / sizeof(unsigned long);
	const size_t num_pmpaddr_regs = CONFIG_PMP_SLOTS;

	unsigned long current_pmpcfg_regs[num_pmpcfg_regs];
	unsigned long current_pmpaddr_regs[num_pmpaddr_regs];

	/* Read the current PMP configuration from the control registers */
	z_riscv_pmp_read_config(current_pmpcfg_regs, num_pmpcfg_regs);
	z_riscv_pmp_read_addr(current_pmpaddr_regs, num_pmpaddr_regs);

	const uint8_t *const current_pmp_cfg_entries = (const uint8_t *)current_pmpcfg_regs;

	for (unsigned int index = 0; index < CONFIG_PMP_SLOTS; ++index) {
		unsigned long start, end;
		uint8_t cfg_byte = current_pmp_cfg_entries[index];

		/* Decode the configured PMP region (start and end addresses) */
		pmp_decode_region(cfg_byte, current_pmpaddr_regs, index, &start, &end);

		/* Compare the decoded region against the list of expected DT regions */
		for (size_t i = 0; i < ARRAY_SIZE(dt_regions); ++i) {
			if ((start == dt_regions[i].base) &&
			    (end == dt_regions[i].base + dt_regions[i].size - 1) &&
			    ((cfg_byte & 0x07) == dt_regions[i].perm)) {

				dt_regions[i].found = true;
				break;
			}
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(dt_regions); i++) {
		zassert_true(dt_regions[i].found,
			     "PMP entry for DT region %zu (base 0x%lx, size 0x%zx, perm 0x%x) not "
			     "found.",
			     i + 1, dt_regions[i].base, dt_regions[i].size, dt_regions[i].perm);
	}
}

ZTEST(riscv_pmp_memattr_entries, test_riscv_mprv_mpp_config)
{
	zassert_true(riscv_mprv_is_enabled(),
		     "MPRV should be enabled (1) to use the privilege specified by the MPP field.");

	zassert_false(riscv_mpp_is_m_mode(),
		      "MPP should be set to 0x00 (U-Mode) before execution.");
}

ZTEST(riscv_pmp_memattr_entries, test_dt_pmp_perm_conversion)
{
	uint8_t result;

	result = DT_MEM_RISCV_TO_PMP_PERM(0);
	zassert_equal(result, 0, "Expected 0, got 0x%x", result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_EMPTY);
	zassert_equal(result, 0, "Expected 0, got 0x%x", result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_R);
	zassert_equal(result, PMP_R, "Expected PMP_R (0x%x), got 0x%x", PMP_R, result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_W);
	zassert_equal(result, PMP_W, "Expected PMP_W (0x%x), got 0x%x", PMP_W, result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_X);
	zassert_equal(result, PMP_X, "Expected PMP_X (0x%x), got 0x%x", PMP_X, result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_R | DT_MEM_RISCV_TYPE_IO_W);
	zassert_equal(result, PMP_R | PMP_W, "Expected R|W (0x%x), got 0x%x", PMP_R | PMP_W,
		      result);

	result = DT_MEM_RISCV_TO_PMP_PERM(DT_MEM_RISCV_TYPE_IO_R | DT_MEM_RISCV_TYPE_IO_W |
					  DT_MEM_RISCV_TYPE_IO_X);
	zassert_equal(result, PMP_R | PMP_W | PMP_X, "Expected R|W|X (0x%x), got 0x%x",
		      PMP_R | PMP_W | PMP_X, result);
}

ZTEST_SUITE(riscv_pmp_memattr_entries, NULL, NULL, NULL, NULL, NULL);
