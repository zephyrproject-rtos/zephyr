/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/arch/cpu.h>

ZTEST_SUITE(intc_riscv_aia, NULL, NULL, NULL, NULL, NULL);

/*
 * APLIC Tests
 */

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

/* Test APLIC register address constants */
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
	zassert_equal(0x1BC0, APLIC_MSIADDRCFG, "MSIADDRCFG offset");
	zassert_equal(0x1BC4, APLIC_MSIADDRCFGH, "MSIADDRCFGH offset");
	zassert_equal(0x3000, APLIC_GENMSI, "GENMSI offset");
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

/* Test APLIC TARGET register field encoding */
ZTEST(intc_riscv_aia, test_aplic_target_encoding)
{
	/* Test TARGET register field positions and masks */
	zassert_equal(18, APLIC_TARGET_HART_SHIFT, "hart shift");
	zassert_equal(0x3FFF, APLIC_TARGET_HART_MASK, "hart mask (14-bit)");
	zassert_equal(BIT(11), APLIC_TARGET_MSI_DEL, "MSI_DEL bit");
	zassert_equal(0x7FF, APLIC_TARGET_EIID_MASK, "EIID mask (11-bit)");

	/* Test building a TARGET value: hart=2, MMSI mode, EIID=65 */
	uint32_t target_val = ((2 & APLIC_TARGET_HART_MASK) << APLIC_TARGET_HART_SHIFT) |
			      APLIC_TARGET_MSI_DEL | (65 & APLIC_TARGET_EIID_MASK);

	zassert_equal(0x00080841, target_val, "TARGET encoding");
}

/* Test APLIC GENMSI register field encoding */
ZTEST(intc_riscv_aia, test_aplic_genmsi_encoding)
{
	/* Test GENMSI register field positions and masks */
	zassert_equal(18, APLIC_GENMSI_HART_SHIFT, "GENMSI hart shift");
	zassert_equal(0x3FFF, APLIC_GENMSI_HART_MASK, "GENMSI hart mask (14-bit)");
	zassert_equal(13, APLIC_GENMSI_CONTEXT_SHIFT, "GENMSI context shift");
	zassert_equal(0x1F, APLIC_GENMSI_CONTEXT_MASK, "GENMSI context mask (5-bit)");
	zassert_equal(BIT(12), APLIC_GENMSI_BUSY, "GENMSI busy bit");
	zassert_equal(BIT(11), APLIC_GENMSI_MMSI_MODE, "GENMSI MMSI mode bit");
	zassert_equal(0x7FF, APLIC_GENMSI_EIID_MASK, "GENMSI EIID mask (11-bit)");

	/* Test building a GENMSI value: hart=1, context=0, MMSI mode, EIID=70 */
	uint32_t genmsi_val = ((1 & APLIC_GENMSI_HART_MASK) << APLIC_GENMSI_HART_SHIFT) |
			      ((0 & APLIC_GENMSI_CONTEXT_MASK) << APLIC_GENMSI_CONTEXT_SHIFT) |
			      APLIC_GENMSI_MMSI_MODE | (70 & APLIC_GENMSI_EIID_MASK);

	zassert_equal(0x00040846, genmsi_val, "GENMSI encoding");
}

/* Test APLIC MSIADDRCFGH geometry field encoding */
ZTEST(intc_riscv_aia, test_aplic_msi_geometry_fields)
{
	/* Test MSI address geometry field positions and masks */
	zassert_equal(31, APLIC_MSIADDRCFGH_L_BIT, "lock bit position");
	zassert_equal(24, APLIC_MSIADDRCFGH_HHXS_SHIFT, "HHXS shift");
	zassert_equal(0x1F, APLIC_MSIADDRCFGH_HHXS_MASK, "HHXS mask (5-bit)");
	zassert_equal(20, APLIC_MSIADDRCFGH_LHXS_SHIFT, "LHXS shift");
	zassert_equal(0x7, APLIC_MSIADDRCFGH_LHXS_MASK, "LHXS mask (3-bit)");
	zassert_equal(16, APLIC_MSIADDRCFGH_HHXW_SHIFT, "HHXW shift");
	zassert_equal(0x7, APLIC_MSIADDRCFGH_HHXW_MASK, "HHXW mask (3-bit)");
	zassert_equal(12, APLIC_MSIADDRCFGH_LHXW_SHIFT, "LHXW shift");
	zassert_equal(0xF, APLIC_MSIADDRCFGH_LHXW_MASK, "LHXW mask (4-bit)");
	zassert_equal(0xFFF, APLIC_MSIADDRCFGH_BAPPN_MASK, "BAPPN mask (12-bit)");
}

/*
 * IMSIC Tests
 */

