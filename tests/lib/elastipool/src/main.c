/*
 * Copyright (c) 2025, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/sys/hash_map.h>
#include <zephyr/toolchain.h>

#define ULONG_BITS (sizeof(unsigned long) * BITS_PER_BYTE)

struct elem {
	uint64_t blob[2];
};

SYS_ELASTIPOOL_DEFINE(static_pool, sizeof(struct elem), __alignof(struct elem), 1, 1);
SYS_ELASTIPOOL_DEFINE_STATIC(dynamic_pool, sizeof(struct elem), __alignof(struct elem), 0, 1);

__maybe_unused __noinit __aligned(WB_UP(__alignof(struct elem))) static uint8_t
	_elastipool_slab_storage_elastic_pool[ROUND_UP(sizeof(struct elem),
						       __alignof(struct elem)) *
					      (size_t)CONFIG_TEST_ELASTIC_POOL_MAX_ELEMS];
SYS_ELASTIPOOL_DEFINE_ADVANCED(
	elastic_pool, sizeof(struct elem), __alignof(struct elem), 1,
	(size_t)CONFIG_TEST_ELASTIC_POOL_MAX_ELEMS,
	SYS_ELASTIPOOL_THREAD_HEAP_ALLOC_FN(1, (size_t)CONFIG_TEST_ELASTIC_POOL_MAX_ELEMS),
	_elastipool_slab_storage_elastic_pool, static);

ZTEST(elastipool, test_sys_elastipool_init)
{
	struct sys_elastipool pool;
	struct sys_elastipool_api api;
	struct sys_elastipool_config config;
	struct sys_elastipool_data data;

	zexpect_ok(sys_elastipool_init(
		&pool, &api, &config, &data, sizeof(struct elem), __alignof(struct elem), 1,
		CONFIG_TEST_ELASTIC_POOL_MAX_ELEMS, elastic_pool.config->storage,
		elastic_pool.config->bmp, elastic_pool.config->map, elastic_pool.api->heap_alloc));

	zexpect_mem_equal(&api, elastic_pool.api, sizeof(api));
	zexpect_mem_equal(&config, elastic_pool.config, sizeof(config));
}

ZTEST(elastipool, test_sys_elastipool_alloc)
{
	void *elem, *elem2;

	zexpect_ok(sys_elastipool_alloc(&static_pool, &elem));
	zexpect_equal(-ENOMEM, sys_elastipool_alloc(&static_pool, &elem));

	zexpect_ok(sys_elastipool_alloc(&dynamic_pool, &elem));
	zexpect_equal(-ENOMEM, sys_elastipool_alloc(&dynamic_pool, &elem));

	zexpect_ok(sys_elastipool_alloc(&elastic_pool, &elem));
	zexpect_ok(sys_elastipool_alloc(&elastic_pool, &elem2));
}

ZTEST(elastipool, test_sys_elastipool_free)
{
	void *elem, *elem2;

	zexpect_ok(sys_elastipool_alloc(&static_pool, &elem));
	zexpect_ok(sys_elastipool_free(&static_pool, elem));
	zexpect_equal(-EINVAL, sys_elastipool_free(&static_pool, (void *)0x4242));

	zexpect_ok(sys_elastipool_alloc(&dynamic_pool, &elem));
	zexpect_ok(sys_elastipool_free(&dynamic_pool, elem));
	zexpect_equal(-EINVAL, sys_elastipool_free(&dynamic_pool, (void *)0x4242));

	zexpect_ok(sys_elastipool_alloc(&elastic_pool, &elem));
	zexpect_ok(sys_elastipool_alloc(&elastic_pool, &elem2));
	zexpect_ok(sys_elastipool_free(&elastic_pool, elem));
	zexpect_ok(sys_elastipool_free(&elastic_pool, elem2));
	zexpect_equal(-EINVAL, sys_elastipool_free(&elastic_pool, (void *)0x4242));
}

ZTEST(elastipool, test_sys_elastipool_clear)
{
	void *elem;
	const struct sys_elastipool *pools[] = {
		&static_pool,
		&dynamic_pool,
	};

	/* test clear for the fixed-size pools (limit is based on available memory) */
	ARRAY_FOR_EACH(pools, i) {
		const struct sys_elastipool *const pool = pools[i];

		/* allocate all blocks */
		for (size_t i = 0; i < pool->config->max_obj; i++) {
			zexpect_ok(sys_elastipool_alloc(pool, &elem));
		}
		/* check that we have used all of the available blocks */
		zexpect_equal(pool->data->pool_size, pool->config->max_obj);
		/* clear all allocations */
		sys_elastipool_clear(pool);
		/* check that all of the available blocks are free */
		zexpect_equal(pool->data->pool_size, 0);
		/* re-allocate all blocks */
		for (size_t i = 0; i < pool->config->max_obj; i++) {
			zexpect_ok(sys_elastipool_alloc(pool, &elem));
		}
	}

	/* test clear for the elastic allocation strategy (limit is based on available memory) */

	/* allocate as many blocks as possible */
	size_t count = 0;

	for (size_t i = 0; i < elastic_pool.config->max_obj; i++, ++count) {
		if (sys_elastipool_alloc(&elastic_pool, &elem) < 0) {
			zassert_true(count >= 2);
			break;
		}
	}

	TC_PRINT("Allocated %zu objects of size %zu\n", count, sizeof(struct elem));

	/* clear all allocations */
	sys_elastipool_clear(&elastic_pool);
	/* check that all of the available blocks are free */
	zexpect_equal(elastic_pool.data->pool_size, 0);
	/* re-allocate as many blocks as possible */
	for (size_t i = 0; i < elastic_pool.config->max_obj; i++) {
		if (sys_elastipool_alloc(&elastic_pool, &elem) < 0) {
			zassert_true(i == count, "failed to re-allocate object %zu", i);
			break;
		}
	}

	TC_PRINT("Re-allocated %zu objects of size %zu\n", count, sizeof(struct elem));
}

