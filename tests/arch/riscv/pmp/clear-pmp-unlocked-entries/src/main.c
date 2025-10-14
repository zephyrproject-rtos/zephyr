/* Copyright (c) 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/arch/riscv/pmp.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

void z_riscv_pmp_read_config(unsigned long *pmp_cfg, size_t pmp_cfg_size);

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

/**
 * @brief Test selective PMP cleanup: only clear unlocked entries.
 *
 * This test verifies that the system function `riscv_pmp_clear_all()` correctly zeros out Physical
 * Memory Protection (PMP) entries that are **unlocked** (PMP_L bit clear), while scrupulously
 * preserving the state of all **locked** entries (PMP_L bit set).
 */
ZTEST(riscv_pmp_clear_unlocked_entries, test_riscv_pmp_clear_unlocked_entries)
{
	const size_t num_pmpcfg_regs = CONFIG_PMP_SLOTS / PMPCFG_STRIDE;

	/* Arrays to store the PMP configuration state before and after the clear operation. */
	unsigned long pmp_cfg_before[num_pmpcfg_regs];
	unsigned long pmp_cfg_after[num_pmpcfg_regs];

	/* --- Pre-Clear M-Status Checks (Expected State for Setup) --- */
	zassert_true(riscv_mprv_is_enabled(),
		     "MPRV should be enabled (1) to use the privilege specified by the MPP field.");

	zassert_false(riscv_mpp_is_m_mode(),
		      "MPP should be set to 0x00 (U-Mode) before execution.");

	/* 1. Capture the initial state of all PMP Configuration CSRs. */
	z_riscv_pmp_read_config(pmp_cfg_before, num_pmpcfg_regs);

	/* 2. Execute the function under test. This should clear all UNLOCKED entries. */
	z_riscv_pmp_clear_all();

	/* 3. Capture the final state for comparison. */
	z_riscv_pmp_read_config(pmp_cfg_after, num_pmpcfg_regs);

	/* Cast the configuration arrays to an array of bytes (uint8_t *).
	 * This is crucial because each PMP entry (pmpcfgX) is an 8-bit field,
	 * regardless of the XLEN (32-bit or 64-bit) of the RISC-V architecture.
	 */
	const uint8_t *const pmp_entries_initial = (const uint8_t *)pmp_cfg_before;
	const uint8_t *const pmp_entries_final = (const uint8_t *)pmp_cfg_after;

	for (int index = 0; index < CONFIG_PMP_SLOTS; ++index) {
		const uint8_t initial_entry = pmp_entries_initial[index];
		const uint8_t final_entry = pmp_entries_final[index];

		/* Check if the PMP Lock bit (PMP_L) was set in the initial state. */
		if (initial_entry & PMP_L) {
			/* If LOCKED: The entry MUST remain completely unchanged. */
			zassert_equal(initial_entry, final_entry,
				      "PMP Entry %d (LOCKED) changed: Initial=0x%x, Final=0x%x. "
				      "Locked entries must be preserved.",
				      index, initial_entry, final_entry);
		} else {
			/* If UNLOCKED: The entry MUST be cleared to 0. */
			zassert_equal(final_entry, 0,
				      "PMP Entry %d (UNLOCKED) was not cleared to 0x0. "
				      "Initial=0x%x, Final=0x%x. Unlocked entries must be cleared.",
				      index, initial_entry, final_entry);
		}
	}

	/* --- Post-Clear M-Status Checks (Expected Return State) --- */
	zassert_false(riscv_mprv_is_enabled(),
		      "MPRV should be disabled (0) to ensure M-mode memory accesses use M-mode "
		      "privilege.");

	zassert_true(riscv_mpp_is_m_mode(), "MPP should be set to 0x3 (M-Mode) after boot.");
}

ZTEST_SUITE(riscv_pmp_clear_unlocked_entries, NULL, NULL, NULL, NULL, NULL);
