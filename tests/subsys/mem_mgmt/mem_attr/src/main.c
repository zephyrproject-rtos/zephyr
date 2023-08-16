/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>

ZTEST(mem_attr, test_mem_attr)
{
	const struct mem_attr_region_t *region;
	size_t num_regions;

	num_regions = mem_attr_get_regions(&region);
	zassert_equal(num_regions, 2, "No regions returned");

	/*
	 * Check the data in the regions
	 */
	for (size_t idx = 0; idx < num_regions; idx++) {
		if (region[idx].dt_size == 0x1000) {
			zassert_equal(region[idx].dt_addr, 0x10000000, "Wrong region address");
			zassert_equal(region[idx].dt_size, 0x1000, "Wrong region size");
			zassert_equal(region[idx].dt_attr, DT_MEM_ARM_MPU_FLASH |
							   DT_MEM_NON_VOLATILE,
							   "Wrong region address");
			zassert_true((strcmp(region[idx].dt_name, "memory@10000000") == 0),
				     "Wrong name");
		} else {
			zassert_equal(region[idx].dt_addr, 0x20000000, "Wrong region address");
			zassert_equal(region[idx].dt_size, 0x2000, "Wrong region size");
			zassert_equal(region[idx].dt_attr, DT_MEM_ARM_MPU_RAM_NOCACHE,
							   "Wrong region address");
			zassert_true((strcmp(region[idx].dt_name, "memory@20000000") == 0),
				      "Wrong name");
		}
	}

	/*
	 * Check the input sanitization
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x10000000, 0x0, DT_MEM_NON_VOLATILE),
		      -ENOTSUP, "Unexpected return value");

	/*
	 * Check a buffer with the correct properties
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x10000100, 0x100,
					 DT_MEM_ARM_MPU_FLASH | DT_MEM_NON_VOLATILE),
		      0, "Unexpected return value");
	zassert_equal(mem_attr_check_buf((void *) 0x20000000, 0x2000, DT_MEM_ARM_MPU_RAM_NOCACHE),
		      0, "Unexpected return value");

	/*
	 * Check partial attributes
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x10000100, 0x100, DT_MEM_NON_VOLATILE),
		      0, "Unexpected return value");

	/*
	 * Check a buffer with the wrong attribute
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x20000000, 0x2000, DT_MEM_OOO),
		      -EINVAL, "Unexpected return value");

	/*
	 * Check a buffer outsize the regions
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x40000000, 0x1000, DT_MEM_NON_VOLATILE),
		      -ENOBUFS, "Unexpected return value");

	/*
	 * Check a buffer too big for the region
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x10000000, 0x2000, DT_MEM_NON_VOLATILE),
		      -ENOSPC, "Unexpected return value");

	/*
	 * Check a buffer in a disabled region
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x30000000, 0x1000, DT_MEM_OOO),
		      -ENOBUFS, "Unexpected return value");
}

ZTEST_SUITE(mem_attr, NULL, NULL, NULL, NULL, NULL);