/* Test IMSIC CSR address definitions */
ZTEST(intc_riscv_aia, test_imsic_csr_addresses)
{
	/* Test direct CSR addresses */
	zassert_equal(0x35C, CSR_MTOPEI, "MTOPEI CSR");
	zassert_equal(0xFB0, CSR_MTOPI, "MTOPI CSR");
	zassert_equal(0x350, CSR_MISELECT, "MISELECT CSR");
	zassert_equal(0x351, CSR_MIREG, "MIREG CSR");
	zassert_equal(0xFC0, CSR_SETEIPNUM_M, "SETEIPNUM_M CSR");
	zassert_equal(0xFC1, CSR_CLREIPNUM_M, "CLREIPNUM_M CSR");

	/* Test indirect CSR addresses */
	zassert_equal(0x70, ICSR_EIDELIVERY, "EIDELIVERY indirect CSR");
	zassert_equal(0x72, ICSR_EITHRESH, "EITHRESH indirect CSR");

	/* Test EIP register addresses (EIP0-EIP7) */
	zassert_equal(0x80, ICSR_EIP0, "EIP0 indirect CSR");
	zassert_equal(0x81, ICSR_EIP1, "EIP1 indirect CSR");
	zassert_equal(0x82, ICSR_EIP2, "EIP2 indirect CSR");
	zassert_equal(0x87, ICSR_EIP7, "EIP7 indirect CSR");

	/* Test EIE register addresses (EIE0-EIE7) */
	zassert_equal(0xC0, ICSR_EIE0, "EIE0 indirect CSR");
	zassert_equal(0xC1, ICSR_EIE1, "EIE1 indirect CSR");
	zassert_equal(0xC2, ICSR_EIE2, "EIE2 indirect CSR");
	zassert_equal(0xC7, ICSR_EIE7, "EIE7 indirect CSR");
}

/* Test IMSIC MTOPEI field masks */
ZTEST(intc_riscv_aia, test_imsic_mtopei_fields)
{
	/* Test MTOPEI register field masks */
	zassert_equal(0x7FF, MTOPEI_EIID_MASK, "EIID mask (11-bit)");
	zassert_equal(16, MTOPEI_PRIO_SHIFT, "Priority shift");
	zassert_equal(0xFF0000, MTOPEI_PRIO_MASK, "Priority mask (8-bit at bit 16)");

	/* Test extracting EIID from MTOPEI value */
	uint32_t mtopei_val = 0x00410042; /* Priority=0x41, EIID=66 */
	uint32_t eiid = mtopei_val & MTOPEI_EIID_MASK;

	zassert_equal(66, eiid, "EIID extraction");

	/* Test extracting priority from MTOPEI value */
	uint32_t prio = (mtopei_val & MTOPEI_PRIO_MASK) >> MTOPEI_PRIO_SHIFT;

	zassert_equal(0x41, prio, "Priority extraction");
}

/* Test IMSIC EIDELIVERY mode definitions */
ZTEST(intc_riscv_aia, test_imsic_eidelivery_modes)
{
	/* Test EIDELIVERY register field values */
	zassert_equal(BIT(0), EIDELIVERY_ENABLE, "enable bit");
	zassert_equal(0x00000000, EIDELIVERY_MODE_MMSI, "MMSI mode (00)");

	/* Test building EIDELIVERY value for MMSI mode */
	uint32_t eidelivery_mmsi = EIDELIVERY_ENABLE | EIDELIVERY_MODE_MMSI;

	zassert_equal(0x00000001, eidelivery_mmsi, "EIDELIVERY MMSI enabled");
}

/* Test IMSIC EIE register indexing */
ZTEST(intc_riscv_aia, test_imsic_eie_indexing)
{
	/* IMSIC implements 8 EIE registers (EIE0-EIE7), 32 IDs each = 256 total EIIDs */

	/* Test EIID to EIE register mapping */
	/* EIID 0-31 -> EIE0, EIID 32-63 -> EIE1, etc. */

	/* Test register index calculation: eiid / 32 */
	uint32_t reg_index_0 = 0 / 32U;

	zassert_equal(0, reg_index_0, "EIID 0 -> EIE0");

	uint32_t reg_index_31 = 31 / 32U;

	zassert_equal(0, reg_index_31, "EIID 31 -> EIE0");

	uint32_t eiid_32 = 32;
	uint32_t reg_index_32 = eiid_32 / 32U;

	zassert_equal(1, reg_index_32, "EIID 32 -> EIE1");

	uint32_t reg_index_65 = 65 / 32U;

	zassert_equal(2, reg_index_65, "EIID 65 -> EIE2");

	uint32_t reg_index_255 = 255 / 32U;

	zassert_equal(7, reg_index_255, "EIID 255 -> EIE7");

	/* Test bit position calculation: eiid % 32 */

	uint32_t bit_0 = 0 % 32U;

	zassert_equal(0, bit_0, "EIID 0 -> bit 0");

	uint32_t bit_31 = 31 % 32U;

	zassert_equal(31, bit_31, "EIID 31 -> bit 31");

	uint32_t eiid_32_bit = 32;
	uint32_t bit_32 = eiid_32_bit % 32U;

	zassert_equal(0, bit_32, "EIID 32 -> bit 0");

	uint32_t bit_65 = 65 % 32U;

	zassert_equal(1, bit_65, "EIID 65 -> bit 1");
}

