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
#include <zephyr/ztress.h>
#include <zephyr/random/random.h>

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
BUILD_ASSERT(DMM_ALIGN_SIZE(DUT_NOCACHE) == sizeof(uint32_t));
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

ZTEST_USER_F(dmm, test_check_multiple_alloc_and_free)
{
	int retval;
	uint8_t buf[256];
	uint8_t buf2[32];
	void *dmm_buf;
	void *dmm_buf2;
	void *mem_reg = fixture->regions[DMM_TEST_REGION_NOCACHE].mem_reg;
	uintptr_t start_address;
	uint32_t curr_use, max_use;

	if (IS_ENABLED(CONFIG_DMM_STATS)) {
		retval = dmm_stats_get(mem_reg, &start_address, &curr_use, &max_use);
		zassert_ok(retval);
	}

	memset(buf, 0, sizeof(buf));
	memset(buf2, 0, sizeof(buf2));

	retval = dmm_buffer_out_prepare(mem_reg, (void *)buf, sizeof(buf), &dmm_buf);
	zassert_ok(retval);
	zassert_true(dmm_buf != NULL);

	retval = dmm_buffer_out_prepare(mem_reg, (void *)buf2, sizeof(buf2), &dmm_buf2);
	zassert_ok(retval);
	zassert_true(dmm_buf2 != NULL);

	retval = dmm_buffer_out_release(mem_reg, dmm_buf2);
	zassert_ok(retval);
	zassert_true(dmm_buf != NULL);

	retval = dmm_buffer_out_release(mem_reg, dmm_buf);
	zassert_ok(retval);
	zassert_true(dmm_buf != NULL);

	if (IS_ENABLED(CONFIG_DMM_STATS)) {
		uint32_t curr_use2;

		retval = dmm_stats_get(mem_reg, &start_address, &curr_use2, &max_use);
		zassert_ok(retval);
		zassert_equal(curr_use, curr_use2);
		TC_PRINT("Stats start_address:%p current use:%d%% max use:%d%%\n",
			(void *)start_address, curr_use2, max_use);
	}
}

struct dmm_stress_data {
	void *mem_reg;
	void *alloc_ptr[32];
	uint8_t alloc_token[32];
	size_t alloc_len[32];
	atomic_t alloc_mask;
	atomic_t busy_mask;
	atomic_t fails;
	atomic_t cnt;
	bool cached;
};

static void stress_free_op(struct dmm_stress_data *data, int prio, int id)
{
	/* buffer is allocated. */
	uint8_t token = data->alloc_token[id];
	size_t len = data->alloc_len[id];
	uint8_t *ptr = data->alloc_ptr[id];
	int rv;

	for (int j = 0; j < len; j++) {
		uint8_t exp_val = (uint8_t)(token + j);

		if (ptr[j] != exp_val) {
			for (int k = 0; k < len; k++) {
				printk("%02x ", ptr[k]);
			}
		}
		zassert_equal(ptr[j], exp_val, "At %d got:%d exp:%d, len:%d id:%d, alloc_cnt:%d",
				j, ptr[j], exp_val, len, id, (uint32_t)data->cnt);
	}

	rv = dmm_buffer_in_release(data->mem_reg, ptr, len, ptr);
	zassert_ok(rv);
	/* Indicate that buffer is released. */
	atomic_and(&data->alloc_mask, ~BIT(id));
}

static bool stress_alloc_op(struct dmm_stress_data *data, int prio, int id)
{
	uint32_t r32 = sys_rand32_get();
	size_t len = r32 % 512;
	uint8_t *ptr = data->alloc_ptr[id];
	int rv;

	/* Rarely allocate bigger buffer. */
	if ((r32 & 0x7) == 0) {
		len += 512;
	}

	rv = dmm_buffer_in_prepare(data->mem_reg, &r32/*dummy*/, len, (void **)&ptr);
	if (rv < 0) {
		atomic_inc(&data->fails);
		return true;
	}

	uint8_t token = r32 >> 24;

	data->alloc_ptr[id] = ptr;
	data->alloc_len[id] = len;
	data->alloc_token[id] = token;
	for (int j = 0; j < len; j++) {
		ptr[j] = (uint8_t)(j + token);
	}
	if (data->cached) {
		sys_cache_data_flush_range(ptr, len);
	}
	atomic_inc(&data->cnt);
	return false;
}

