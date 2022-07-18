/* Copyright 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/syscon.h>
#include <ztest.h>
#include <zephyr/linker/devicetree_regions.h>

#define RES_SECT LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(res))

uint8_t var_in_res0[DT_REG_SIZE(DT_NODELABEL(syscon))] __attribute((__section__(RES_SECT)));

static void test_size(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	const size_t expected_size = DT_REG_SIZE(DT_NODELABEL(syscon));
	size_t size;

	zassert_not_null(dev, NULL);
	zassert_ok(syscon_get_size(dev, &size), NULL);
	zassert_equal(size, expected_size, "size(0x%x) != expected_size(0x%x)", size,
		      expected_size);
}

static void test_out_of_bounds(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uint32_t val;

	zassert_equal(syscon_read_reg(dev, DT_REG_SIZE(DT_NODELABEL(syscon)), &val), -EINVAL, NULL);
	zassert_equal(syscon_write_reg(dev, DT_REG_SIZE(DT_NODELABEL(syscon)), val), -EINVAL, NULL);
}

static void test_read(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uintptr_t base_addr;
	uint32_t val;

	zassert_ok(syscon_get_base(dev, &base_addr), NULL);
	for (size_t i = 0; i < ARRAY_SIZE(var_in_res0); ++i) {
		((uint8_t *)base_addr)[i] = i;
		zassert_ok(syscon_read_reg(dev, i, &val), NULL);
		zassert_equal(i, val, NULL);
	}
}

static void test_write(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uintptr_t base_addr;

	zassert_ok(syscon_get_base(dev, &base_addr), NULL);
	for (uint32_t i = 0; i < ARRAY_SIZE(var_in_res0); ++i) {
		zassert_ok(syscon_write_reg(dev, i, i), NULL);
		zassert_equal(((uint8_t *)base_addr)[i], i, NULL);
	}
}

void test_main(void)
{
	ztest_test_suite(syscon,
			 ztest_unit_test(test_size),
			 ztest_unit_test(test_out_of_bounds),
			 ztest_unit_test(test_read),
			 ztest_unit_test(test_write));
	ztest_run_test_suite(syscon);
}
