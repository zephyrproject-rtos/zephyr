/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

#include <zephyr/linker/devicetree_regions.h>

#define TEST_SRAM_NODE	DT_NODELABEL(test_sram)
#define TEST_SRAM_SECT	LINKER_DT_NODE_REGION_NAME(TEST_SRAM_NODE)
#define TEST_SRAM_ADDR	DT_REG_ADDR(TEST_SRAM_NODE)
#define TEST_SRAM_SIZE	DT_REG_SIZE(TEST_SRAM_NODE)

uint8_t var_in_test_sram[TEST_SRAM_SIZE] Z_GENERIC_SECTION(TEST_SRAM_SECT);

extern char __SRAM_REGION_start[];
extern char __SRAM_REGION_end[];
extern char __SRAM_REGION_size[];
extern char __SRAM_REGION_load_start[];

ZTEST(devicetree_memory_region, test_memory_region)
{
	zassert_true(!strcmp(LINKER_DT_NODE_REGION_NAME(TEST_SRAM_NODE), "SRAM_REGION"), "");

	zassert_equal_ptr(var_in_test_sram, TEST_SRAM_ADDR, "");

	zassert_equal_ptr(__SRAM_REGION_start, TEST_SRAM_ADDR, "");
	zassert_equal_ptr(__SRAM_REGION_end, TEST_SRAM_ADDR + TEST_SRAM_SIZE, "");
	zassert_equal_ptr(__SRAM_REGION_load_start, TEST_SRAM_ADDR, "");

	zassert_equal((unsigned long) __SRAM_REGION_size, TEST_SRAM_SIZE, "");
}

ZTEST_SUITE(devicetree_memory_region, NULL, NULL, NULL, NULL, NULL);
