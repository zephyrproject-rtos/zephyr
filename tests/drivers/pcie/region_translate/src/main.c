/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/* Include the drivers to exercise their static callbacks without hardware initialization. */
#if defined(TEST_PCIE_ECAM)
#include "../../../../../drivers/pcie/controller/pcie_ecam.c"
#define controller_data  pcie_ecam_data
#define controller_init  pcie_ecam_init
#define region_translate pcie_ecam_region_translate
#elif defined(TEST_PCIE_BRCMSTB)
#include "../../../../../drivers/pcie/controller/pcie_brcmstb.c"
#define controller_data  pcie_brcmstb_data
#define controller_init  pcie_brcmstb_init
#define region_translate pcie_brcmstb_region_translate
#endif

#define MEM_BUS_START    0x10000000UL
#define MEM_PHYS_START   0x80000000UL
#define MEM64_BUS_START  0x100000000UL
#define MEM64_PHYS_START 0x1000000000UL
#define WINDOW_SIZE      0x1000000UL

static struct controller_data data;

static const struct device dev = {
	.data = &data,
};

#if defined(TEST_PCIE_ECAM)
/* The included driver retains references to these unused host operations. */
uint32_t pcie_generic_ctrl_conf_read(mm_reg_t cfg_addr, pcie_bdf_t bdf, unsigned int reg)
{
	ARG_UNUSED(cfg_addr);
	ARG_UNUSED(bdf);
	ARG_UNUSED(reg);
	zassert_unreachable("Unexpected PCIe configuration read");

	return UINT32_MAX;
}

void pcie_generic_ctrl_conf_write(mm_reg_t cfg_addr, pcie_bdf_t bdf, unsigned int reg,
				  uint32_t value)
{
	ARG_UNUSED(cfg_addr);
	ARG_UNUSED(bdf);
	ARG_UNUSED(reg);
	ARG_UNUSED(value);
	zassert_unreachable("Unexpected PCIe configuration write");
}
#endif

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	ARG_UNUSED(controller_init);
	data = (struct controller_data){0};
	data.regions[PCIE_REGION_IO].bus_start = MEM_BUS_START;
	data.regions[PCIE_REGION_IO].phys_start = 0x40000000UL;
	data.regions[PCIE_REGION_IO].size = WINDOW_SIZE;
	data.regions[PCIE_REGION_MEM].bus_start = MEM_BUS_START;
	data.regions[PCIE_REGION_MEM].phys_start = MEM_PHYS_START;
	data.regions[PCIE_REGION_MEM].size = WINDOW_SIZE;
	data.regions[PCIE_REGION_MEM64].bus_start = MEM64_BUS_START;
	data.regions[PCIE_REGION_MEM64].phys_start = MEM64_PHYS_START;
	data.regions[PCIE_REGION_MEM64].size = WINDOW_SIZE;
}

ZTEST(pcie_region_translate, test_mem64_bar_in_mem_window)
{
	uintptr_t addr;

	zassert_true(region_translate(&dev, 0U, true, true, MEM_BUS_START + 0x1000U, &addr));
	zassert_equal(addr, MEM_PHYS_START + 0x1000U);
}

ZTEST(pcie_region_translate, test_mem64_bar_in_mem64_window)
{
	uintptr_t addr;

	zassert_true(region_translate(&dev, 0U, true, true, MEM64_BUS_START + 0x1000U, &addr));
	zassert_equal(addr, MEM64_PHYS_START + 0x1000U);
}

ZTEST(pcie_region_translate, test_mem32_bar)
{
	uintptr_t addr;

	zassert_true(region_translate(&dev, 0U, true, false, MEM_BUS_START + 0x1000U, &addr));
	zassert_equal(addr, MEM_PHYS_START + 0x1000U);
}

ZTEST(pcie_region_translate, test_io_bar)
{
	uintptr_t addr;

	zassert_true(region_translate(&dev, 0U, false, false, MEM_BUS_START + 0x1000U, &addr));
	zassert_equal(addr, 0x40001000UL);
}

ZTEST(pcie_region_translate, test_window_boundaries)
{
	uintptr_t addr;

	zassert_true(region_translate(&dev, 0U, true, true, MEM_BUS_START, &addr));
	zassert_equal(addr, MEM_PHYS_START);
	zassert_true(
		region_translate(&dev, 0U, true, true, MEM_BUS_START + WINDOW_SIZE - 1U, &addr));
	zassert_equal(addr, MEM_PHYS_START + WINDOW_SIZE - 1U);
	addr = 0x1234U;
	zassert_false(region_translate(&dev, 0U, true, true, MEM_BUS_START - 1U, &addr));
	zassert_equal(addr, 0x1234U);
	zassert_false(region_translate(&dev, 0U, true, true, MEM_BUS_START + WINDOW_SIZE, &addr));
	zassert_equal(addr, 0x1234U);
}

ZTEST(pcie_region_translate, test_outside_windows)
{
	uintptr_t addr = 0x1234U;

	zassert_false(region_translate(&dev, 0U, true, true, MEM64_BUS_START + WINDOW_SIZE, &addr));
	zassert_equal(addr, 0x1234U);
}

ZTEST(pcie_region_translate, test_empty_windows)
{
	uintptr_t addr = 0x1234U;

	data.regions[PCIE_REGION_MEM].size = 0U;
	data.regions[PCIE_REGION_MEM64].size = 0U;
	zassert_false(region_translate(&dev, 0U, true, true, MEM_BUS_START, &addr));
	zassert_equal(addr, 0x1234U);
}

ZTEST(pcie_region_translate, test_address_spaces)
{
	uintptr_t addr = 0x1234U;

	data.regions[PCIE_REGION_IO].bus_start = 0x20000000UL;
	zassert_false(region_translate(&dev, 0U, true, false, 0x20000000UL, &addr));
	zassert_equal(addr, 0x1234U);
	zassert_false(region_translate(&dev, 0U, false, false, MEM_BUS_START, &addr));
	zassert_equal(addr, 0x1234U);
}

ZTEST(pcie_region_translate, test_window_at_address_limit)
{
	uintptr_t addr;

	data.regions[PCIE_REGION_MEM64].bus_start = UINTPTR_MAX - 0xffU;
	data.regions[PCIE_REGION_MEM64].phys_start = 0x90000000UL;
	data.regions[PCIE_REGION_MEM64].size = 0x100U;
	zassert_true(region_translate(&dev, 0U, true, true, UINTPTR_MAX, &addr));
	zassert_equal(addr, 0x900000ffUL);
}

ZTEST_SUITE(pcie_region_translate, NULL, NULL, before, NULL, NULL);
