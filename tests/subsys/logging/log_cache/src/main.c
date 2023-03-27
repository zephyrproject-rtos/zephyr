/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log list
 *
 */

#include <../subsys/logging/log_cache.h>

#include <zephyr/tc_util.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdbool.h>


#define ENTRY_SIZE(data_len) \
	ROUND_UP(sizeof(struct log_cache_entry) + data_len, sizeof(uintptr_t))
#define TEST_ENTRY_LEN 8

struct test_id {
	uint8_t x;
	uint16_t y;
};

union test_ids {
	struct test_id id;
	uintptr_t raw;
};

static bool cmp(uintptr_t id0, uintptr_t id1)
{
	union test_ids t0 = { .raw = id0 };
	union test_ids t1 = { .raw = id1 };

	return (t0.id.x == t1.id.x) && (t0.id.y == t1.id.y);
}

static void buf_fill(uint8_t *data, uint8_t x)
{
	for (int i = 0; i < TEST_ENTRY_LEN; i++) {
		data[i] = x;
	}
}

static bool buf_check(uint8_t *data, uint8_t x)
{
	for (int i = 0; i < TEST_ENTRY_LEN; i++) {
		if (data[i] !=  x) {
			return false;
		}
	}

	return true;
}

static void cache_get(struct log_cache *cache, uintptr_t id,
		      uint8_t **buf, bool exp_hit, uint32_t line)
{
	uint32_t hit = log_cache_get_hit(cache);
	uint32_t miss = log_cache_get_miss(cache);
	bool res;

	res = log_cache_get(cache, id, buf);
	zassert_equal(res, exp_hit, "line %u\n", line);
	if (exp_hit) {
		zassert_equal(hit + 1, log_cache_get_hit(cache), "line %u\n", line);
		zassert_equal(miss, log_cache_get_miss(cache), "line %u\n", line);
	} else {
		zassert_equal(hit, log_cache_get_hit(cache), "line %u\n", line);
		zassert_equal(miss + 1, log_cache_get_miss(cache), "line %u\n", line);
	}
}

ZTEST(test_log_cache, test_log_cache_basic)
{
	/* Space for 3 entries */
	static uint8_t data[3 * ENTRY_SIZE(TEST_ENTRY_LEN)];
	static const struct log_cache_config config = {
		.buf = data,
		.buf_len = sizeof(data),
		.item_size = TEST_ENTRY_LEN,
		.cmp = cmp
	};
	int err;
	uint8_t *buf;
	struct log_cache cache;
	union test_ids id0 = {
		.id = { .x = 100, .y = 1245 }
	};
	union test_ids id1 = {
		.id = { .x = 101, .y = 1245 }
	};
	union test_ids id2 = {
		.id = { .x = 102, .y = 1245 }
	};
	union test_ids id3 = {
		.id = { .x = 103, .y = 1245 }
	};

	err = log_cache_init(&cache, &config);
	zassert_equal(err, 0, NULL);

	/* Try to find id0, cache is empty */
	cache_get(&cache, id0.raw, &buf, false, __LINE__);

	buf_fill(buf, 1);
	/* Put id0 entry */
	log_cache_put(&cache, buf);

	/* Try to find id0 with success. */
	cache_get(&cache, id0.raw, &buf, true, __LINE__);
	zassert_true(buf_check(buf, 1), "Buffer check failed");

	/* Miss id1 in cache then put it */
	cache_get(&cache, id1.raw, &buf, false, __LINE__);
	buf_fill(buf, 2);
	log_cache_put(&cache, buf);

	/* Miss id2 in cache then put it */
	cache_get(&cache, id2.raw, &buf, false, __LINE__);
	buf_fill(buf, 3);
	log_cache_put(&cache, buf);

	/* Miss id3 in cache then put it. At that point id0 should still be in
	 * the cache but we now filled whole cache and oldest entry will be
	 * evicted.
	 */
	cache_get(&cache, id0.raw, &buf, true, __LINE__);
	cache_get(&cache, id1.raw, &buf, true, __LINE__);
	cache_get(&cache, id2.raw, &buf, true, __LINE__);
	cache_get(&cache, id3.raw, &buf, false, __LINE__);
	buf_fill(buf, 4);
	log_cache_put(&cache, buf);

	/* id0 is evicted since it is the oldest one. */
	cache_get(&cache, id0.raw, &buf, false, __LINE__);
	zassert_true(buf_check(buf, 2), "Buffer check failed");
	log_cache_put(&cache, buf);

	/* And id0 is now in cache */
	cache_get(&cache, id0.raw, &buf, true, __LINE__);

	/* buf id1 got evicted */
	cache_get(&cache, id1.raw, &buf, false, __LINE__);
	zassert_true(buf_check(buf, 3), "Buffer check failed");
	log_cache_put(&cache, buf);
}

ZTEST_SUITE(test_log_cache, NULL, NULL, NULL, NULL, NULL);
