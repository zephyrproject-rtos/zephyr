/**
 * @file test_min_heap.c
 * @brief Min-heap tests using ztest framework
 *
 * This file contains unit tests for verifying the functionality of
 * the min-heap implementation in Zephyr. It uses `ztest` and demonstrates
 * heap initialization, insertion, ordering, popping and destroying.
 */

/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/min_heap.h>

/** @brief Maximum number of items in the test heap */
#define MAX_ITEMS 10

/** @brief Test item structure with heap node and identifier string*/
struct test_item {
	/** Min heap node containing the key */
	struct min_heap_node node;
	/** Name for the test item */
	const char *name;
};

/** @brief Buffer to hold heap node pointers */
static struct min_heap_node *heap_buf[MAX_ITEMS];
/** @brief Min heap instance for testing */
static struct min_heap heap;

/** @brief Predefined set of test items with associated priorities */
static struct test_item items[MAX_ITEMS] = {
	{ .node.key = 5, .name = "Task5" },
	{ .node.key = 1, .name = "Task1" },
	{ .node.key = 3, .name = "Task3" },
	{ .node.key = 7, .name = "Task7" },
	{ .node.key = 2, .name = "Task2" },
	{ .node.key = 8, .name = "Task8" },
	{ .node.key = 4, .name = "Task4" },
	{ .node.key = 0, .name = "Task0" },
	{ .node.key = 6, .name = "Task6" },
	{ .node.key = 9, .name = "Task9" }
};

/**
 * @brief Comparator function for min-heap nodes
 *
 * @param a First heap node
 * @param b Second heap node
 *
 * @return Negative if a < b, 0 if equal, positive if a > b
 */
static int test_cmp(const struct min_heap_node *a,
		    const struct min_heap_node *b)
{
	return a->key - b->key;
}

/**
 * @brief Test ordering of items inserted into min-heap
 *
 * Inserts predefined items into the heap and asserts that
 * popping the heap returns them in ascending order of key.
 */
ZTEST(min_heap_suite, test_min_heap_ordering)
{
	int ret;

	ret = min_heap_init(&heap, heap_buf, MAX_ITEMS, test_cmp);
	zassert_equal(ret, 0,
		      "min_heap initialization failed %d", ret);

	for (int i = 0; i < MAX_ITEMS; i++) {

		ret = min_heap_insert(&heap, &items[i].node);
		zassert_equal(ret, 0,
			      "min_heap insert failed %d", ret);
	}

	int expected = 0;

	while (heap.size > 0) {
		struct min_heap_node *node = min_heap_pop(&heap);

		zassert_equal(node->key, expected,
			      "Expected %d but got %d", expected, node->key);
		expected++;
	}

	ret = min_heap_destroy(&heap);
	zassert_equal(ret, 0,
		      "min_heap destroy failed %d", ret);
}

/**
 * @brief Test min_heap_remove_by_idx() functionality
 *
 * This test inserts all test items into the heap and then removes specific
 * nodes by using `min_heap_remove_by_idx()`. It verifies the correct ordering
 * of the remaining elements when popped from the heap.
 */
ZTEST(min_heap_suite, test_min_heap_remove)
{
	int ret;

	ret = min_heap_init(&heap, heap_buf, MAX_ITEMS, test_cmp);
	zassert_equal(ret, 0,
		      "min_heap initialization failed %d", ret);

	for (int i = 0; i < MAX_ITEMS; i++) {

		min_heap_insert(&heap, &items[i].node);
		zassert_equal(ret, 0,
			      "min_heap insert failed %d", ret);
	}

	/* Remove Task3 (index 2, key 3) and Task6 (index 8, key 6) */
	min_heap_remove_by_idx(&heap, &items[2].node);
	min_heap_remove_by_idx(&heap, &items[8].node);

	int expected_keys[] = { 0, 1, 2, 4, 5, 7, 8, 9 };
	int count = ARRAY_SIZE(expected_keys);

	for (int i = 0; i < count; i++) {

		struct min_heap_node *node = min_heap_pop(&heap);

		zassert_not_null(node, "Heap returned NULL unexpectedly");
		zassert_equal(node->key, expected_keys[i],
			      "Expected key %d but got %d",
			      expected_keys[i], node->key);
	}

	zassert_equal(1, min_heap_is_empty(&heap),
		      "Heap not empty after removals");

	ret = min_heap_destroy(&heap);
	zassert_equal(ret, 0,
		      "min_heap destroy failed %d", ret);
}

/**
 * @brief Test MIN_HEAP_INIT macro initialization
 *
 * This test verifies that a min heap initialized using the `MIN_HEAP_INIT`
 * macro behaves correctly. It checks whether elements inserted are returned in
 * order of increasing key values and that the heap is empty at the end
 */
ZTEST(min_heap_suite, test_min_heap_macro_init)
{
	int expected = 0, ret;
	struct min_heap_node *static_buf[MAX_ITEMS];
	struct min_heap heap_macro = MIN_HEAP_INIT(static_buf,
				      MAX_ITEMS, test_cmp);

	for (int i = 0; i < MAX_ITEMS; i++) {

		ret = min_heap_insert(&heap_macro, &items[i].node);
		zassert_equal(ret, 0,
			      "min_heap insert failed %d", ret);

	}
	while (heap_macro.size > 0) {
		struct min_heap_node *node = min_heap_pop(&heap_macro);

		zassert_equal(node->key, expected, "Expected %d but got %d",
			      expected, node->key);
		expected++;
	}

	ret = min_heap_destroy(&heap_macro);
	zassert_equal(ret, 0,
		      "min_heap destroy failed %d", ret);
}

ZTEST_SUITE(min_heap_suite, NULL, NULL, NULL, NULL, NULL);