/* min large enough that the bitmap spans multiple words and slots >= 64 exist */
#define HIGHBIT_POOL_MIN 100
SYS_ELASTIPOOL_DEFINE_STATIC(highbit_pool, sizeof(uint64_t), __alignof(uint64_t),
			     HIGHBIT_POOL_MIN, HIGHBIT_POOL_MIN);

/* min is an exact multiple of the bitmap word size (in bits) on both ILP32 and LP64 */
#define FULLWORD_POOL_MIN 128
SYS_ELASTIPOOL_DEFINE_STATIC(fullword_pool, sizeof(uint64_t), __alignof(uint64_t),
			     FULLWORD_POOL_MIN, FULLWORD_POOL_MIN);

/* storage must be sized by object count (min), not by object size */
BUILD_ASSERT(sizeof(_elastipool_slab_storage_highbit_pool) ==
		     ROUND_UP(sizeof(uint64_t), __alignof(uint64_t)) * HIGHBIT_POOL_MIN,
	     "SYS_ELASTIPOOL_DEFINE storage is not sized by min");

ZTEST(elastipool, test_storage_sized_by_min)
{
	static void *ptrs[HIGHBIT_POOL_MIN];
	const uint8_t *storage = highbit_pool.config->storage;
	const size_t stride = ROUND_UP(sizeof(uint64_t), __alignof(uint64_t));

	for (size_t i = 0; i < HIGHBIT_POOL_MIN; ++i) {
		zassert_ok(sys_elastipool_alloc(&highbit_pool, &ptrs[i]));
		zassert_true(((uint8_t *)ptrs[i] >= storage) &&
				     ((uint8_t *)ptrs[i] < storage + stride * HIGHBIT_POOL_MIN),
			     "slot %zu outside pool storage", i);
		*(uint64_t *)ptrs[i] = 0xA5A50000ULL + i;
	}

	/* no two slots may overlap: every pattern must be intact after all writes */
	for (size_t i = 0; i < HIGHBIT_POOL_MIN; ++i) {
		zexpect_equal(*(uint64_t *)ptrs[i], 0xA5A50000ULL + i,
			      "slot %zu corrupted by a neighbouring slot", i);
	}
}

ZTEST(elastipool, test_free_high_bit)
{
	static void *ptrs[HIGHBIT_POOL_MIN];
	void *elem;

	for (size_t i = 0; i < HIGHBIT_POOL_MIN; ++i) {
		zassert_ok(sys_elastipool_alloc(&highbit_pool, &ptrs[i]));
	}

	/* free a slot whose bitmap bit lives beyond word 0, then re-allocate it */
	zassert_ok(sys_elastipool_free(&highbit_pool, ptrs[70]));
	zassert_ok(sys_elastipool_alloc(&highbit_pool, &elem));
	zexpect_equal_ptr(elem, ptrs[70], "freed high-bit slot was not the one re-allocated");

	/* full free/alloc round-trip must succeed for every slot */
	for (size_t i = 0; i < HIGHBIT_POOL_MIN; ++i) {
		zassert_ok(sys_elastipool_free(&highbit_pool, ptrs[i]), "free of slot %zu failed",
			   i);
	}
	zexpect_equal(highbit_pool.data->pool_size, 0);
	for (size_t i = 0; i < HIGHBIT_POOL_MIN; ++i) {
		zassert_ok(sys_elastipool_alloc(&highbit_pool, &elem));
	}
}

