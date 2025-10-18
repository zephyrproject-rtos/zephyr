/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

void z_riscv_pmp_read_config(unsigned long *pmp_cfg, size_t pmp_cfg_size);
void z_riscv_pmp_read_addr(unsigned long *pmp_addr, size_t pmp_addr_size);

/**
 * @brief Verifies the RISC-V PMP entries configured via Device Tree (zephyr,memattr).
 *
 * @details Reads the PMP configuration (pmpcfg) and address (pmpaddr) registers
 * and asserts that the entries programmed by the CONFIG_MEM_ATTR process match
 * the expected values for the Device Tree-defined memory regions.
 */
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

	const unsigned long expected_pmp_addr[3] = {0x20003fff, 0x20000000, 0x2000c000};
	const uint8_t expected_pmp_cfg_entries[3] = {0x1f, 0x00, 0x0f};

	size_t found_idx = -1;
	bool found_sequence = false;
	const size_t expected_len = ARRAY_SIZE(expected_pmp_addr);

	for (size_t i = 0; i <= CONFIG_PMP_SLOTS - expected_len; ++i) {
		if (memcmp(&current_pmpaddr_regs[i], expected_pmp_addr,
			   expected_len * sizeof(unsigned long)) == 0) {

			if (memcmp(&current_pmp_cfg_entries[i], expected_pmp_cfg_entries,
				   expected_len * sizeof(uint8_t)) == 0) {
				found_sequence = true;
				found_idx = i;
				break;
			}
		}
	}

	zassert_true(found_sequence,
		     "Expected PMP sequence (Addrs: 0x%lx, Cfg: 0x%x, ...) was not found in any "
		     "slot. Last checked slot: %zu",
		     expected_pmp_addr[0], expected_pmp_cfg_entries[0], found_idx);
}

ZTEST(riscv_pmp_memattr_entries, test_dt_pmp_perm_conversion)
{
	uint8_t result;

	result = DT_MEM_RISCV_TO_PMP_PERM(0);
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
