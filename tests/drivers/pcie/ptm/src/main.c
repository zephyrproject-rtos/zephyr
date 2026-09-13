/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/ptm.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_BDF PCIE_BDF(0, 1, 0)
#define TEST_PTM_BASE PCIE_CONF_EXT_CAPPTR

#define TEST_PTM_CAP_REG  (TEST_PTM_BASE + (0x04U / sizeof(uint32_t)))
#define TEST_PTM_CTRL_REG (TEST_PTM_BASE + (0x08U / sizeof(uint32_t)))

#define TEST_PTM_CAP_REQUESTER BIT(0)
#define TEST_PTM_CAP_RESPONDER BIT(1)
#define TEST_PTM_CAP_ROOT      BIT(2)
#define TEST_PTM_CTRL_ENABLE   BIT(0)
#define TEST_PTM_CTRL_ROOT     BIT(1)

#define TEST_CTRL_PATTERN 0xA5C35A5CU
#define TEST_CFG_DWORDS   128U
#define TEST_READ_HISTORY 8U
#define TEST_WRITE_HISTORY 4U

static uint32_t config_space[TEST_CFG_DWORDS];
static unsigned int read_regs[TEST_READ_HISTORY];
static unsigned int write_regs[TEST_WRITE_HISTORY];
static uint32_t write_values[TEST_WRITE_HISTORY];
static size_t read_count;
static size_t write_count;
static bool cap_present;

uint32_t pcie_get_ext_cap(pcie_bdf_t bdf, uint32_t cap_id)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_equal(cap_id, PCIE_EXT_CAP_ID_PTM);

	return cap_present ? TEST_PTM_BASE : 0U;
}

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_true(reg < ARRAY_SIZE(config_space),
		     "configuration register %u is out of range", reg);
	zassert_true(read_count < ARRAY_SIZE(read_regs), "read history overflow");

	read_regs[read_count++] = reg;
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

/* Include the real PTM implementation so the static root init path is testable. */
#include "../../../../../drivers/pcie/host/ptm.c"

static struct pcie_dev root_pcie = {
	.bdf = TEST_BDF,
};

static const struct pcie_ptm_root_config root_config = {
	.pcie = &root_pcie,
};

static struct device root_device = {
	.name = "pcie-ptm-root-test",
	.config = &root_config,
};

static void reset_config_space(void)
{
	memset(config_space, 0, sizeof(config_space));
	memset(read_regs, 0, sizeof(read_regs));
	memset(write_regs, 0, sizeof(write_regs));
	memset(write_values, 0, sizeof(write_values));

	read_count = 0U;
	write_count = 0U;
	cap_present = true;

	config_space[TEST_PTM_CAP_REG] = TEST_PTM_CAP_REQUESTER;
	config_space[TEST_PTM_CTRL_REG] = TEST_CTRL_PATTERN;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_config_space();
}

ZTEST(pcie_ptm, test_enable_uses_dword_offsets_and_preserves_control)
{
	zassert_true(pcie_ptm_enable(TEST_BDF));

	zassert_equal(read_count, 2U);
	zassert_equal(read_regs[0], TEST_PTM_CAP_REG);
	zassert_equal(read_regs[1], TEST_PTM_CTRL_REG);
	zassert_equal(write_count, 1U);
	zassert_equal(write_regs[0], TEST_PTM_CTRL_REG);
	zassert_equal(write_values[0], TEST_CTRL_PATTERN | TEST_PTM_CTRL_ENABLE);
	zassert_equal(config_space[TEST_PTM_CTRL_REG], TEST_CTRL_PATTERN | TEST_PTM_CTRL_ENABLE);
	zassert_false(write_values[0] & TEST_PTM_CTRL_ROOT);
}

ZTEST(pcie_ptm, test_root_enable_uses_dword_offsets_and_preserves_control)
{
	config_space[TEST_PTM_CAP_REG] = TEST_PTM_CAP_RESPONDER | TEST_PTM_CAP_ROOT;

	zassert_equal(pcie_ptm_root_init(&root_device), 0);

	zassert_equal(read_count, 2U);
	zassert_equal(read_regs[0], TEST_PTM_CAP_REG);
	zassert_equal(read_regs[1], TEST_PTM_CTRL_REG);
	zassert_equal(write_count, 1U);
	zassert_equal(write_regs[0], TEST_PTM_CTRL_REG);
	zassert_equal(write_values[0],
		      TEST_CTRL_PATTERN | TEST_PTM_CTRL_ENABLE | TEST_PTM_CTRL_ROOT);
	zassert_equal(config_space[TEST_PTM_CTRL_REG],
		      TEST_CTRL_PATTERN | TEST_PTM_CTRL_ENABLE | TEST_PTM_CTRL_ROOT);
}

ZTEST(pcie_ptm, test_root_requires_responder_capability)
{
	config_space[TEST_PTM_CAP_REG] = TEST_PTM_CAP_ROOT;

	zassert_equal(pcie_ptm_root_init(&root_device), -ENOTSUP);

	zassert_equal(read_count, 1U);
	zassert_equal(read_regs[0], TEST_PTM_CAP_REG);
	zassert_equal(write_count, 0U);
	zassert_equal(config_space[TEST_PTM_CTRL_REG], TEST_CTRL_PATTERN);
}

ZTEST(pcie_ptm, test_root_requires_root_capability)
{
	config_space[TEST_PTM_CAP_REG] = TEST_PTM_CAP_RESPONDER;

	zassert_equal(pcie_ptm_root_init(&root_device), -ENOTSUP);

	zassert_equal(read_count, 1U);
	zassert_equal(read_regs[0], TEST_PTM_CAP_REG);
	zassert_equal(write_count, 0U);
	zassert_equal(config_space[TEST_PTM_CTRL_REG], TEST_CTRL_PATTERN);
}

ZTEST(pcie_ptm, test_missing_requester_capability_does_not_write)
{
	config_space[TEST_PTM_CAP_REG] = TEST_PTM_CAP_RESPONDER;

	zassert_false(pcie_ptm_enable(TEST_BDF));

	zassert_equal(read_count, 1U);
	zassert_equal(read_regs[0], TEST_PTM_CAP_REG);
	zassert_equal(write_count, 0U);
	zassert_equal(config_space[TEST_PTM_CTRL_REG], TEST_CTRL_PATTERN);
}

ZTEST(pcie_ptm, test_missing_ptm_capability_does_not_access_config_space)
{
	cap_present = false;

	zassert_false(pcie_ptm_enable(TEST_BDF));

	zassert_equal(read_count, 0U);
	zassert_equal(write_count, 0U);
	zassert_equal(config_space[TEST_PTM_CTRL_REG], TEST_CTRL_PATTERN);
}

ZTEST_SUITE(pcie_ptm, NULL, NULL, before, NULL, NULL);
