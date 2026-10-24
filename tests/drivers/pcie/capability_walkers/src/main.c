/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/ztest.h>

#define CONF_WORDS BIT(10)
#define FAKE_READ_LIMIT (CONF_WORDS + 16U)

static uint32_t config_space[CONF_WORDS];
static size_t read_count;
static bool read_limit_hit;

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	ARG_UNUSED(bdf);

	read_count++;
	if (read_count > FAKE_READ_LIMIT) {
		read_limit_hit = true;
		return 0U;
	}

	if (reg >= ARRAY_SIZE(config_space)) {
		return 0xffffffffU;
	}

	return config_space[reg];
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	ARG_UNUSED(bdf);
	ARG_UNUSED(reg);
	ARG_UNUSED(data);
}

/* Keep the included driver from registering its unrelated PCIe bus init hook. */
#undef SYS_INIT
#define SYS_INIT(...)

#include "../../../../../drivers/pcie/host/pcie.c"

static void set_standard_head(unsigned int reg)
{
	config_space[PCIE_CONF_CMDSTAT] = PCIE_CONF_CMDSTAT_CAPS;
	config_space[PCIE_CONF_CAPPTR] = reg << 2;
}

static void set_standard_cap(unsigned int reg, uint32_t id, unsigned int next)
{
	config_space[reg] = id | (next << 10);
}

static void set_extended_cap(unsigned int reg, uint32_t id, unsigned int next)
{
	config_space[reg] = id | BIT(16) | ((next << 2) << 20);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(config_space, 0, sizeof(config_space));
	read_count = 0U;
	read_limit_hit = false;
}

ZTEST(pcie_capability_walkers, test_standard_self_loop_terminates)
{
	set_standard_head(16U);
	set_standard_cap(16U, 0x01U, 16U);

	zassert_equal(pcie_get_cap(PCIE_BDF(0, 0, 0), 0xfeU), 0U);
	zassert_false(read_limit_hit, "standard capability walker exceeded defensive read limit");
}

ZTEST(pcie_capability_walkers, test_standard_two_node_cycle_terminates)
{
	set_standard_head(16U);
	set_standard_cap(16U, 0x01U, 20U);
	set_standard_cap(20U, 0x05U, 16U);

	zassert_equal(pcie_get_cap(PCIE_BDF(0, 0, 0), 0xfeU), 0U);
	zassert_false(read_limit_hit, "standard capability walker exceeded defensive read limit");
}

ZTEST(pcie_capability_walkers, test_standard_valid_capability_is_found)
{
	set_standard_head(16U);
	set_standard_cap(16U, 0x01U, 20U);
	set_standard_cap(20U, 0x05U, 0U);

	zassert_equal(pcie_get_cap(PCIE_BDF(0, 0, 0), 0x05U), 20U);
	zassert_false(read_limit_hit);
}

ZTEST(pcie_capability_walkers, test_extended_self_loop_terminates)
{
	set_extended_cap(PCIE_CONF_EXT_CAPPTR, 0x0001U, PCIE_CONF_EXT_CAPPTR);

	zassert_equal(pcie_get_ext_cap(PCIE_BDF(0, 0, 0), 0x1234U), 0U);
	zassert_false(read_limit_hit, "extended capability walker exceeded defensive read limit");
}

ZTEST(pcie_capability_walkers, test_extended_two_node_cycle_terminates)
{
	set_extended_cap(PCIE_CONF_EXT_CAPPTR, 0x0001U, 72U);
	set_extended_cap(72U, 0x0010U, PCIE_CONF_EXT_CAPPTR);

	zassert_equal(pcie_get_ext_cap(PCIE_BDF(0, 0, 0), 0x1234U), 0U);
	zassert_false(read_limit_hit, "extended capability walker exceeded defensive read limit");
}

ZTEST(pcie_capability_walkers, test_extended_valid_capability_is_found)
{
	set_extended_cap(PCIE_CONF_EXT_CAPPTR, 0x0001U, 72U);
	set_extended_cap(72U, 0x1234U, 0U);

	zassert_equal(pcie_get_ext_cap(PCIE_BDF(0, 0, 0), 0x1234U), 72U);
	zassert_false(read_limit_hit);
}

ZTEST(pcie_capability_walkers, test_extended_zero_header_stops_walk)
{
	config_space[PCIE_CONF_EXT_CAPPTR] = 0U;

	zassert_equal(pcie_get_ext_cap(PCIE_BDF(0, 0, 0), 0x1234U), 0U);
	zassert_equal(read_count, 1U);
}

ZTEST(pcie_capability_walkers, test_extended_all_ones_header_stops_walk)
{
	config_space[PCIE_CONF_EXT_CAPPTR] = 0xffffffffU;

	zassert_equal(pcie_get_ext_cap(PCIE_BDF(0, 0, 0), 0x1234U), 0U);
	zassert_equal(read_count, 1U);
}

ZTEST_SUITE(pcie_capability_walkers, NULL, NULL, before, NULL, NULL);
