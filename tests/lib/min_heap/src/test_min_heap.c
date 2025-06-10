/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/min_heap.h>
#include <zephyr/ztest_assert.h>

struct data {
	int key;
	int value;
};

static int compare_gt(const void *a, const void *b)
{
	const struct data *da = a;
	const struct data *db = b;
	int ret;

	if (da->key < db->key) {
		ret = 1;
	} else if (da->key > db->key) {
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

static int compare_ls(const void *a, const void *b)
{
	const struct data *da = a;
	const struct data *db = b;

	return da->key - db->key;
}

static bool match_key(const void *a, const void *b)
{
	const struct data *da = a;
	const int *key = b;

	return da->key == *key;
}

#define HEAP_CAPACITY 8
#define LOWEST_PRIORITY_LS 2
#define LOWEST_PRIORITY_GT 30

MIN_HEAP_DEFINE_STATIC(my_heap, HEAP_CAPACITY, sizeof(struct data),
		       __alignof__(struct data), compare_ls);
MIN_HEAP_DEFINE_STATIC(my_heap_gt, HEAP_CAPACITY, sizeof(struct data),
		       __alignof__(struct data), compare_gt);

struct data elements[] = {
	{ .key = 10, .value = 100 },
	{ .key = 5,  .value = 200 },
	{ .key = 30, .value = 300 },
	{ .key = 2,  .value = 400 },
	{ .key = 3,  .value = 400 },
	{ .key = 4,  .value = 400 },
	{ .key = 6,  .value = 400 },
	{ .key = 22,  .value = 400 },
};

static void validate_heap_order_gt(struct min_heap *h)
{
	struct data result[HEAP_CAPACITY];
	struct data temp;
	int idx = 0;
	bool ret;

	while (h->size > 0) {
		ret = min_heap_pop(h, &temp);
		zassert_true(ret, "pop failure");
		memcpy(&result[idx++], &temp, sizeof(struct data));
	}

	for (int i = 1; i < idx; i++) {

		zassert_true(result[i].key <= result[i - 1].key,
			"Heap order violated at index %d: %d < %d",
			i, result[i].key, result[i - 1].key);
	}
}

static void validate_heap_order_ls(struct min_heap *h)
{
	struct data result[HEAP_CAPACITY];
	struct data temp;
	int idx = 0;
	bool ret;

	while (h->size > 0) {
		ret = min_heap_pop(h, &temp);
		zassert_true(ret, "pop failure");
		memcpy(&result[idx++], &temp, sizeof(struct data));
	}

	for (int i = 1; i < idx; i++) {

		zassert_true(result[i].key >= result[i - 1].key,
			"Heap order violated at index %d: %d < %d",
			i, result[i].key, result[i - 1].key);
	}
}

ZTEST(min_heap_api, test_insert)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		ret = min_heap_push(&my_heap, &elements[i]);
		zassert_ok(ret, "min_heap_push failed");
	}
	validate_heap_order_ls(&my_heap);

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		ret = min_heap_push(&my_heap_gt, &elements[i]);
		zassert_ok(ret, "min_heap_push failed");
	}

	/* Try to push one more element to the now full heap */
	ret = min_heap_push(&my_heap_gt, &elements[0]);
	zassert_equal(ret, -ENOMEM, "push on full heap should return -ENOMEM");

	validate_heap_order_gt(&my_heap_gt);
}

ZTEST(min_heap_api, test_peek_and_pop)
{
	int ret;
	uint8_t storage[HEAP_CAPACITY * sizeof(struct data)];

	MIN_HEAP_DEFINE(runtime_heap, storage, HEAP_CAPACITY,
			sizeof(struct data), compare_ls);

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		ret = min_heap_push(&runtime_heap, &elements[i]);
		zassert_ok(ret, "min_heap_push failed");
	}

	struct data *peek, pop;
	int peek_key;

	peek = min_heap_peek(&runtime_heap);
	peek_key = peek->key;
	min_heap_pop(&runtime_heap, &pop);

	zassert_equal(peek_key, pop.key, "Peek/pop error");
	zassert_equal(pop.key, LOWEST_PRIORITY_LS, "heap error %d", pop.key);
	validate_heap_order_ls(&runtime_heap);
	zassert_is_null(min_heap_peek(&runtime_heap), "peek on empty heap should return NULL");
}

ZTEST(min_heap_api, test_find_and_remove)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		ret = min_heap_push(&my_heap, &elements[i]);
		zassert_ok(ret, "min_heap_push failed");
	}

	int target_key = 5;
	int wrong_key = 100;
	size_t index;
	struct data removed, *found, *found_ignore_index, *not_found;

	found = min_heap_find(&my_heap, match_key, &target_key, &index);
	found_ignore_index = min_heap_find(&my_heap, match_key, &target_key, NULL);
	not_found = min_heap_find(&my_heap, match_key, &wrong_key, &index);

	zassert_not_null(found, "min_heap_find failure");
	zassert_not_null(found_ignore_index, "min_heap_find failure");
	zassert_is_null(not_found, "min_heap_find failure");
	zassert_equal(found->key, target_key, "Found wrong element");
	min_heap_remove(&my_heap, index, &removed);
	zassert_false(min_heap_is_empty(&my_heap), "Heap should not be empty");
	min_heap_remove(&my_heap, HEAP_CAPACITY, &removed);
	zassert_false(min_heap_remove(&my_heap, HEAP_CAPACITY, &removed),
		      "remove with invalid index should return false");
	validate_heap_order_ls(&my_heap);
	zassert_true(min_heap_is_empty(&my_heap), "Empty check fail");
}

ZTEST_SUITE(min_heap_api, NULL, NULL, NULL, NULL, NULL);