/* Test IMSIC EIE bit manipulation */
ZTEST(intc_riscv_aia, test_imsic_eie_bit_operations)
{
	/* Test setting bits in EIE registers */
	uint32_t eie0 = 0x00000000;

	/* Enable EIID 0 (bit 0 of EIE0) */
	eie0 |= BIT(0);
	zassert_equal(0x00000001, eie0, "Enable EIID 0");

	/* Enable EIID 31 (bit 31 of EIE0) */
	eie0 |= BIT(31);
	zassert_equal(0x80000001, eie0, "Enable EIID 31");

	/* Test clearing bits */
	eie0 &= ~BIT(0);
	zassert_equal(0x80000000, eie0, "Disable EIID 0");

	/* Test checking bit state */
	zassert_true(!!(eie0 & BIT(31)), "EIID 31 is enabled");
	zassert_false(!!(eie0 & BIT(0)), "EIID 0 is disabled");
}

/* Test IMSIC CSR address calculation for indirect access */
ZTEST(intc_riscv_aia, test_imsic_indirect_csr_addressing)
{
	/* Test calculating indirect CSR address for EIE registers */
	/* EIE0 = 0xC0, EIE1 = 0xC1, ..., EIE7 = 0xC7 */

	uint32_t eie0_addr = ICSR_EIE0 + 0;

	zassert_equal(0xC0, eie0_addr, "EIE0 address");

	uint32_t eie1_addr = ICSR_EIE0 + 1;

	zassert_equal(0xC1, eie1_addr, "EIE1 address");

	uint32_t eie7_addr = ICSR_EIE0 + 7;

	zassert_equal(0xC7, eie7_addr, "EIE7 address");

	/* Test calculating indirect CSR address for EIP registers */

	/* EIP0 = 0x80, EIP1 = 0x81, ..., EIP7 = 0x87 */

	uint32_t eip0_addr = ICSR_EIP0 + 0;

	zassert_equal(0x80, eip0_addr, "EIP0 address");

	uint32_t eip1_addr = ICSR_EIP0 + 1;

	zassert_equal(0x81, eip1_addr, "EIP1 address");

	uint32_t eip7_addr = ICSR_EIP0 + 7;

	zassert_equal(0x87, eip7_addr, "EIP7 address");
}

/*
 * Integration Tests
 */

/* Test that APLIC and IMSIC work together for MSI routing */
ZTEST(intc_riscv_aia, test_aia_msi_routing_encoding)
{
	/* Test encoding an MSI route: source 10 -> hart 1, EIID 65 */
	uint32_t hart = 1;
	uint32_t eiid = 65;

	/* APLIC TARGET register encoding */
	uint32_t target_val = ((hart & APLIC_TARGET_HART_MASK) << APLIC_TARGET_HART_SHIFT) |
			      APLIC_TARGET_MSI_DEL | (eiid & APLIC_TARGET_EIID_MASK);

	zassert_equal(0x00040841, target_val, "MSI routing encoding");

	/* IMSIC EIE register and bit for EIID 65 */
	uint32_t eie_reg_index = eiid / 32U;                /* 65 / 32 = 2 -> EIE2 */
	uint32_t eie_bit = eiid % 32U;                      /* 65 % 32 = 1 -> bit 1 */
	uint32_t eie_icsr_addr = ICSR_EIE0 + eie_reg_index; /* 0xC0 + 2 = 0xC2 */

	zassert_equal(2, eie_reg_index, "EIID 65 -> EIE2");
	zassert_equal(1, eie_bit, "EIID 65 -> bit 1");
	zassert_equal(0xC2, eie_icsr_addr, "EIE2 address");
}

