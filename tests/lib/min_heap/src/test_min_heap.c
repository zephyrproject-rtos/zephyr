/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/min_heap.h>
#include <zephyr/ztest_assert.h>

struct data {
	struct min_heap_handle handle;
	int key;
	int value;
};

static int compare_gt(const struct min_heap_handle *a, const struct min_heap_handle *b)
{
	const struct data *da = CONTAINER_OF(a, struct data, handle);
	const struct data *db = CONTAINER_OF(b, struct data, handle);

	if (da->key < db->key) {
		return 1;
	} else if (da->key > db->key) {
		return -1;
	}

	return 0;
}

static int compare_ls(const struct min_heap_handle *a, const struct min_heap_handle *b)
{
	const struct data *da = CONTAINER_OF(a, struct data, handle);
	const struct data *db = CONTAINER_OF(b, struct data, handle);

	return da->key - db->key;
}

#define HEAP_CAPACITY 8
#define LOWEST_KEY_LS 2
#define LOWEST_KEY_GT 30

MIN_HEAP_DEFINE_STATIC(my_heap, HEAP_CAPACITY, compare_ls);
MIN_HEAP_DEFINE_STATIC(my_heap_gt, HEAP_CAPACITY, compare_gt);

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

/* Pop all elements and verify they come out in descending key order */
static void validate_heap_order_gt(struct min_heap *h)
{
	int prev_key = INT_MAX;
	struct min_heap_handle *handle;

	while ((handle = min_heap_pop(h)) != NULL) {
		struct data *d = CONTAINER_OF(handle, struct data, handle);

		zassert_true(d->key <= prev_key, "Heap order violated: %d < %d", d->key, prev_key);
		prev_key = d->key;
		/* Reset handle so elements can be re-inserted in later tests */
		d->handle.idx = 0U;
	}
}

/* Pop all elements and verify they come out in ascending key order */
static void validate_heap_order_ls(struct min_heap *h)
{
	int prev_key = INT_MIN;
	struct min_heap_handle *handle;

	while ((handle = min_heap_pop(h)) != NULL) {
		struct data *d = CONTAINER_OF(handle, struct data, handle);

		zassert_true(d->key >= prev_key, "Heap order violated: %d < %d", d->key, prev_key);
		prev_key = d->key;
		/* Reset handle so elements can be re-inserted in later tests */
		d->handle.idx = 0U;
	}
}

ZTEST(min_heap_api, test_insert)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		elements[i].handle.idx = 0U;
		ret = min_heap_push(&my_heap, &elements[i].handle);
		zassert_ok(ret, "min_heap_push failed");
	}
	validate_heap_order_ls(&my_heap);

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		elements[i].handle.idx = 0U;
		ret = min_heap_push(&my_heap_gt, &elements[i].handle);
		zassert_ok(ret, "min_heap_push failed");
	}

	/* Try to push one more element to the now full heap */
	struct data extra = {.key = 99, .value = 0};

	ret = min_heap_push(&my_heap_gt, &extra.handle);
	zassert_equal(ret, -1, "push on full heap should return -1");

	validate_heap_order_gt(&my_heap_gt);
}

ZTEST(min_heap_api, test_peek_and_pop)
{
	int ret;

	MIN_HEAP_DEFINE_STATIC(runtime_heap, HEAP_CAPACITY, compare_ls);

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		elements[i].handle.idx = 0U;
		ret = min_heap_push(&runtime_heap, &elements[i].handle);
		zassert_ok(ret, "min_heap_push failed");
	}

	const struct min_heap_handle *peek_h = min_heap_peek(&runtime_heap);

	zassert_not_null(peek_h, "peek on non-empty heap returned NULL");

	const struct data *peeked = CONTAINER_OF(peek_h, struct data, handle);
	int peek_key = peeked->key;

	struct min_heap_handle *pop_h = min_heap_pop(&runtime_heap);
	struct data *popped = CONTAINER_OF(pop_h, struct data, handle);

	zassert_equal(peek_key, popped->key, "peek/pop mismatch");
	zassert_equal(popped->key, LOWEST_KEY_LS, "Expected minimum key %d, got %d", LOWEST_KEY_LS,
		      popped->key);
	popped->handle.idx = 0U;

	validate_heap_order_ls(&runtime_heap);
	zassert_is_null(min_heap_peek(&runtime_heap), "peek on empty heap should return NULL");
}

