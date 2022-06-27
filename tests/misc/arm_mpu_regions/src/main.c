/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/slist.h>
#include <zephyr/arch/arm/aarch32/mpu/arm_mpu.h>
#include <ztest.h>
#include <string.h>

extern const struct arm_mpu_config mpu_config;

static arm_mpu_region_attr_t cacheable = REGION_RAM_ATTR(REGION_1M);
static arm_mpu_region_attr_t noncacheable = REGION_RAM_NOCACHE_ATTR(REGION_1M);

static void test_regions(void)
{
	int cnt = 0;

	for (size_t i = 0; i < mpu_config.num_regions; i++) {
		const struct arm_mpu_region *r = &mpu_config.mpu_regions[i];

		if (!strcmp(r->name, "SRAM_CACHE")) {
			zassert_equal(r->base, 0x20200000, "Wrong base");
			zassert_equal(r->attr.rasr, cacheable.rasr,
				      "Wrong attr for SRAM_CACHE");
			cnt++;
		} else if (!strcmp(r->name, "SRAM_NO_CACHE")) {
			zassert_equal(r->base, 0x20300000, "Wrong base");
			zassert_equal(r->attr.rasr, noncacheable.rasr,
				      "Wrong attr for SRAM_NO_CACHE");
			cnt++;
		} else if (!strcmp(r->name, "SRAM_DTCM_FAKE")) {
			zassert_equal(r->base, 0xabcdabcd, "Wrong base");
			zassert_equal(r->attr.rasr, cacheable.rasr,
				      "Wrong attr for SRAM_DTCM_FAKE");
			cnt++;
		}
	}

	if (cnt != 3) {
		/*
		 * SRAM0 and SRAM_NO_MPU should not create any MPU region.
		 * Check that.
		 */
		ztest_test_fail();
	}
}

void test_main(void)
{
	ztest_test_suite(test_c_arm_mpu_regions,
			ztest_unit_test(test_regions)
			);

	ztest_run_test_suite(test_c_arm_mpu_regions);
}
