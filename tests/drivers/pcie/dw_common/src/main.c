/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/ztest.h>

#include "pcie_dw_common.h"

#define FAKE_DBI_SIZE 0x301000U
#define FAKE_CFG_SIZE 0x1000U

#define PCIE_DW_PORT_LINK_CONTROL     0x710U
#define PCIE_DW_LINK_WIDTH_SPEED_CTRL 0x80cU
#define PCIE_DW_MISC_CONTROL_1        0x8bcU
#define PCIE_DW_ATU_VIEWPORT          0x900U
#define PCIE_DW_ATU_CR1               0x904U
#define PCIE_DW_ATU_CR2               0x908U
#define PCIE_DW_ATU_LOWER_BASE        0x90cU
#define PCIE_DW_ATU_UPPER_BASE        0x910U
#define PCIE_DW_ATU_LIMIT             0x914U
#define PCIE_DW_ATU_LOWER_TARGET      0x918U
#define PCIE_DW_ATU_UPPER_TARGET      0x91cU
#define PCIE_DW_ATU_UNROLL_BASE       0x300000U
#define PCIE_DW_ATU_UNROLL_STRIDE     0x200U
#define PCIE_DW_DBI2_OFFSET           0x100000U

#define PCIE_DW_ATU_ENABLE    BIT(31)
#define PCIE_DW_ATU_TYPE_MEM  0x0U
#define PCIE_DW_ATU_TYPE_IO   0x2U
#define PCIE_DW_ATU_TYPE_CFG0 0x4U
#define PCIE_DW_ATU_TYPE_CFG1 0x5U

#define PCI_CFG_COMMAND         0x04U
#define PCI_CFG_CLASS_REVISION  0x08U
#define PCI_CFG_BAR0            0x10U
#define PCI_CFG_BAR1            0x14U
#define PCI_CFG_BUS_NUMBERS     0x18U
#define PCI_CFG_CAP_PTR         0x34U
#define PCI_CFG_INTERRUPT_LINE  0x3cU
#define PCI_CAP_EXP             0x40U
#define PCI_EXP_LINK_CAPABILITY 0x0cU
#define PCI_EXP_LINK_CONTROL2   0x30U

static uint32_t fake_dbi[FAKE_DBI_SIZE / sizeof(uint32_t)];
static uint32_t fake_cfg[FAKE_CFG_SIZE / sizeof(uint32_t)];
static struct pcie_dw_data data;
static struct pcie_dw_config fake_config;
static struct pcie_ctrl_config fake_ctrl_config;
static struct device fake_dev = {
	.name = "pcie_dw_fake",
	.data = &data,
	.config = &fake_config,
};

static uint32_t dbi_read(uint32_t offset)
{
	return fake_dbi[offset / sizeof(uint32_t)];
}

static void dbi_write(uint32_t offset, uint32_t value)
{
	fake_dbi[offset / sizeof(uint32_t)] = value;
}

static void pcie_dw_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(fake_dbi, 0, sizeof(fake_dbi));
	memset(fake_cfg, 0, sizeof(fake_cfg));
	memset(&data, 0, sizeof(data));
	memset(&fake_config, 0, sizeof(fake_config));
	memset(&fake_ctrl_config, 0, sizeof(fake_ctrl_config));
	fake_ctrl_config.cfg_addr = (uintptr_t)fake_cfg;
	fake_ctrl_config.cfg_size = sizeof(fake_cfg);
	fake_config.common = &fake_ctrl_config;
	fake_config.dbi_size = sizeof(fake_dbi);
#ifdef DEVICE_MMIO_IS_IN_RAM
	data.pcie_dbi = (mm_reg_t)fake_dbi;
	data.config = (mm_reg_t)fake_cfg;
#else
	fake_config.pcie_dbi.addr = (mm_reg_t)fake_dbi;
	fake_config.config.addr = (mm_reg_t)fake_cfg;
