/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/linker/devicetree_regions.h>

#define TEST_SRAM1      DT_NODELABEL(test_sram1)
#define TEST_SRAM2      DT_NODELABEL(test_sram2)

ZTEST(devicetree_api_ext, test_linker_regions)
{
	zassert_true(!strcmp(LINKER_DT_NODE_REGION_NAME(TEST_SRAM1), "SRAM_REGION"), "");
	zassert_true(!strcmp(LINKER_DT_NODE_REGION_NAME(TEST_SRAM2), "SRAM_REGION_2"), "");
}

ZTEST_SUITE(devicetree_api_ext, NULL, NULL, NULL, NULL, NULL);