/* Test EIID range boundaries */
ZTEST(intc_riscv_aia, test_eiid_range_boundaries)
{
	/* AIA supports 11-bit EIID (0-2047), but practical limit depends on CONFIG_NUM_IRQS */

	/* Test EIID 0 (reserved, should not be used) */
	uint32_t eiid_0 = 0;

	zassert_equal(0, eiid_0 & APLIC_TARGET_EIID_MASK, "EIID 0 encoding");

	/* Test minimum valid EIID (1) */
	uint32_t eiid_1 = 1;

	zassert_equal(1, eiid_1 & APLIC_TARGET_EIID_MASK, "EIID 1 encoding");

	/* Test common EIID values used in tests */
	uint32_t eiid_65 = 65;

	zassert_equal(65, eiid_65 & APLIC_TARGET_EIID_MASK, "EIID 65 encoding");

	uint32_t eiid_70 = 70;

	zassert_equal(70, eiid_70 & APLIC_TARGET_EIID_MASK, "EIID 70 encoding");

	/* Test maximum 11-bit EIID (2047) */
	uint32_t eiid_max = 2047;

	zassert_equal(2047, eiid_max & APLIC_TARGET_EIID_MASK, "EIID 2047 encoding");

	/* Test that values beyond 11-bit are masked correctly */
	uint32_t eiid_overflow = 0xFFFFFFFFU;

	zassert_equal(0x7FF, eiid_overflow & APLIC_TARGET_EIID_MASK, "EIID overflow masking");
}

/* Test hart index encoding boundaries */
ZTEST(intc_riscv_aia, test_hart_index_boundaries)
{
	/* AIA supports 14-bit hart index (0-16383) */

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

/* Functional tests for IMSIC claim behavior */

static inline int imsic_get_pending(uint32_t eiid)
{
	uint32_t reg_index = eiid / 32U;
	uint32_t bit = eiid % 32U;
	uint32_t eip = micsr_read(ICSR_EIP0 + reg_index);

	return !!(eip & BIT(bit));
}

static inline void imsic_set_pending(uint32_t eiid)
{
	riscv_aia_inject_msi(arch_proc_id(), eiid);
}

static inline void imsic_drain_pending(void)
{
	while (riscv_imsic_claim() != 0) {
	}
}

/* Test that claim() atomically clears pending bit (AIA spec requirement) */
ZTEST(intc_riscv_aia, test_imsic_claim_clears_pending)
{
	const uint32_t test_eiid = 65;
	unsigned int key;

	key = irq_lock();

	riscv_imsic_enable_eiid(test_eiid);
	imsic_set_pending(test_eiid);

	zassert_true(imsic_get_pending(test_eiid), "EIID %u should be pending", test_eiid);

	uint32_t claimed = riscv_imsic_claim();

	zassert_equal(test_eiid, claimed, "claim() should return EIID %u, got %u",
		      test_eiid, claimed);
	zassert_false(imsic_get_pending(test_eiid),
		      "EIID %u pending bit should be cleared after claim()", test_eiid);

	riscv_imsic_disable_eiid(test_eiid);
	irq_unlock(key);
}

/* Test that claim() returns 0 when no interrupt is pending */
ZTEST(intc_riscv_aia, test_imsic_claim_returns_zero_when_empty)
{
	const uint32_t test_eiid = 66;
	unsigned int key;

	key = irq_lock();

	riscv_imsic_enable_eiid(test_eiid);
	imsic_drain_pending();

	uint32_t claimed = riscv_imsic_claim();

	zassert_equal(0, claimed, "claim() should return 0 when no interrupt pending");

	riscv_imsic_disable_eiid(test_eiid);
	irq_unlock(key);
}

/* Test that multiple claims each clear only their own pending bit */
ZTEST(intc_riscv_aia, test_imsic_claim_multiple_pending)
{
	const uint32_t eiid_a = 67;
	const uint32_t eiid_b = 68;
	unsigned int key;

	key = irq_lock();

	riscv_imsic_enable_eiid(eiid_a);
	riscv_imsic_enable_eiid(eiid_b);

	imsic_set_pending(eiid_a);
	imsic_set_pending(eiid_b);

	zassert_true(imsic_get_pending(eiid_a), "EIID %u should be pending", eiid_a);
	zassert_true(imsic_get_pending(eiid_b), "EIID %u should be pending", eiid_b);

	uint32_t first = riscv_imsic_claim();

	zassert_equal(eiid_a, first, "First claim should be EIID %u", eiid_a);
	zassert_false(imsic_get_pending(eiid_a), "EIID %u should be cleared after claim", eiid_a);
	zassert_true(imsic_get_pending(eiid_b), "EIID %u should still be pending", eiid_b);

	uint32_t second = riscv_imsic_claim();

	zassert_equal(eiid_b, second, "Second claim should be EIID %u", eiid_b);
	zassert_false(imsic_get_pending(eiid_a), "EIID %u should be cleared", eiid_a);
	zassert_false(imsic_get_pending(eiid_b), "EIID %u should be cleared", eiid_b);

	riscv_imsic_disable_eiid(eiid_a);
	riscv_imsic_disable_eiid(eiid_b);
	irq_unlock(key);
}
