/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/pmp.h>

/*
 * Test regions defined using PMP_SOC_REGION_DEFINE macro.
 * These will be collected via iterable sections and programmed
 * into PMP during z_riscv_pmp_init().
 *
 * Use addresses from QEMU flash region (0x20000000-0x24000000)
 */
#define TEST_REGION1_ADDR 0x20010000UL
#define TEST_REGION1_SIZE 0x1000UL
#define TEST_REGION1_PERM (PMP_R | PMP_X)

#define TEST_REGION2_ADDR 0x20020000UL
#define TEST_REGION2_SIZE 0x2000UL
#define TEST_REGION2_PERM (PMP_R | PMP_W)

PMP_SOC_REGION_DEFINE(test_region1, TEST_REGION1_ADDR, TEST_REGION1_ADDR + TEST_REGION1_SIZE,
		      TEST_REGION1_PERM);

PMP_SOC_REGION_DEFINE(test_region2, TEST_REGION2_ADDR, TEST_REGION2_ADDR + TEST_REGION2_SIZE,
		      TEST_REGION2_PERM);

struct expected_region {
	uintptr_t base;
	size_t size;
	uint8_t perm;
	bool found;
};

static struct expected_region expected_regions[] = {
	{
		.base = TEST_REGION1_ADDR,
		.size = TEST_REGION1_SIZE,
		.perm = TEST_REGION1_PERM,
		.found = false,
	},
	{
		.base = TEST_REGION2_ADDR,
		.size = TEST_REGION2_SIZE,
		.perm = TEST_REGION2_PERM,
		.found = false,
	},
};

ZTEST(riscv_pmp_soc_regions, test_soc_regions_configured)
{
	const size_t num_pmpcfg_regs = CONFIG_PMP_SLOTS / sizeof(unsigned long);
	const size_t num_pmpaddr_regs = CONFIG_PMP_SLOTS;

	unsigned long current_pmpcfg_regs[num_pmpcfg_regs];
	unsigned long current_pmpaddr_regs[num_pmpaddr_regs];

	z_riscv_pmp_read_config(current_pmpcfg_regs, num_pmpcfg_regs);
	z_riscv_pmp_read_addr(current_pmpaddr_regs, num_pmpaddr_regs);

	const uint8_t *const cfg_entries = (const uint8_t *)current_pmpcfg_regs;

	for (size_t i = 0; i < ARRAY_SIZE(expected_regions); ++i) {
		expected_regions[i].found = false;
	}

	for (unsigned int index = 0; index < CONFIG_PMP_SLOTS; ++index) {
		unsigned long start, end;
		uint8_t cfg_byte = cfg_entries[index];

		if ((cfg_byte & PMP_A) == 0) {
			continue;
		}

		pmp_decode_region(cfg_byte, current_pmpaddr_regs, index, &start, &end);

		for (size_t i = 0; i < ARRAY_SIZE(expected_regions); ++i) {
			if ((start == expected_regions[i].base) &&
			    (end == expected_regions[i].base + expected_regions[i].size - 1) &&
			    ((cfg_byte & 0x07) == expected_regions[i].perm)) {
				expected_regions[i].found = true;
				break;
			}
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(expected_regions); i++) {
		zassert_true(expected_regions[i].found,
			     "SoC region %zu (base 0x%lx, size 0x%zx, perm 0x%x) "
			     "not found in PMP registers",
			     i, expected_regions[i].base, expected_regions[i].size,
			     expected_regions[i].perm);
	}
}

ZTEST(riscv_pmp_soc_regions, test_soc_regions_are_global)
{
	const size_t num_pmpcfg_regs = CONFIG_PMP_SLOTS / sizeof(unsigned long);
	const size_t num_pmpaddr_regs = CONFIG_PMP_SLOTS;

	unsigned long current_pmpcfg_regs[num_pmpcfg_regs];
	unsigned long current_pmpaddr_regs[num_pmpaddr_regs];

	z_riscv_pmp_read_config(current_pmpcfg_regs, num_pmpcfg_regs);
	z_riscv_pmp_read_addr(current_pmpaddr_regs, num_pmpaddr_regs);

	const uint8_t *const cfg_entries = (const uint8_t *)current_pmpcfg_regs;

	unsigned int region1_index = CONFIG_PMP_SLOTS;

	for (unsigned int index = 0; index < CONFIG_PMP_SLOTS; ++index) {
		unsigned long start, end;
		uint8_t cfg_byte = cfg_entries[index];

		if ((cfg_byte & PMP_A) == 0) {
			continue;
		}

		pmp_decode_region(cfg_byte, current_pmpaddr_regs, index, &start, &end);

		if (start == TEST_REGION1_ADDR) {
			region1_index = index;
			break;
		}
	}

	zassert_true(region1_index < CONFIG_PMP_SLOTS, "Test region 1 not found in PMP entries");

	zassert_true(region1_index < CONFIG_PMP_SLOTS / 2,
		     "SoC region appears too late in PMP entries (index %u), "
		     "may not be a global entry",
		     region1_index);
}

ZTEST(riscv_pmp_soc_regions, test_iterable_section)
{
	size_t count;
	const struct pmp_soc_region *region;

	STRUCT_SECTION_COUNT(pmp_soc_region, &count);
	zassert_true(count >= 2, "Expected at least 2 regions, found %zu", count);

	STRUCT_SECTION_GET(pmp_soc_region, 0, &region);
	zassert_equal((uintptr_t)region->start, TEST_REGION1_ADDR,
		      "Region1 start address mismatch");
	zassert_equal((uintptr_t)region->end, TEST_REGION1_ADDR + TEST_REGION1_SIZE,
		      "Region1 end address mismatch");
	zassert_equal(region->perm, TEST_REGION1_PERM, "Region1 permission mismatch");

	STRUCT_SECTION_GET(pmp_soc_region, 1, &region);
	zassert_equal((uintptr_t)region->start, TEST_REGION2_ADDR,
		      "Region2 start address mismatch");
	zassert_equal((uintptr_t)region->end, TEST_REGION2_ADDR + TEST_REGION2_SIZE,
		      "Region2 end address mismatch");
	zassert_equal(region->perm, TEST_REGION2_PERM, "Region2 permission mismatch");
}

ZTEST_SUITE(riscv_pmp_soc_regions, NULL, NULL, NULL, NULL, NULL);
