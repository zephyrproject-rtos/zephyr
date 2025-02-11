/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <dmm.h>

#define DUT_CACHE   DT_ALIAS(dut_cache)
#define DUT_NOCACHE DT_ALIAS(dut_nocache)

#define DMM_TEST_GET_REG_START(node_id)				\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, memory_regions),	\
		    (DT_REG_ADDR(DT_PHANDLE(node_id, memory_regions))), (0))

#define DMM_TEST_GET_REG_SIZE(node_id)				\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, memory_regions),	\
		    (DT_REG_SIZE(DT_PHANDLE(node_id, memory_regions))), (0))

#if CONFIG_DCACHE
BUILD_ASSERT(DMM_ALIGN_SIZE(DUT_CACHE) == CONFIG_DCACHE_LINE_SIZE);
BUILD_ASSERT(DMM_ALIGN_SIZE(DUT_NOCACHE) == 1);
#endif

struct dmm_test_region {
	void *mem_reg;
	uintptr_t start;
	size_t size;
};

enum {
	DMM_TEST_REGION_CACHE,
	DMM_TEST_REGION_NOCACHE,
	DMM_TEST_REGION_COUNT
};

struct dmm_fixture {
	struct dmm_test_region regions[DMM_TEST_REGION_COUNT];
	uint32_t fill_value;
};

static const struct dmm_test_region dmm_test_regions[DMM_TEST_REGION_COUNT] = {
	[DMM_TEST_REGION_CACHE] = {
		.mem_reg = DMM_DEV_TO_REG(DUT_CACHE),
		.start = DMM_TEST_GET_REG_START(DUT_CACHE),
		.size = DMM_TEST_GET_REG_SIZE(DUT_CACHE)
	},
	[DMM_TEST_REGION_NOCACHE] = {
		.mem_reg = DMM_DEV_TO_REG(DUT_NOCACHE),
		.start = DMM_TEST_GET_REG_START(DUT_NOCACHE),
		.size = DMM_TEST_GET_REG_SIZE(DUT_NOCACHE)
	},
};

static void *test_setup(void)
{
	static struct dmm_fixture fixture;

	memcpy(fixture.regions, dmm_test_regions, sizeof(dmm_test_regions));
	fixture.fill_value = 0x1;
	return &fixture;
}

static void test_cleanup(void *argc)
{
}

static bool dmm_buffer_in_region_check(struct dmm_test_region *dtr, void *buf, size_t size)
{
	uintptr_t start = (uintptr_t)buf;

	return ((start >= dtr->start) && ((start + size) <= (dtr->start + dtr->size)));
}

static void dmm_check_output_buffer(struct dmm_test_region *dtr, uint32_t *fill_value,
				    void *data, size_t size, bool was_prealloc, bool is_cached)
{
	void *buf;
	int retval;

	memset(data, (*fill_value)++, size);
	retval = dmm_buffer_out_prepare(dtr->mem_reg, data, size, &buf);
	zassert_ok(retval);
	if (IS_ENABLED(CONFIG_DCACHE) && is_cached) {
		zassert_true(IS_ALIGNED(buf, CONFIG_DCACHE_LINE_SIZE));
	}

	if (IS_ENABLED(CONFIG_HAS_NORDIC_DMM)) {
		if (was_prealloc) {
			zassert_equal(data, buf);
		} else {
			zassert_not_equal(data, buf);
		}
		zassert_true(dmm_buffer_in_region_check(dtr, buf, size));
	} else {
		zassert_equal(data, buf);
	}
	sys_cache_data_invd_range(buf, size);
	zassert_mem_equal(buf, data, size);

	retval = dmm_buffer_out_release(dtr->mem_reg, buf);
	zassert_ok(retval);
}

static void dmm_check_input_buffer(struct dmm_test_region *dtr, uint32_t *fill_value,
				   void *data, size_t size, bool was_prealloc, bool is_cached)
{
	void *buf;
	int retval;
	uint8_t intermediate_buf[128];

	zassert_true(size < sizeof(intermediate_buf));

	retval = dmm_buffer_in_prepare(dtr->mem_reg, data, size, &buf);
	zassert_ok(retval);
	if (IS_ENABLED(CONFIG_DCACHE) && is_cached) {
		zassert_true(IS_ALIGNED(buf, CONFIG_DCACHE_LINE_SIZE));
	}

	if (IS_ENABLED(CONFIG_HAS_NORDIC_DMM)) {
		if (was_prealloc) {
			zassert_equal(data, buf);
		} else {
			zassert_not_equal(data, buf);
		}
		zassert_true(dmm_buffer_in_region_check(dtr, buf, size));
	} else {
		zassert_equal(data, buf);
	}

	/* Simulate external bus master writing to memory region */
	memset(buf, (*fill_value)++, size);
	sys_cache_data_flush_range(buf, size);
	/* Preserve actual memory region contents before polluting the cache */
	memcpy(intermediate_buf, buf, size);
	if (IS_ENABLED(CONFIG_DCACHE) && is_cached) {
		/* Purposefully pollute the cache to make sure library manages cache properly */
		memset(buf, (*fill_value)++, size);
	}

	retval = dmm_buffer_in_release(dtr->mem_reg, data, size, buf);
	zassert_ok(retval);

	zassert_mem_equal(data, intermediate_buf, size);
}

ZTEST_USER_F(dmm, test_check_dev_cache_in_allocate)
{
	uint8_t user_data[16];

	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
			       user_data, sizeof(user_data), false, true);
}

ZTEST_USER_F(dmm, test_check_dev_cache_in_preallocate)
{
	static uint8_t user_data[16] DMM_MEMORY_SECTION(DUT_CACHE);

	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
			       user_data, sizeof(user_data), true, true);
}

ZTEST_USER_F(dmm, test_check_dev_cache_out_allocate)
{
	uint8_t user_data[16];

	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
				user_data, sizeof(user_data), false, true);
}

ZTEST_USER_F(dmm, test_check_dev_cache_out_preallocate)
{
	static uint8_t user_data[16] DMM_MEMORY_SECTION(DUT_CACHE);

	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
				user_data, sizeof(user_data), true, true);
}

ZTEST_USER_F(dmm, test_check_dev_nocache_in_allocate)
{
	uint8_t user_data[16];

	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
			       user_data, sizeof(user_data), false, false);
}

ZTEST_USER_F(dmm, test_check_dev_nocache_in_preallocate)
{
	static uint8_t user_data[16] DMM_MEMORY_SECTION(DUT_NOCACHE);

	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
			       user_data, sizeof(user_data), true, false);
}

ZTEST_USER_F(dmm, test_check_dev_nocache_out_allocate)
{
	uint8_t user_data[16];

	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
				user_data, sizeof(user_data), false, false);
}

ZTEST_USER_F(dmm, test_check_dev_nocache_out_preallocate)
{
	static uint8_t user_data[16] DMM_MEMORY_SECTION(DUT_NOCACHE);

	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
				user_data, sizeof(user_data), true, false);
}

ZTEST_SUITE(dmm, NULL, test_setup, NULL, test_cleanup, NULL);

int dmm_test_prepare(void)
{
	const struct dmm_test_region *dtr;

	for (size_t i = 0; i < ARRAY_SIZE(dmm_test_regions); i++) {
		dtr = &dmm_test_regions[i];
		memset((void *)dtr->start, 0x00, dtr->size);
	}

	return 0;
}

SYS_INIT(dmm_test_prepare, PRE_KERNEL_1, 0);