#endif
	data.last_bus = 0xfU;
	data.num_windows = 8U;
}

ZTEST(pcie_dw_common, test_root_complex_setup)
{
	fake_config.root_bus = 0U;
	fake_config.last_bus = 0xfU;
	fake_config.num_lanes = 1U;
	fake_config.max_link_speed = 2U;

	dbi_write(PCIE_DW_PORT_LINK_CONTROL, BIT(7));
	dbi_write(PCI_CFG_COMMAND, BIT(20));
	dbi_write(PCI_CFG_CLASS_REVISION, 0x5aU);
	dbi_write(PCI_CFG_CAP_PTR, PCI_CAP_EXP);
	dbi_write(PCI_CAP_EXP, 0x10U);
	dbi_write(PCI_CAP_EXP + PCI_EXP_LINK_CAPABILITY, 0xfU);
	dbi_write(PCI_CAP_EXP + PCI_EXP_LINK_CONTROL2, 0xfU);

	zassert_ok(pcie_dw_setup_root(&fake_dev));
	zassert_false(data.atu_unroll);
	zassert_true(data.num_windows > 1U);
	zassert_equal(dbi_read(PCIE_DW_ATU_VIEWPORT), 0U);
	zassert_equal(dbi_read(PCIE_DW_PORT_LINK_CONTROL) & BIT(7), 0U);
	zassert_equal(dbi_read(PCIE_DW_PORT_LINK_CONTROL) & GENMASK(21, 16), BIT(16));
	zassert_equal(dbi_read(PCIE_DW_PORT_LINK_CONTROL) & BIT(5), BIT(5));
	zassert_equal(dbi_read(PCIE_DW_LINK_WIDTH_SPEED_CTRL) & GENMASK(12, 8), BIT(8));
	zassert_equal(dbi_read(PCIE_DW_LINK_WIDTH_SPEED_CTRL) & BIT(17), BIT(17));
	zassert_equal(dbi_read(PCIE_DW_MISC_CONTROL_1) & BIT(0), 0U);
	zassert_equal(dbi_read(PCI_CFG_COMMAND) & 0xffffU, BIT(8) | BIT(2) | BIT(1) | BIT(0));
	zassert_equal(dbi_read(PCI_CFG_CLASS_REVISION), 0x0604005aU);
	zassert_equal(dbi_read(PCI_CFG_BUS_NUMBERS) & 0xffffffU, 0x0f0100U);
	zassert_equal(dbi_read(PCI_CFG_INTERRUPT_LINE) & GENMASK(15, 8), BIT(8));
	zassert_equal(dbi_read(PCI_CFG_BAR0), 0U);
	zassert_equal(dbi_read(PCI_CFG_BAR1), 0U);
	zassert_equal(dbi_read(PCIE_DW_DBI2_OFFSET + PCI_CFG_BAR0), 0U);
	zassert_equal(dbi_read(PCI_CAP_EXP + PCI_EXP_LINK_CAPABILITY) & 0xfU, 2U);
	zassert_equal(dbi_read(PCI_CAP_EXP + PCI_EXP_LINK_CONTROL2) & 0xfU, 2U);
}

ZTEST(pcie_dw_common, test_viewport_outbound_window)
{
	struct pcie_dw_region *region = &data.regions[PCIE_DW_REGION_MEM];

	region->phys_start = 0xf0200000U;
	region->bus_start = 0x02000000U;
	region->size = 0x20000U;
	data.atu_unroll = false;

	zassert_ok(pcie_dw_setup_outbound_ranges(&fake_dev));
	zassert_equal(dbi_read(PCIE_DW_ATU_VIEWPORT), 1U);
	zassert_equal(dbi_read(PCIE_DW_ATU_CR1), PCIE_DW_ATU_TYPE_MEM);
	zassert_equal(dbi_read(PCIE_DW_ATU_CR2), PCIE_DW_ATU_ENABLE);
	zassert_equal(dbi_read(PCIE_DW_ATU_LOWER_BASE), 0xf0200000U);
	zassert_equal(dbi_read(PCIE_DW_ATU_UPPER_BASE), 0U);
	zassert_equal(dbi_read(PCIE_DW_ATU_LIMIT), 0xf021ffffU);
	zassert_equal(dbi_read(PCIE_DW_ATU_LOWER_TARGET), 0x02000000U);
	zassert_equal(dbi_read(PCIE_DW_ATU_UPPER_TARGET), 0U);
}

