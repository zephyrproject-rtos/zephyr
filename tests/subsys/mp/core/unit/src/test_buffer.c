/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_structure.h>

extern struct k_heap _system_heap;

struct mp_buffer_api_fixture {
	struct sys_memory_stats mem_before;
};

static void *buffer_suite_setup(void)
{
	static struct mp_buffer_api_fixture fixture;

	return &fixture;
}

static void buffer_before(void *f)
{
	struct mp_buffer_api_fixture *fix = f;

	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->mem_before);
}

static void buffer_after(void *f)
{
	struct mp_buffer_api_fixture *fix = f;
	struct sys_memory_stats mem_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &mem_after);
	zassert_equal(fix->mem_before.allocated_bytes, mem_after.allocated_bytes,
		      "Memory leak detected: before=%zu after=%zu", fix->mem_before.allocated_bytes,
		      mem_after.allocated_bytes);
}

ZTEST_SUITE(mp_buffer_api, NULL, buffer_suite_setup, buffer_before, buffer_after, NULL);

static struct mp_buffer_pool pool;

ZTEST(mp_buffer_api, test_sanity)
{
	struct mp_structure *config = mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_STRUCTURE_END);

	mp_buffer_pool_init(&pool);
	zassert_false(pool.started, "pool.started != false after init");

	zassert_true(mp_buffer_pool_configure(NULL, config) < 0,
		     "configure NULL pool did not fail");

	zassert_true(mp_buffer_pool_configure(&pool, config) < 0,
		     "configure(no callback) did not fail");

	mp_structure_destroy(config);

	zassert_true(mp_buffer_pool_start(NULL) < 0, "start NULL pool did not fail");

	pool.start = NULL;
	zassert_true(mp_buffer_pool_start(&pool) < 0, "start no callback did not fail");

	zassert_true(mp_buffer_pool_stop(NULL) < 0, "stop NULL pool did not fail");

	pool.stop = NULL;
	zassert_true(mp_buffer_pool_stop(&pool) < 0, "stop no callback did not fail");
}
