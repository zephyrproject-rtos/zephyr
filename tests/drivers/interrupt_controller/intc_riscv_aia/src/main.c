/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Generic APLIC tests, valid in both MSI and direct delivery mode.
 *
 * Delivery-mode-specific tests live in separate source files gated by
 * the matching Kconfig option (see CMakeLists.txt):
 *   - aplic_msi_imsic.c: CONFIG_RISCV_APLIC_MSI (APLIC MSI mode + IMSIC)
 *   - aplic_direct.c: CONFIG_RISCV_APLIC_DIRECT (APLIC direct mode)
 */

#include <stdint.h>
#include <zephyr/irq.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>

ZTEST_SUITE(intc_riscv_aia, NULL, NULL, NULL, NULL, NULL);

/* Test APLIC register offset calculations */
ZTEST(intc_riscv_aia, test_aplic_sourcecfg_offset)
{
	/* Test sourcecfg offset calculation: SOURCECFG_BASE + (src - 1) * 4 */
	zassert_equal(0x0004, aplic_sourcecfg_off(1), "source 1 offset");
	zassert_equal(0x0008, aplic_sourcecfg_off(2), "source 2 offset");
	zassert_equal(0x000C, aplic_sourcecfg_off(3), "source 3 offset");
	zassert_equal(0x0010, aplic_sourcecfg_off(4), "source 4 offset");
	zassert_equal(0x0104, aplic_sourcecfg_off(65), "source 65 offset");
}

ZTEST(intc_riscv_aia, test_aplic_target_offset)
{
	/* Test target register offset calculation: TARGET_BASE + (src - 1) * 4 */
	zassert_equal(0x3004, aplic_target_off(1), "target 1 offset");
	zassert_equal(0x3008, aplic_target_off(2), "target 2 offset");
	zassert_equal(0x300C, aplic_target_off(3), "target 3 offset");
	zassert_equal(0x3010, aplic_target_off(4), "target 4 offset");
	zassert_equal(0x3104, aplic_target_off(65), "target 65 offset");
}

/* Test APLIC register address constants common to both delivery modes */
ZTEST(intc_riscv_aia, test_aplic_register_addresses)
{
	/* Verify critical APLIC register offsets per AIA spec */
	zassert_equal(0x0000, APLIC_DOMAINCFG, "DOMAINCFG offset");
	zassert_equal(0x0004, APLIC_SOURCECFG_BASE, "SOURCECFG_BASE offset");
	zassert_equal(0x1C00, APLIC_SETIP_BASE, "SETIP_BASE offset");
	zassert_equal(0x1CDC, APLIC_SETIPNUM, "SETIPNUM offset");
	zassert_equal(0x1E00, APLIC_SETIE_BASE, "SETIE_BASE offset");
	zassert_equal(0x1EDC, APLIC_SETIENUM, "SETIENUM offset");
	zassert_equal(0x1F00, APLIC_CLRIE_BASE, "CLRIE_BASE offset");
	zassert_equal(0x1FDC, APLIC_CLRIENUM, "CLRIENUM offset");
	zassert_equal(0x3004, APLIC_TARGET_BASE, "TARGET_BASE offset");
}

/* Test APLIC DOMAINCFG bit definitions */
ZTEST(intc_riscv_aia, test_aplic_domaincfg_bits)
{
	/* Test DOMAINCFG register bit field definitions */
	zassert_equal(BIT(8), APLIC_DOMAINCFG_IE, "IE bit");
	zassert_equal(BIT(2), APLIC_DOMAINCFG_DM, "DM bit");
	zassert_equal(BIT(0), APLIC_DOMAINCFG_BE, "BE bit");
}

/* Test APLIC source mode constants */
ZTEST(intc_riscv_aia, test_aplic_source_modes)
{
	/* Test source mode values per AIA spec */
	zassert_equal(0x0, APLIC_SM_INACTIVE, "SM_INACTIVE");
	zassert_equal(0x1, APLIC_SM_DETACHED, "SM_DETACHED");
	zassert_equal(0x4, APLIC_SM_EDGE_RISE, "SM_EDGE_RISE");
	zassert_equal(0x5, APLIC_SM_EDGE_FALL, "SM_EDGE_FALL");
	zassert_equal(0x6, APLIC_SM_LEVEL_HIGH, "SM_LEVEL_HIGH");
	zassert_equal(0x7, APLIC_SM_LEVEL_LOW, "SM_LEVEL_LOW");
}

/* Test hart index encoding boundaries */
ZTEST(intc_riscv_aia, test_hart_index_boundaries)
{
	/* AIA supports 14-bit hart index (0-16383), same field in both modes */

	/* Test hart 0 */
	uint32_t hart_0 = 0;

	uint32_t encoded_0 = (hart_0 & APLIC_TARGET_HART_MASK) << APLIC_TARGET_HART_SHIFT;

	zassert_equal(0x00000000, encoded_0, "Hart 0 encoding");

	/* Test hart 1 */
	uint32_t hart_1 = 1;

	uint32_t encoded_1 = (hart_1 & APLIC_TARGET_HART_MASK) << APLIC_TARGET_HART_SHIFT;

	zassert_equal(0x00040000, encoded_1, "Hart 1 encoding");

	/* Test hart 2 */
	uint32_t hart_2 = 2;

	uint32_t encoded_2 = (hart_2 & APLIC_TARGET_HART_MASK) << APLIC_TARGET_HART_SHIFT;

	zassert_equal(0x00080000, encoded_2, "Hart 2 encoding");

	/* Test maximum 14-bit hart index (16383) */
	uint32_t hart_max = 16383;

	uint32_t encoded_max = (hart_max & APLIC_TARGET_HART_MASK) << APLIC_TARGET_HART_SHIFT;

	zassert_equal(0xFFFC0000, encoded_max, "Hart 16383 encoding");

	/* Test that values beyond 14-bit are masked correctly */
	uint32_t hart_overflow = 0xFFFFFFFFU;
	uint32_t encoded_overflow = (hart_overflow & APLIC_TARGET_HART_MASK)
				    << APLIC_TARGET_HART_SHIFT;
	zassert_equal(0xFFFC0000, encoded_overflow, "Hart overflow masking");
}

/* Verify functionality of SMAIA extension introduced by AIA */
ZTEST(intc_riscv_aia, test_smaia_isa_ext)
{
	if (!IS_ENABLED(CONFIG_RISCV_ISA_EXT_SMAIA)) {
		ztest_test_skip();
	}

	int ret = -1;

	csr_write(mie, 0);
	csr_write(mieh, 0);

	/*
	 * Start at bit 16: bits 0-15 include standard interrupt bits with
	 * architecture-defined WARL behavior (e.g. VSSIE, VSTIE require H-extension).
	 * Bits 16-63 are AIA local interrupt enables and are unconditionally writable.
	 */
	for (uint32_t irq = 16; irq < 64; irq++) {
		irq_enable(irq);
		ret = irq_is_enabled(irq);
		zassert_equal(ret, 1, "Expected IRQ %u in mie is enabled. ret = %d", irq, ret);

		irq_disable(irq);
		ret = irq_is_enabled(irq);
		zassert_equal(ret, 0, "Expected IRQ %u in mie is disabled. ret = %d", irq, ret);
	}
}
