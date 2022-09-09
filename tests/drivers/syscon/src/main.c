/* Copyright 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/ztest.h>
#include <zephyr/linker/devicetree_regions.h>

#define RES_SECT LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(res))

uint8_t var_in_res0[DT_REG_SIZE(DT_NODELABEL(syscon))] __attribute((__section__(RES_SECT)));

ZTEST(syscon, test_size)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	const size_t expected_size = DT_REG_SIZE(DT_NODELABEL(syscon));
	size_t size;

	zassert_not_null(dev, NULL);
	zassert_ok(syscon_get_size(dev, &size));
	zassert_equal(size, expected_size, "size(0x%x) != expected_size(0x%x)", size,
		      expected_size);
}

ZTEST(syscon, test_out_of_bounds)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uint32_t val;

	zassert_equal(syscon_read_reg(dev, DT_REG_SIZE(DT_NODELABEL(syscon)), &val), -EINVAL);
	zassert_equal(syscon_write_reg(dev, DT_REG_SIZE(DT_NODELABEL(syscon)), val), -EINVAL);
}

ZTEST(syscon, test_read)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uintptr_t base_addr;
	uint32_t val;

	zassert_ok(syscon_get_base(dev, &base_addr));
	for (size_t i = 0; i < ARRAY_SIZE(var_in_res0); ++i) {
		((uint8_t *)base_addr)[i] = i;
		zassert_ok(syscon_read_reg(dev, i, &val));
		zassert_equal(i, val);
	}
}

ZTEST(syscon, test_write)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uintptr_t base_addr;

	zassert_ok(syscon_get_base(dev, &base_addr));
	for (uint32_t i = 0; i < ARRAY_SIZE(var_in_res0); ++i) {
		zassert_ok(syscon_write_reg(dev, i, i));
		zassert_equal(((uint8_t *)base_addr)[i], i);
	}
}

ZTEST_SUITE(syscon, NULL, NULL, NULL, NULL, NULL);
