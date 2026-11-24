/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/sys/sys_io.h>

#include "intc_gicv3_priv.h"

#define TEST_SGI_ID  5U
#define TEST_SGI_IRQ 13U
#define TEST_AFF1    0x12U
#define TEST_AFF2    0x34U
#define TEST_AFF3    0x56U

#define TEST_MPIDR(aff0)                                                                           \
	(((uint64_t)(aff0) << MPIDR_AFF0_SHIFT) | ((uint64_t)TEST_AFF1 << MPIDR_AFF1_SHIFT) |      \
	 ((uint64_t)TEST_AFF2 << MPIDR_AFF2_SHIFT) | ((uint64_t)TEST_AFF3 << MPIDR_AFF3_SHIFT))

/* Encoded independently of the architecture helpers to catch bit placement errors. */
#define TEST_SGIR_VALUE(aff3, aff2, aff1, rs, intid, tgt)                                          \
	((((uint64_t)(aff3) & 0xffU) << 48) | (((uint64_t)(rs) & 0xfU) << 44) |                    \
	 (((uint64_t)(aff2) & 0xffU) << 32) | (((uint64_t)(intid) & 0xfU) << 24) |                 \
	 (((uint64_t)(aff1) & 0xffU) << 16) | ((uint64_t)(tgt) & 0xffffU))

static void check_encoding(uint32_t aff0, uint32_t rs, uint16_t target_list)
{
	uint64_t expected =
		TEST_SGIR_VALUE(TEST_AFF3, TEST_AFF2, TEST_AFF1, rs, TEST_SGI_ID, target_list);
	uint64_t actual = 0U;

	zassert_true(gicv3_sgi_encode(TEST_MPIDR(aff0), target_list, TEST_SGI_ID, true, &actual),
		     "Aff0 %u must encode with RSS support", aff0);
	zassert_equal(actual, expected, "Aff0 %u: got 0x%llx, expected 0x%llx", aff0,
		      (unsigned long long)actual, (unsigned long long)expected);
}

ZTEST(intc_gicv3, test_aff0_below_16_uses_range_zero)
{
	check_encoding(0U, 0U, BIT(0));
	check_encoding(15U, 0U, BIT(15));
}

ZTEST(intc_gicv3, test_aff0_above_15_encodes_range_selector)
{
	check_encoding(16U, 1U, BIT(0));
	check_encoding(31U, 1U, BIT(15));
	check_encoding(32U, 2U, BIT(0));
	check_encoding(255U, 15U, BIT(15));
}

ZTEST(intc_gicv3, test_rs_macro_reuses_base_encoding)
{
	uint64_t base = GICV3_SGIR_VALUE(TEST_AFF3, TEST_AFF2, TEST_AFF1, TEST_SGI_ID,
					 SGIR_IRM_TO_AFF, BIT(3));
	uint64_t value = GICV3_SGIR_VALUE_RS(TEST_AFF3, TEST_AFF2, TEST_AFF1, 0x5U, TEST_SGI_ID,
					     SGIR_IRM_TO_AFF, BIT(3));

	zassert_equal(value, base | ((uint64_t)0x5U << SGIR_RS_SHIFT), NULL);
}

ZTEST(intc_gicv3, test_rss_requires_both_capabilities)
{
	zassert_true(gicv3_rss_supported(GICD_TYPER_RSS_MASK, ICC_CTLR_EL1_RSS), NULL);
	zassert_false(gicv3_rss_supported(GICD_TYPER_RSS_MASK, 0U), NULL);
	zassert_false(gicv3_rss_supported(0U, ICC_CTLR_EL1_RSS), NULL);
	zassert_false(gicv3_rss_supported(0U, 0U), NULL);
}

ZTEST(intc_gicv3, test_missing_rss_rejects_range_selected_sgi)
{
	uint64_t sgi_val = 0xdeadbeefdeadbeefULL;
	bool dist_only = gicv3_rss_supported(GICD_TYPER_RSS_MASK, 0U);
	bool cpu_only = gicv3_rss_supported(0U, ICC_CTLR_EL1_RSS);

	zassert_false(gicv3_sgi_encode(TEST_MPIDR(16U), BIT(0), TEST_SGI_ID, dist_only, &sgi_val),
		      "Distributor RSS alone must not enable range-selected SGIs");
	zassert_equal(sgi_val, 0xdeadbeefdeadbeefULL, "Rejected SGI must not be encoded");

	zassert_false(gicv3_sgi_encode(TEST_MPIDR(255U), BIT(15), TEST_SGI_ID, cpu_only, &sgi_val),
		      "CPU interface RSS alone must not enable range-selected SGIs");
	zassert_equal(sgi_val, 0xdeadbeefdeadbeefULL, "Rejected SGI must not be encoded");

	zassert_true(gicv3_sgi_encode(TEST_MPIDR(15U), BIT(15), TEST_SGI_ID, false, &sgi_val),
		     "Aff0 below 16 must keep the legacy encoding");
}

static volatile bool sgi_received;

static void sgi_handler(const void *arg)
{
	ARG_UNUSED(arg);

	sgi_received = true;
}

ZTEST(intc_gicv3, test_missing_rss_blocks_sgi_write)
{
	uint32_t aff0;
	uint64_t mpidr;
	bool dist_rss;
	bool cpu_rss;

	/*
	 * With assertions enabled the rejection path intentionally panics, so
	 * only the no_assert scenario can observe that the write is skipped.
	 */
	if (IS_ENABLED(CONFIG_ASSERT)) {
		ztest_test_skip();
	}

	if (arch_num_cpus() != 1U) {
		ztest_test_skip();
	}

	aff0 = MPIDR_AFFLVL(GET_MPIDR(), 0);
	if (aff0 > GICV3_SGI_AFF0_BLOCK_MASK) {
		ztest_test_skip();
	}

	dist_rss = (sys_read32(GICD_TYPER) & GICD_TYPER_RSS_MASK) != 0U;
	cpu_rss = (read_sysreg(ICC_CTLR_EL1) & ICC_CTLR_EL1_RSS) != 0U;
	if (dist_rss && cpu_rss) {
		ztest_test_skip();
	}

	IRQ_CONNECT(TEST_SGI_IRQ, 0, sgi_handler, NULL, 0);
	irq_enable(TEST_SGI_IRQ);

	/* Positive control: the same IRQ path must deliver a legacy self-SGI. */
	mpidr = GET_MPIDR();
	gic_raise_sgi(TEST_SGI_IRQ, mpidr, BIT(aff0 & GICV3_SGI_AFF0_BLOCK_MASK));
	k_msleep(10);
	zassert_true(sgi_received, "Legacy self SGI was not delivered");
	sgi_received = false;

	/*
	 * Aff0 16 selects block 1. Without RSS the range selector is RES0, so
	 * a write would target the current PE in block 0.
	 */
	mpidr = (mpidr & ~(uint64_t)MPIDR_AFFLVL_MASK) | ((uint64_t)16U << MPIDR_AFF0_SHIFT);
	gic_raise_sgi(TEST_SGI_IRQ, mpidr, BIT(aff0 & GICV3_SGI_AFF0_BLOCK_MASK));

	k_msleep(10);

	zassert_false(sgi_received, "Range-selected SGI must not be written without RSS support");
}

ZTEST_SUITE(intc_gicv3, NULL, NULL, NULL, NULL, NULL);