ZTEST(pcie_dw_common, test_unrolled_outbound_window)
{
	const uint32_t atu = PCIE_DW_ATU_UNROLL_BASE + PCIE_DW_ATU_UNROLL_STRIDE;
	struct pcie_dw_region *region = &data.regions[PCIE_DW_REGION_IO];

	region->phys_start = 0xf0100000U;
	region->bus_start = 0x00100000U;
	region->size = 0x1000U;
	data.atu_unroll = true;

	zassert_ok(pcie_dw_setup_outbound_ranges(&fake_dev));
	zassert_equal(dbi_read(atu + 0x00U), PCIE_DW_ATU_TYPE_IO);
	zassert_equal(dbi_read(atu + 0x04U), PCIE_DW_ATU_ENABLE);
	zassert_equal(dbi_read(atu + 0x08U), 0xf0100000U);
	zassert_equal(dbi_read(atu + 0x10U), 0xf0100fffU);
	zassert_equal(dbi_read(atu + 0x14U), 0x00100000U);
}

ZTEST(pcie_dw_common, test_unrolled_outbound_window_bounds)
{
	struct pcie_dw_region *region = &data.regions[PCIE_DW_REGION_IO];

	region->phys_start = 0xf0100000U;
	region->bus_start = 0x00100000U;
	region->size = 0x1000U;
	data.atu_unroll = true;
	fake_config.dbi_size = PCIE_DW_ATU_UNROLL_BASE + PCIE_DW_ATU_UNROLL_STRIDE;

	zassert_equal(pcie_dw_setup_outbound_ranges(&fake_dev), -ERANGE);
}

ZTEST(pcie_dw_common, test_outbound_window_count)
{
	struct pcie_dw_region *io = &data.regions[PCIE_DW_REGION_IO];
	struct pcie_dw_region *mem = &data.regions[PCIE_DW_REGION_MEM];

	io->phys_start = 0xf0100000U;
	io->bus_start = 0x00100000U;
	io->size = 0x1000U;
	mem->phys_start = 0xf0200000U;
	mem->bus_start = 0x02000000U;
	mem->size = 0x20000U;
	data.num_windows = 2U;

	zassert_equal(pcie_dw_setup_outbound_ranges(&fake_dev), -ENOSPC);
}

ZTEST(pcie_dw_common, test_configuration_routing)
{
	const pcie_bdf_t root = PCIE_BDF(0, 0, 0);
	const pcie_bdf_t endpoint = PCIE_BDF(1, 0, 3);

	dbi_write(0, 0xabcd1234U);
	fake_cfg[2] = 0xfeedfaceU;

	zassert_equal(pcie_dw_conf_read(&fake_dev, root, 0), 0xabcd1234U);
	zassert_equal(pcie_dw_conf_read(&fake_dev, PCIE_BDF(0, 1, 0), 0), UINT32_MAX);
	zassert_equal(pcie_dw_conf_read(&fake_dev, root, 1024), UINT32_MAX);
	zassert_equal(pcie_dw_conf_read(&fake_dev, PCIE_BDF(1, 1, 0), 0), UINT32_MAX);
	zassert_equal(pcie_dw_conf_read(&fake_dev, endpoint, 2), 0xfeedfaceU);
	zassert_equal(dbi_read(PCIE_DW_ATU_VIEWPORT), 0U);
	zassert_equal(dbi_read(PCIE_DW_ATU_CR1), PCIE_DW_ATU_TYPE_CFG0);
	zassert_equal(dbi_read(PCIE_DW_ATU_LOWER_TARGET), 0x01030000U);

	pcie_dw_conf_write(&fake_dev, PCIE_BDF(2, 0, 0), 3, 0x11223344U);
	zassert_equal(fake_cfg[3], 0x11223344U);
	zassert_equal(dbi_read(PCIE_DW_ATU_CR1), PCIE_DW_ATU_TYPE_CFG1);
	zassert_equal(dbi_read(PCIE_DW_ATU_LOWER_TARGET), 0x02000000U);

	fake_ctrl_config.cfg_size = sizeof(uint32_t);
	zassert_equal(pcie_dw_conf_read(&fake_dev, endpoint, 1), UINT32_MAX);
}

