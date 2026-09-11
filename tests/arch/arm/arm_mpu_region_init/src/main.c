/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Andrei-Edward Popa
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/ztest.h>

#define TEST_REGION_NAME "test_region"
#define TEST_REGION_BASE 0x60000000U
#define TEST_REGION_SIZE 0x10000U

/*
 * ARM_MPU_REGION_INIT() uses this internal helper. The test only needs
 * a known encoding for its fixed 64 KiB test region.
 */
static inline uint32_t size_to_mpu_rasr_size(uint32_t size)
{
	zassert_equal(size, TEST_REGION_SIZE, "Unexpected region size");

	return REGION_64K;
}

ZTEST(arm_mpu_region_init, test_region_init)
{
	const struct arm_mpu_region region = ARM_MPU_REGION_INIT(TEST_REGION_NAME, TEST_REGION_BASE,
								 TEST_REGION_SIZE, REGION_IO_ATTR);
	const uint32_t encoded_size = REGION_64K;

	zassert_str_equal(region.name, TEST_REGION_NAME, "Unexpected region name");
	zassert_equal(region.base, TEST_REGION_BASE, "Unexpected region base");

#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
	const arm_mpu_region_attr_t expected_attr = REGION_IO_ATTR(0U);

	zassert_equal(region.size, encoded_size, "Unexpected region size");
	zassert_equal(region.attr.rasr, expected_attr.rasr,
		      "Region size leaked into region attributes");
#else
	const arm_mpu_region_attr_t expected_attr = REGION_IO_ATTR(encoded_size);

	zassert_equal(region.attr.rasr, expected_attr.rasr,
		      "Region size missing from region attributes");
#endif
}

ZTEST_SUITE(arm_mpu_region_init, NULL, NULL, NULL, NULL, NULL);
