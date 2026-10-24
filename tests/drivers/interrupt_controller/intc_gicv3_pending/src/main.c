/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "intc_gic_common_priv.h"

#define TEST_SPI        (GIC_SPI_INT_BASE + GIC_NUM_CFG_PER_REG)
#define TEST_CONFIG_REG (GICD_ICFGRn + (TEST_SPI / GIC_NUM_CFG_PER_REG) * sizeof(uint32_t))

/* These LPI IDs alias the SPI configuration register without an LPI guard. */
#define SET_LPI   ((TEST_CONFIG_REG - GICD_ISPENDRn) / sizeof(uint32_t) * GIC_NUM_INTR_PER_REG)
#define CLEAR_LPI ((TEST_CONFIG_REG - GICD_ICPENDRn) / sizeof(uint32_t) * GIC_NUM_INTR_PER_REG)

BUILD_ASSERT(SET_LPI >= GIC_LPI_INT_BASE);
BUILD_ASSERT(CLEAR_LPI >= GIC_LPI_INT_BASE);

static uint32_t saved_config;
static uint32_t assert_count;
static bool expecting_assert;

void zassert_fail(const char *cond, const char *file, unsigned int line, const char *fmt, ...)
{
	ARG_UNUSED(cond);
	ARG_UNUSED(fmt);

	zassert_true(expecting_assert, "Unexpected assertion at %s:%u", file, line);
	assert_count++;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	uint32_t enable_mask = BIT_MASK(GIC_NUM_CFG_PER_REG) << (TEST_SPI % GIC_NUM_INTR_PER_REG);
	mem_addr_t enable_reg =
		GICD_ISENABLERn + (TEST_SPI / GIC_NUM_INTR_PER_REG) * sizeof(uint32_t);

	zassert_equal(sys_read32(enable_reg) & enable_mask, 0U, "Test SPI block is in use");
	saved_config = sys_read32(TEST_CONFIG_REG);
	sys_write32(saved_config | GICD_ICFGR_TYPE, TEST_CONFIG_REG);
	zassert_equal(sys_read32(TEST_CONFIG_REG), saved_config | GICD_ICFGR_TYPE,
		      "SPI configuration marker was not written");
	assert_count = 0U;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	expecting_assert = false;
	sys_write32(saved_config, TEST_CONFIG_REG);
	arm_gic_irq_clear_pending(TEST_SPI);
}

ZTEST(gicv3_pending, test_lpi_is_pending)
{
	expecting_assert = true;
	bool pending = arm_gic_irq_is_pending(SET_LPI + 1U);
	bool first_pending = arm_gic_irq_is_pending(GIC_LPI_INT_BASE);

	expecting_assert = false;

	zassert_false(pending, "LPI query read the SPI configuration marker");
	zassert_false(first_pending);
	zassert_equal(assert_count, 2U * IS_ENABLED(CONFIG_ASSERT));
	zassert_equal(sys_read32(TEST_CONFIG_REG), saved_config | GICD_ICFGR_TYPE);
}

ZTEST(gicv3_pending, test_lpi_set_pending)
{
	expecting_assert = true;
	arm_gic_irq_set_pending(GIC_LPI_INT_BASE);
	arm_gic_irq_set_pending(SET_LPI);
	expecting_assert = false;

	zassert_equal(sys_read32(TEST_CONFIG_REG), saved_config | GICD_ICFGR_TYPE,
		      "LPI set changed SPI configuration");
	zassert_equal(assert_count, 2U * IS_ENABLED(CONFIG_ASSERT));
}

ZTEST(gicv3_pending, test_lpi_clear_pending)
{
	expecting_assert = true;
	arm_gic_irq_clear_pending(GIC_LPI_INT_BASE);
	arm_gic_irq_clear_pending(CLEAR_LPI);
	expecting_assert = false;

	zassert_equal(sys_read32(TEST_CONFIG_REG), saved_config | GICD_ICFGR_TYPE,
		      "LPI clear changed SPI configuration");
	zassert_equal(assert_count, 2U * IS_ENABLED(CONFIG_ASSERT));
}

ZTEST(gicv3_pending, test_spi_pending)
{
	zassert_false(arm_gic_irq_is_pending(TEST_SPI));
	arm_gic_irq_set_pending(TEST_SPI);
	zassert_true(arm_gic_irq_is_pending(TEST_SPI));
	arm_gic_irq_clear_pending(TEST_SPI);
	zassert_false(arm_gic_irq_is_pending(TEST_SPI));
	zassert_equal(assert_count, 0U);
}

ZTEST_SUITE(gicv3_pending, NULL, NULL, before, after, NULL);
