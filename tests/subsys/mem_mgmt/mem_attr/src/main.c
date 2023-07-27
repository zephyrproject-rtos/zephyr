/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/mem_mgmt/mem_attr.h>

ZTEST(mem_attr, test_mem_attr)
{
	const struct mem_attr_region_t *region;

	region = mem_attr_get_regions();
	zassert_true(region != NULL, "No regions returned");

	/*
	 * Check the data in the regions
	 */
	zassert_equal(region[0].dt_addr, 0x10000000, "Wrong region address");
	zassert_equal(region[0].dt_size, 0x1000, "Wrong region size");
	zassert_equal(region[0].dt_attr, DT_MEMORY_ATTR_RAM, "Wrong region address");

	zassert_equal(region[1].dt_addr, 0x20000000, "Wrong region address");
	zassert_equal(region[1].dt_size, 0x2000, "Wrong region size");
	zassert_equal(region[1].dt_attr, DT_MEMORY_ATTR_RAM_NOCACHE, "Wrong region address");

	/*
	 * Check the input sanitization
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x0, 0x1000, DT_MEMORY_ATTR_RAM),
		      -ENOTSUP, "Unexpected return value");
	zassert_equal(mem_attr_check_buf((void *) 0x10000000, 0x0, DT_MEMORY_ATTR_RAM),
		      -ENOTSUP, "Unexpected return value");
	zassert_equal(mem_attr_check_buf((void *) 0x10000000, 0x100, DT_MEMORY_ATTR_UNKNOWN + 1),
		      -ENOTSUP, "Unexpected return value");

	/*
	 * Check a buffer with the correct properties
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x10000100, 0x100, DT_MEMORY_ATTR_RAM),
		      0, "Unexpected return value");
	zassert_equal(mem_attr_check_buf((void *) 0x20000000, 0x2000, DT_MEMORY_ATTR_RAM_NOCACHE),
		      0, "Unexpected return value");

	/*
	 * Check a buffer with the wrong attribute
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x20000000, 0x2000, DT_MEMORY_ATTR_RAM),
		      -EINVAL, "Unexpected return value");

	/*
	 * Check a buffer outsize the regions
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x40000000, 0x1000, DT_MEMORY_ATTR_RAM),
		      -ENOBUFS, "Unexpected return value");

	/*
	 * Check a buffer too big for the region
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x10000000, 0x2000, DT_MEMORY_ATTR_RAM),
		      -ENOSPC, "Unexpected return value");

	/*
	 * Check a buffer in a disabled region
	 */
	zassert_equal(mem_attr_check_buf((void *) 0x30000000, 0x1000, DT_MEMORY_ATTR_FLASH),
		      -ENOBUFS, "Unexpected return value");
}

ZTEST_SUITE(mem_attr, NULL, NULL, NULL, NULL, NULL);