ZTEST(elastipool, test_alloc_full_word_boundary)
{
	void *elem;
	static void *ptrs[FULLWORD_POOL_MIN];

	/* every slot must be allocatable when min is an exact bitmap-word multiple */
	for (size_t i = 0; i < FULLWORD_POOL_MIN; ++i) {
		zassert_ok(sys_elastipool_alloc(&fullword_pool, &ptrs[i]),
			   "allocation %zu of %d failed", i, FULLWORD_POOL_MIN);
	}
	zexpect_equal(-ENOMEM, sys_elastipool_alloc(&fullword_pool, &elem));

	zassert_ok(sys_elastipool_free(&fullword_pool, ptrs[FULLWORD_POOL_MIN - 1]));
	zassert_ok(sys_elastipool_alloc(&fullword_pool, &elem));
	zexpect_equal_ptr(elem, ptrs[FULLWORD_POOL_MIN - 1]);
}

ZTEST(elastipool, test_clear_static_bounded)
{
	struct sys_elastipool pool;
	struct sys_elastipool_api api;
	struct sys_elastipool_config config;
	struct sys_elastipool_data data;
	static uint8_t storage[ROUND_UP(sizeof(struct elem), __alignof(struct elem)) * 5];
	static struct {
		unsigned long pre[2];
		unsigned long bmp[DIV_ROUND_UP(5, ULONG_BITS)];
		unsigned long post[2];
	} guarded;
	unsigned long canary;
	void *elem;

	memset(&guarded, 0xaa, sizeof(guarded));
	memset(&canary, 0xaa, sizeof(canary));
	memset(guarded.bmp, 0, sizeof(guarded.bmp));

	zassert_ok(sys_elastipool_init(&pool, &api, &config, &data, sizeof(struct elem),
				       __alignof(struct elem), 5, 5, storage, guarded.bmp, NULL,
				       NULL));

	for (size_t i = 0; i < 5; ++i) {
		zassert_ok(sys_elastipool_alloc(&pool, &elem));
	}

	/* clear must touch only the bitmap words, never the surrounding memory */
	sys_elastipool_clear(&pool);
	zexpect_equal(pool.data->pool_size, 0);
	for (size_t i = 0; i < ARRAY_SIZE(guarded.pre); ++i) {
		zexpect_equal(guarded.pre[i], canary, "clear underran the bitmap (word %zu)", i);
	}
	for (size_t i = 0; i < ARRAY_SIZE(guarded.post); ++i) {
		zexpect_equal(guarded.post[i], canary, "clear overran the bitmap (word %zu)", i);
	}
}

SYS_HASHMAP_DEFINE_STATIC(init_dyn_map);

ZTEST(elastipool, test_init_min_zero_dynamic)
{
	struct sys_elastipool pool;
	struct sys_elastipool_api api;
	struct sys_elastipool_config config;
	struct sys_elastipool_data data;
	void *elem;
	void *elem2;

	if (!IS_ENABLED(CONFIG_SYS_HASH_MAP)) {
		ztest_test_skip();
	}

	/* min_obj == 0 is the documented dynamic-only mode */
	zassert_ok(sys_elastipool_init(&pool, &api, &config, &data, sizeof(struct elem),
				       __alignof(struct elem), 0, 1, NULL, NULL, &init_dyn_map,
				       NULL));

	zassert_ok(sys_elastipool_alloc(&pool, &elem));
	zexpect_equal(-ENOMEM, sys_elastipool_alloc(&pool, &elem2));
	zassert_ok(sys_elastipool_free(&pool, elem));
	zexpect_equal(pool.data->pool_size, 0);
}

static void after(void *arg)
{
	sys_elastipool_clear(&static_pool);
	sys_elastipool_clear(&dynamic_pool);
	sys_elastipool_clear(&elastic_pool);
	sys_elastipool_clear(&highbit_pool);
	sys_elastipool_clear(&fullword_pool);
}

ZTEST_SUITE(elastipool, NULL, NULL, NULL, after, NULL);
