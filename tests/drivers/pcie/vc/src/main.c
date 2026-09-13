/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/vc.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_BDF PCIE_BDF(0, 1, 0)
#define TEST_VC_BASE PCIE_CONF_EXT_CAPPTR

#define TEST_VC_CAP1_REG (TEST_VC_BASE + (0x04U / sizeof(uint32_t)))
#define TEST_VC_RES_CTRL_REG(_vc) \
	(TEST_VC_BASE + ((0x14U + (_vc) * 0x0CU) / sizeof(uint32_t)))
#define TEST_VC_RES_STATUS_REG(_vc) \
	(TEST_VC_BASE + ((0x18U + (_vc) * 0x0CU) / sizeof(uint32_t)))

#define TEST_VC_ENABLE BIT(31)
#define TEST_VC_NEGOTIATION_PENDING BIT(17)
#define TEST_VC_PA_RR_SELECT BIT(17)
#define TEST_VC_ID(_id) ((uint32_t)(_id) << 24)

#define TEST_CFG_DWORDS 128U
#define TEST_WRITE_HISTORY 16U

static uint32_t config_space[TEST_CFG_DWORDS];
static unsigned int write_regs[TEST_WRITE_HISTORY];
static uint32_t write_values[TEST_WRITE_HISTORY];
static size_t write_count;
static bool cap_present;
static unsigned int pending_status_reg;
static int pending_reads_remaining;
static int status_reads_after_write;

uint32_t pcie_get_ext_cap(pcie_bdf_t bdf, uint32_t cap_id)
{
	zassert_equal(bdf, TEST_BDF);

	if (!cap_present) {
		return 0;
	}

	if (cap_id == PCIE_EXT_CAP_ID_VC || cap_id == PCIE_EXT_CAP_ID_MFVC_VC) {
		return TEST_VC_BASE;
	}

	return 0;
}

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_true(reg < ARRAY_SIZE(config_space),
		     "configuration register %u is out of range", reg);

	if (reg == pending_status_reg && write_count > 0) {
		status_reads_after_write++;
		if (pending_reads_remaining > 0) {
			pending_reads_remaining--;
			return config_space[reg] | TEST_VC_NEGOTIATION_PENDING;
		}
	}

	return config_space[reg];
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_true(reg < ARRAY_SIZE(config_space),
		     "configuration register %u is out of range", reg);
	zassert_true(write_count < ARRAY_SIZE(write_regs), "write history overflow");

	config_space[reg] = data;
	write_regs[write_count] = reg;
	write_values[write_count] = data;
	write_count++;
}

static void reset_config_space(void)
{
	memset(config_space, 0, sizeof(config_space));
	memset(write_regs, 0, sizeof(write_regs));
	memset(write_values, 0, sizeof(write_values));

	cap_present = true;
	write_count = 0;
	pending_status_reg = UINT_MAX;
	pending_reads_remaining = 0;
	status_reads_after_write = 0;

	/* One extended VC plus the default VC0. */
	config_space[TEST_VC_CAP1_REG] = 1U;
	config_space[TEST_VC_RES_CTRL_REG(0)] = TEST_VC_ENABLE;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_config_space();
}

ZTEST(pcie_vc, test_enable_writes_control_and_waits_for_negotiation)
{
	pending_status_reg = TEST_VC_RES_STATUS_REG(1);
	pending_reads_remaining = 2;

	zassert_ok(pcie_vc_enable(TEST_BDF));

	zassert_equal(write_count, 1U);
	zassert_equal(write_regs[0], TEST_VC_RES_CTRL_REG(1));
	zassert_true(write_values[0] & TEST_VC_ENABLE);
	zassert_true(config_space[TEST_VC_RES_CTRL_REG(1)] & TEST_VC_ENABLE);
	zassert_equal(status_reads_after_write, 3);
}

