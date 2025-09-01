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
#include <zephyr/drivers/counter.h>

#include <dmm.h>

#define IS_ALIGNED64(x) IS_ALIGNED(x, sizeof(uint64_t))

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
static const struct device *counter = DEVICE_DT_GET(DT_NODELABEL(cycle_timer));
static uint32_t t_delta;

static uint32_t ts_get(void)
{
	uint32_t t;

	(void)counter_get_value(counter, &t);
	return t;
}

static uint32_t ts_from_get(uint32_t from)
{
	return ts_get() - from;
}

static uint32_t cyc_to_us(uint32_t cyc)
{
	return counter_ticks_to_us(counter, cyc);
}

static uint32_t cyc_to_rem_ns(uint32_t cyc)
{
	uint32_t us = counter_ticks_to_us(counter, cyc);
	uint32_t ns;

	cyc = cyc - counter_us_to_ticks(counter, (uint64_t)us);
	ns = counter_ticks_to_us(counter, 1000 * cyc);

	return ns;
}

static void *test_setup(void)
{
	static struct dmm_fixture fixture;
	uint32_t t;

	counter_start(counter);
	t = ts_get();
	t_delta = ts_get() - t;
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
				    void *data, size_t size, bool was_prealloc,
				    bool is_cached, bool print_report)
{
	void *buf;
	int retval;
	uint32_t t;
	bool aligned;

	memset(data, (*fill_value)++, size);
	t = ts_get();
	retval = dmm_buffer_out_prepare(dtr->mem_reg, data, size, &buf);
	t = ts_from_get(t);
	aligned = IS_ALIGNED64(data) && IS_ALIGNED64(buf) && IS_ALIGNED64(size);

	if (print_report) {
		TC_PRINT("%saligned buffer out prepare size:%d buf:%p took %d.%dus (%d cycles)\n",
			aligned ? "" : "not ", size, buf, cyc_to_us(t), cyc_to_rem_ns(t), t);
	}

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

	t = ts_get();
	retval = dmm_buffer_out_release(dtr->mem_reg, buf);
	t = ts_from_get(t);
	if (print_report) {
		TC_PRINT("buffer out release buf:%p size:%d took %d.%dus (%d cycles)\n",
			buf, size, cyc_to_us(t), cyc_to_rem_ns(t), t);
	}
	zassert_ok(retval);
}

static void dmm_check_input_buffer(struct dmm_test_region *dtr, uint32_t *fill_value,
				   void *data, size_t size, bool was_prealloc,
				   bool is_cached, bool print_report)
{
	void *buf;
	int retval;
	uint32_t t;
	uint8_t intermediate_buf[128];
	bool aligned;

	zassert_true(size <= sizeof(intermediate_buf));

	t = ts_get();
	retval = dmm_buffer_in_prepare(dtr->mem_reg, data, size, &buf);
	t = ts_from_get(t);
	aligned = IS_ALIGNED64(data) && IS_ALIGNED64(buf) && IS_ALIGNED64(size);
	zassert_ok(retval);
	if (print_report) {
		TC_PRINT("%saligned buffer in prepare buf:%p size:%d took %d.%dus (%d cycles)\n",
			aligned ? "" : "not ", buf, size, cyc_to_us(t), cyc_to_rem_ns(t), t);
	}
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

	t = ts_get();
	retval = dmm_buffer_in_release(dtr->mem_reg, data, size, buf);
	t = ts_from_get(t);
	if (print_report) {
		TC_PRINT("buffer in release buf:%p size:%d took %d.%dus (%d cycles)\n",
			buf, size, cyc_to_us(t), cyc_to_rem_ns(t), t);
	}
	zassert_ok(retval);

	zassert_mem_equal(data, intermediate_buf, size);
}

ZTEST_USER_F(dmm, test_check_dev_cache_in_allocate)
{
	uint8_t user_data[128] __aligned(sizeof(uint64_t));

	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
			       user_data, 16, false, true, false);
	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
			       user_data, 16, false, true, true);
	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
			       user_data, sizeof(user_data), false, true, true);
}

ZTEST_USER_F(dmm, test_check_dev_cache_in_preallocate)
{
	static uint8_t user_data[16] DMM_MEMORY_SECTION(DUT_CACHE);

	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
			       user_data, sizeof(user_data), true, true, true);
}

ZTEST_USER_F(dmm, test_check_dev_cache_out_allocate)
{
	uint8_t user_data[129] __aligned(sizeof(uint64_t));

	/* First run to get code into ICACHE so that following runs has consistent timing. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
				user_data, 16, false, true, false);

	/* Aligned user buffer. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
				user_data, 16, false, true, true);
	/* Unaligned user buffer. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
				&user_data[1], 16, false, true, true);

	/* Aligned user buffer. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
				user_data, sizeof(user_data) - 1, false, true, true);
	/* Unaligned user buffer. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
				&user_data[1], sizeof(user_data) - 1, false, true, true);
}

ZTEST_USER_F(dmm, test_check_dev_cache_out_preallocate)
{
	static uint8_t user_data[16] DMM_MEMORY_SECTION(DUT_CACHE);

	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_CACHE], &fixture->fill_value,
				user_data, sizeof(user_data), true, true, true);
}

ZTEST_USER_F(dmm, test_check_dev_nocache_in_allocate)
{
	uint8_t user_data[129] __aligned(sizeof(uint64_t));

	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
			       user_data, 16, false, false, false);

	/* Aligned user buffer. */
	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
			       user_data, 16, false, false, true);

	/* Unaligned user buffer. */
	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
			       &user_data[1], 16, false, false, true);

	/* Aligned user buffer. */
	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
			       user_data, sizeof(user_data) - 1, false, false, true);

	/* Unaligned user buffer. */
	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
			       &user_data[1], sizeof(user_data) - 1, false, false, true);
}

ZTEST_USER_F(dmm, test_check_dev_nocache_in_preallocate)
{
	static uint8_t user_data[16] DMM_MEMORY_SECTION(DUT_NOCACHE);

	dmm_check_input_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
			       user_data, sizeof(user_data), true, false, true);
}

ZTEST_USER_F(dmm, test_check_dev_nocache_out_allocate)
{
	uint8_t user_data[129] __aligned(sizeof(uint64_t));

	/* First run to get code into ICACHE so that following results are consistent. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
				user_data, 16, false, false, false);

	/* Aligned user buffer. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
				user_data, 16, false, false, true);
	/* Unaligned user buffer. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
				&user_data[1], 16, false, false, true);

	/* Aligned user buffer. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
				user_data, sizeof(user_data) - 1, false, false, true);
	/* Unaligned user buffer. */
	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
				&user_data[1], sizeof(user_data) - 1, false, false, true);
}

ZTEST_USER_F(dmm, test_check_dev_nocache_out_preallocate)
{
	static uint8_t user_data[16] DMM_MEMORY_SECTION(DUT_NOCACHE);

	dmm_check_output_buffer(&fixture->regions[DMM_TEST_REGION_NOCACHE], &fixture->fill_value,
				user_data, sizeof(user_data), true, false, true);
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

/* Needs to execute before DMM initialization. */
SYS_INIT(dmm_test_prepare, EARLY, 0);