ZTEST(pcie_dw_common, test_bar_allocation_and_translation)
{
	uintptr_t address;
	bool result;

	data.regions[PCIE_DW_REGION_MEM] = (struct pcie_dw_region){
		.phys_start = 0xf4200000U,
		.bus_start = 0x02000000U,
		.size = 0x300000U,
	};
	data.regions[PCIE_DW_REGION_MEM64] = (struct pcie_dw_region){
		.phys_start = 0x100000000ULL,
		.bus_start = 0x200000000ULL,
		.size = 0x200000U,
	};

	zassert_true(pcie_dw_region_allocate(&fake_dev, 0, true, false, 0x20000U, &address));
	zassert_equal(address, 0x02000000U);
	zassert_true(pcie_dw_region_allocate(&fake_dev, 0, true, false, 0x100000U, &address));
	zassert_equal(address, 0x02100000U);
	zassert_false(pcie_dw_region_allocate(&fake_dev, 0, true, false, 0x400000U, &address));
	zassert_false(pcie_dw_region_allocate(&fake_dev, 0, true, false, 0x30000U, &address));

	result = pcie_dw_region_get_allocate_base(&fake_dev, 0, true, false, 0x100000U, &address);
	zassert_true(result);
	zassert_equal(address, 0x02200000U);
	zassert_true(pcie_dw_region_translate(&fake_dev, 0, true, false, 0x02100000U, &address));
	zassert_equal(address, 0xf4300000U);
	zassert_false(pcie_dw_region_translate(&fake_dev, 0, true, false, 0x03000000U, &address));

	zassert_true(pcie_dw_region_allocate(&fake_dev, 0, true, true, 0x100000U, &address));
	zassert_equal(address, 0x200000000ULL);
	zassert_true(pcie_dw_region_translate(&fake_dev, 0, true, true, 0x200000000ULL, &address));
	zassert_equal(address, 0x100000000ULL);

	zassert_false(pcie_dw_region_allocate(&fake_dev, 0, false, true, 0x10U, &address));
	zassert_false(pcie_dw_region_translate(&fake_dev, 0, false, true, 0x1000U, &address));

	data.regions[PCIE_DW_REGION_IO] = (struct pcie_dw_region){
		.bus_start = 0x1000U,
		.size = 0x100U,
		.allocation_offset = 0x80U,
	};
	result = pcie_dw_region_get_allocate_base(&fake_dev, 0, false, false, 0x1000U, &address);
	zassert_false(result);
}

ZTEST(pcie_dw_common, test_32bit_bar_in_mem64_only)
{
	uintptr_t address;

	data.regions[PCIE_DW_REGION_MEM64] = (struct pcie_dw_region){
		.phys_start = 0x100000000ULL,
		.bus_start = 0x200000000ULL,
		.size = 0x200000U,
	};

	/* No 32-bit memory range: a 32-bit BAR must not be placed above 4 GiB. */
	zassert_false(pcie_dw_region_allocate(&fake_dev, 0, true, false, 0x1000U, &address));

	zassert_true(pcie_dw_region_allocate(&fake_dev, 0, true, true, 0x1000U, &address));
	zassert_equal(address, 0x200000000ULL);

	/* A MEM64 range below 4 GiB can still serve 32-bit BARs. */
	data.regions[PCIE_DW_REGION_MEM64].bus_start = 0x30000000U;
	data.regions[PCIE_DW_REGION_MEM64].allocation_offset = 0U;
	zassert_true(pcie_dw_region_allocate(&fake_dev, 0, true, false, 0x1000U, &address));
	zassert_equal(address, 0x30000000U);
}