ZTEST(pcie_vc, test_disable_writes_control_then_waits_for_negotiation)
{
	config_space[TEST_VC_RES_CTRL_REG(1)] = TEST_VC_ENABLE;
	pending_status_reg = TEST_VC_RES_STATUS_REG(1);
	pending_reads_remaining = 2;

	zassert_ok(pcie_vc_disable(TEST_BDF));

	zassert_equal(write_count, 1U);
	zassert_equal(write_regs[0], TEST_VC_RES_CTRL_REG(1));
	zassert_false(write_values[0] & TEST_VC_ENABLE);
	zassert_false(config_space[TEST_VC_RES_CTRL_REG(1)] & TEST_VC_ENABLE);
	zassert_equal(status_reads_after_write, 3);
}

ZTEST(pcie_vc, test_enable_preflight_avoids_partial_update)
{
	/* Two extended VCs. VC1 is disabled and VC2 is already enabled. */
	config_space[TEST_VC_CAP1_REG] = 2U;
	config_space[TEST_VC_RES_CTRL_REG(2)] = TEST_VC_ENABLE;

	zassert_equal(pcie_vc_enable(TEST_BDF), -EALREADY);
	zassert_equal(write_count, 0U);
	zassert_false(config_space[TEST_VC_RES_CTRL_REG(1)] & TEST_VC_ENABLE);
}

ZTEST(pcie_vc, test_map_tc_uses_total_vc_count_and_writes_each_resource)
{
	struct pcie_vctc_map map = {
		.vc_tc = {PCIE_VC_SET_TC0, PCIE_VC_SET_TC1},
		.vc_count = 2,
	};

	zassert_ok(pcie_vc_map_tc(TEST_BDF, &map));

	zassert_equal(write_count, 2U);
	zassert_equal(write_regs[0], TEST_VC_RES_CTRL_REG(0));
	zassert_equal(write_regs[1], TEST_VC_RES_CTRL_REG(1));
	zassert_equal(config_space[TEST_VC_RES_CTRL_REG(0)],
		      TEST_VC_ENABLE | TEST_VC_PA_RR_SELECT | PCIE_VC_SET_TC0);
	zassert_equal(config_space[TEST_VC_RES_CTRL_REG(1)],
		      TEST_VC_ID(1) | TEST_VC_PA_RR_SELECT | PCIE_VC_SET_TC1);
}

ZTEST(pcie_vc, test_map_tc_rejects_extended_count_instead_of_total_count)
{
	struct pcie_vctc_map map = {
		.vc_tc = {PCIE_VC_SET_TC0},
		.vc_count = 1,
	};

	zassert_equal(pcie_vc_map_tc(TEST_BDF, &map), -EINVAL);
	zassert_equal(write_count, 0U);
}

ZTEST(pcie_vc, test_map_tc_rejects_invalid_maps_without_writes)
{
	struct pcie_vctc_map missing_tc0 = {
		.vc_tc = {PCIE_VC_SET_TC1, PCIE_VC_SET_TC0},
		.vc_count = 2,
	};
	struct pcie_vctc_map duplicate_tc = {
		.vc_tc = {PCIE_VC_SET_TC0, PCIE_VC_SET_TC0 | PCIE_VC_SET_TC1},
		.vc_count = 2,
	};

	zassert_equal(pcie_vc_map_tc(TEST_BDF, NULL), -EINVAL);
	zassert_equal(pcie_vc_map_tc(TEST_BDF, &missing_tc0), -EINVAL);
	zassert_equal(pcie_vc_map_tc(TEST_BDF, &duplicate_tc), -EINVAL);
	zassert_equal(write_count, 0U);
}

ZTEST(pcie_vc, test_missing_capability_is_not_supported)
{
	cap_present = false;

	zassert_equal(pcie_vc_enable(TEST_BDF), -ENOTSUP);
	zassert_equal(pcie_vc_disable(TEST_BDF), -ENOTSUP);
	zassert_equal(write_count, 0U);
}

ZTEST_SUITE(pcie_vc, NULL, NULL, before, NULL, NULL);