ZTEST(min_heap_api, test_remove)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		elements[i].handle.idx = 0U;
		ret = min_heap_push(&my_heap, &elements[i].handle);
		zassert_ok(ret, "min_heap_push failed");
	}

	/* Remove element with key = 5 directly by handle */
	struct data *target = &elements[1];

	zassert_true(target->handle.idx > 0U, "Element not in heap");
	zassert_true(min_heap_remove(&my_heap, &target->handle), "Failed to remove valid handle");
	zassert_equal(target->handle.idx, 0U, "idx not cleared after remove");
	zassert_false(min_heap_is_empty(&my_heap), "heap should not be empty");

	struct data not_inserted = {.key = 77};

	zassert_false(min_heap_remove(&my_heap, &not_inserted.handle),
		      "Remove operation of uninserted handle should return false");

	validate_heap_order_ls(&my_heap);
	zassert_true(min_heap_is_empty(&my_heap), "heap should be empty");
}

ZTEST(min_heap_api, test_foreach)
{
	struct min_heap_handle *h;
	struct data *d;
	int ret;

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {

		elements[i].handle.idx = 0U;
		ret = min_heap_push(&my_heap, &elements[i].handle);
		zassert_ok(ret, "min_heap_push failed");
	}

	bool visited[ARRAY_SIZE(elements)] = {};
	int count = 0;

	MIN_HEAP_FOREACH(&my_heap, h) {
		d = CONTAINER_OF(h, struct data, handle);
		int idx = (int)(d - elements);

		zassert_true(idx >= 0 && idx < (int)ARRAY_SIZE(elements),
			     "Handle outside elements array");
		zassert_false(visited[idx], "handle %d visited twice", idx);
		visited[idx] = true;
		count++;
	}
	zassert_equal(count, (int)ARRAY_SIZE(elements), "FOREACH visit count mismatch");
	for (int i = 0; i < (int)ARRAY_SIZE(elements); i++) {

		zassert_true(visited[i], "element[%d] not visited by FOREACH", i);
	}

	bool visited2[ARRAY_SIZE(elements)] = {};
	int count2 = 0;

	MIN_HEAP_FOREACH_CONTAINER(&my_heap, d, handle)
	{
		int idx = (int)(d - elements);

		zassert_true(idx >= 0 && idx < (int)ARRAY_SIZE(elements),
			     "container outside elements array");
		zassert_false(visited2[idx], "Container %d visited twice", idx);
		visited2[idx] = true;
		count2++;
	}
	zassert_equal(count2, (int)ARRAY_SIZE(elements), "FOREACH_CONTAINER visit count mismatch");
	for (int i = 0; i < (int)ARRAY_SIZE(elements); i++) {
		zassert_true(visited2[i], "element[%d] not visited by FOREACH_CONTAINER", i);
	}

	/* Drain heap, reset all idx*/
	validate_heap_order_ls(&my_heap);

	int empty_count = 0;

	MIN_HEAP_FOREACH(&my_heap, h) {
		empty_count++;
	}
	zassert_equal(empty_count, 0, "FOREACH body executed on empty heap");

	MIN_HEAP_FOREACH_CONTAINER(&my_heap, d, handle)
	{
		empty_count++;
	}
	zassert_equal(empty_count, 0, "FOREACH_CONTAINER body executes on empty heap");
}

ZTEST_SUITE(min_heap_api, NULL, NULL, NULL, NULL, NULL);