bool stress_func(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct dmm_stress_data *data = user_data;
	uint32_t r = sys_rand32_get();
	int rpt = r & 0x3;

	r >>= 2;

	for (int i = 0; i < rpt + 1; i++) {
		int id = r % 32;
		int key;
		bool free_op;
		bool clear_bit;

		key = irq_lock();
		if ((data->busy_mask & BIT(id)) == 0) {
			data->busy_mask |= BIT(id);
			if (data->alloc_mask & BIT(id)) {
				free_op = true;
			} else {
				data->alloc_mask |= BIT(id);
				free_op = false;
			}
		} else {
			irq_unlock(key);
			continue;
		}

		irq_unlock(key);
		r >>= 5;

		if (free_op) {
			stress_free_op(data, prio, id);
			clear_bit = true;
		} else {
			clear_bit = stress_alloc_op(data, prio, id);
		}

		key = irq_lock();
		data->busy_mask &= ~BIT(id);
		if (clear_bit) {
			data->alloc_mask &= ~BIT(id);
		}
		irq_unlock(key);
	}

	return true;
}

static void free_all(struct dmm_stress_data *data)
{
	while (data->alloc_mask) {
		int id = 31 - __builtin_clz(data->alloc_mask);

		stress_free_op(data, 0, id);
		data->alloc_mask &= ~BIT(id);
	}
}

static void stress_allocator(void *mem_reg, bool cached)
{
	uint32_t timeout = 3000;
	struct dmm_stress_data ctx;
	int rv;
	uint32_t curr_use;

	if (mem_reg == NULL) {
		ztest_test_skip();
	}

	memset(&ctx, 0, sizeof(ctx));
	ctx.mem_reg = mem_reg;
	ctx.cached = cached;

	if (IS_ENABLED(CONFIG_DMM_STATS)) {
		rv = dmm_stats_get(ctx.mem_reg, NULL, &curr_use, NULL);
		zassert_ok(rv);
	}

	ztress_set_timeout(K_MSEC(timeout));

	ZTRESS_EXECUTE(ZTRESS_THREAD(stress_func, &ctx, INT32_MAX, INT32_MAX, Z_TIMEOUT_TICKS(4)),
		       ZTRESS_THREAD(stress_func, &ctx, INT32_MAX, INT32_MAX, Z_TIMEOUT_TICKS(4)),
		       ZTRESS_THREAD(stress_func, &ctx, INT32_MAX, INT32_MAX, Z_TIMEOUT_TICKS(4)));

	free_all(&ctx);
	TC_PRINT("Executed %d allocation operation. Failed to allocate %d times.\n",
			(uint32_t)ctx.cnt, (uint32_t)ctx.fails);

	if (IS_ENABLED(CONFIG_DMM_STATS)) {
		uint32_t curr_use2;

		rv = dmm_stats_get(ctx.mem_reg, NULL, &curr_use2, NULL);
		zassert_ok(rv);
		zassert_equal(curr_use, curr_use2, "Unexpected usage got:%d exp:%d",
				curr_use2, curr_use);
	}
}

ZTEST_F(dmm, test_stress_allocator_nocache)
{
	stress_allocator(fixture->regions[DMM_TEST_REGION_NOCACHE].mem_reg, false);
}

ZTEST_F(dmm, test_stress_allocator_cache)
{
	stress_allocator(fixture->regions[DMM_TEST_REGION_CACHE].mem_reg, true);
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