ZTEST(pcie_dw_common, test_bridge_base_commits_offset)
{
	uintptr_t address;

	data.regions[PCIE_DW_REGION_MEM] = (struct pcie_dw_region){
		.bus_start = 0x02000000U,
		.size = 0x1000000U,
	};

	zassert_true(pcie_dw_region_allocate(&fake_dev, 0, true, false, 0x20000U, &address));
	zassert_equal(address, 0x02000000U);

	zassert_true(
		pcie_dw_region_get_allocate_base(&fake_dev, 0, true, false, 0x100000U, &address));
	zassert_equal(address, 0x02100000U);

	/* The next allocation must start at the bridge window base. */
	zassert_true(pcie_dw_region_allocate(&fake_dev, 0, true, false, 0x1000U, &address));
	zassert_equal(address, 0x02100000U);
}

ZTEST(pcie_dw_common, test_bridge_64bit_bar_uses_mem_range)
{
	const pcie_bdf_t direct = PCIE_BDF(1, 0, 0);
	const pcie_bdf_t behind_bridge = PCIE_BDF(2, 0, 0);
	uintptr_t address;

	data.root_bus = 0U;
	data.regions[PCIE_DW_REGION_MEM] = (struct pcie_dw_region){
		.phys_start = 0xf4200000U,
		.bus_start = 0x02000000U,
		.size = 0x1000000U,
	};
	data.regions[PCIE_DW_REGION_MEM64] = (struct pcie_dw_region){
		.phys_start = 0x100000000ULL,
		.bus_start = 0x200000000ULL,
		.size = 0x200000U,
	};

	/* Devices directly behind the root port use the 64-bit range. */
	zassert_true(pcie_dw_region_allocate(&fake_dev, direct, true, true, 0x1000U, &address));
	zassert_equal(address, 0x200000000ULL);
	zassert_true(pcie_dw_region_translate(&fake_dev, direct, true, true, address, &address));
	zassert_equal(address, 0x100000000ULL);

	/* A 64-bit BAR behind a bridge must stay in the 32-bit bridge window. */
	zassert_true(
		pcie_dw_region_allocate(&fake_dev, behind_bridge, true, true, 0x1000U, &address));
	zassert_equal(address, 0x02000000U);
	zassert_true(
		pcie_dw_region_translate(&fake_dev, behind_bridge, true, true, address, &address));
	zassert_equal(address, 0xf4200000U);
}

ZTEST(pcie_dw_common, test_bridge_base_at_end_of_address_space)
{
	const size_t size = 0x1000000U;
	uintptr_t address;

	/*
	 * The exclusive end of this range is one past UINTPTR_MAX. The aligned
	 * base at the start of the range is still representable.
	 */
	data.regions[PCIE_DW_REGION_MEM] = (struct pcie_dw_region){
		.bus_start = UINTPTR_MAX - size + 1U,
		.size = size,
	};

	zassert_true(
		pcie_dw_region_get_allocate_base(&fake_dev, 0, true, false, 0x100000U, &address));
	zassert_equal(address, (uintptr_t)(UINTPTR_MAX - size + 1U));
	zassert_equal(data.regions[PCIE_DW_REGION_MEM].allocation_offset, 0U);
}

ZTEST_SUITE(pcie_dw_common, NULL, NULL, pcie_dw_before, NULL, NULL);
