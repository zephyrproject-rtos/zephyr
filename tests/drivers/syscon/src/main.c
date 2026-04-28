/* Copyright 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/ztest.h>
#include <zephyr/linker/devicetree_regions.h>

#define RES_SECT LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(res))

uint8_t var_in_res0[DT_REG_SIZE(DT_NODELABEL(res))] __attribute((__section__(RES_SECT)));

ZTEST(syscon, test_size)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	const size_t expected_size = DT_REG_SIZE(DT_NODELABEL(syscon));
	size_t size;

	zassert_ok(syscon_get_size(dev, &size));
	zassert_equal(size, expected_size, "size(%zu) != expected_size(%zu)", size,
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
	for (size_t i = 0; i < DT_REG_SIZE(DT_NODELABEL(syscon)); ++i) {
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
	for (uint32_t i = 0; i < DT_REG_SIZE(DT_NODELABEL(syscon)); ++i) {
		zassert_ok(syscon_write_reg(dev, i, i));
		zassert_equal(((uint8_t *)base_addr)[i], i);
	}
}

ZTEST(syscon, test_update_bits)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uintptr_t base_addr;
	uint32_t val;

	zassert_ok(syscon_get_base(dev, &base_addr));

	/* Write a known value, then update specific bits */
	zassert_ok(syscon_write_reg(dev, 0, 0xA5));
	zassert_ok(syscon_update_bits(dev, 0, 0x0F, 0x03));
	zassert_ok(syscon_read_reg(dev, 0, &val));
	zassert_equal(val, 0xA3, "expected 0xA3, got 0x%02x", val);

	/* Update with mask that covers all bits */
	zassert_ok(syscon_update_bits(dev, 0, 0xFF, 0x42));
	zassert_ok(syscon_read_reg(dev, 0, &val));
	zassert_equal(val, 0x42);

	/* Update with zero mask changes nothing */
	zassert_ok(syscon_update_bits(dev, 0, 0x00, 0xFF));
	zassert_ok(syscon_read_reg(dev, 0, &val));
	zassert_equal(val, 0x42);
}

ZTEST(syscon, test_update_bits_out_of_bounds)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));

	zassert_equal(syscon_update_bits(dev, DT_REG_SIZE(DT_NODELABEL(syscon)), 0xFF, 0x00),
		      -EINVAL);
}

ZTEST(syscon, test_misaligned_access)
{
	const struct device *const dev32 = DEVICE_DT_GET(DT_NODELABEL(syscon32));
	uint32_t val;

	/* Aligned access should succeed */
	zassert_ok(syscon_write_reg(dev32, 0, 0xDEADBEEF));
	zassert_ok(syscon_read_reg(dev32, 0, &val));
	zassert_equal(val, 0xDEADBEEF);

	zassert_ok(syscon_write_reg(dev32, 4, 0xCAFEBABE));
	zassert_ok(syscon_read_reg(dev32, 4, &val));
	zassert_equal(val, 0xCAFEBABE);

	/* Misaligned accesses should be rejected */
	zassert_equal(syscon_read_reg(dev32, 1, &val), -EINVAL);
	zassert_equal(syscon_read_reg(dev32, 2, &val), -EINVAL);
	zassert_equal(syscon_read_reg(dev32, 3, &val), -EINVAL);
	zassert_equal(syscon_write_reg(dev32, 1, 0), -EINVAL);
	zassert_equal(syscon_write_reg(dev32, 5, 0), -EINVAL);
	zassert_equal(syscon_update_bits(dev32, 3, 0xFF, 0), -EINVAL);
}

ZTEST_SUITE(syscon, NULL, NULL, NULL, NULL, NULL);
