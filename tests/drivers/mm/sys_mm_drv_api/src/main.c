/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/mm/system_mm.h>

ZTEST(sys_mm_drv_api, test_query_memory_region_sanity)
{
	const struct sys_mm_drv_region *regions, *region;

	regions = sys_mm_drv_query_memory_regions();
	zassert_not_null(regions, NULL);

	SYS_MM_DRV_MEMORY_REGION_FOREACH(regions, region)
		; /* just iterate, do nothing */

	zassert_equal(region->size, 0, NULL);

	sys_mm_drv_query_memory_regions_free(regions);
}

ZTEST_SUITE(sys_mm_drv_api, NULL, NULL, NULL, NULL, NULL);
