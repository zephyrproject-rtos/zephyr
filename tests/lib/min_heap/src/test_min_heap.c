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
#include <zephyr/sys/min_heap.h>

/** @brief Maximum number of items in the test heap */
#define MAX_ITEMS 10

/** @brief Test item structure with heap node and identifier string*/
struct test_item {
	/** Min heap node containing the key */
	struct min_heap_node node;
	/** Name for the test item */
	const char *name;
	/** Key for comparison */
	int key;
};

/** @brief Buffer to hold heap node pointers */
static struct min_heap_node *heap_buf[MAX_ITEMS];
/** @brief Min heap instance for testing */
static struct min_heap heap;

/** @brief Predefined set of test items with associated priorities */
static struct test_item items[MAX_ITEMS] = {
	{ .name = "Task5", .key = 5 },
	{ .name = "Task1", .key = 1 },
	{ .name = "Task3", .key = 3 },
	{ .name = "Task7", .key = 7 },
	{ .name = "Task2", .key = 2 },
	{ .name = "Task8", .key = 8 },
	{ .name = "Task4", .key = 4 },
	{ .name = "Task0", .key = 0 },
	{ .name = "Task6", .key = 6 },
	{ .name = "Task9", .key = 9 }
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
	const struct test_item *item_a = CONTAINER_OF(a,
					 struct test_item, node);
	const struct test_item *item_b = CONTAINER_OF(b,
					 struct test_item, node);

	return item_a->key - item_b->key;
}

MIN_HEAP_DEFINE(test_heap, MAX_ITEMS, test_cmp);

/**
 * @brief Test ordering of items inserted into min-heap
 *
 * Inserts predefined items into the heap and asserts that
 * popping the heap returns them in ascending order of key.
 */
ZTEST(min_heap_suite, test_min_heap_ordering)
{
	int ret;

	min_heap_init(&heap, heap_buf, MAX_ITEMS, test_cmp);

	for (int i = 0; i < MAX_ITEMS; i++) {

		ret = min_heap_insert(&heap, &items[i].node);
		zassert_equal(ret, 0,
			      "min_heap insert failed %d", ret);
	}

	int expected = 0;

	while (heap.size > 0) {
		struct min_heap_node *node;
		struct test_item *item;

		node = min_heap_pop(&heap);
		zassert_not_null(node, "Heap returned NULL unexpectedly");
		item = CONTAINER_OF(node, struct test_item, node);
		zassert_equal(item->key, expected,
			      "Expected %d but got %d", expected, item->key);
		expected++;
	}
}

/**
 * @brief Test min_heap_remove() functionality
 *
 * This test inserts all test items into the heap and then removes specific
 * nodes by using `min_heap_remove()`. It verifies the correct ordering
 * of the remaining elements when popped from the heap.
 */
ZTEST(min_heap_suite, test_min_heap_remove)
{
	int ret;
	struct min_heap_node *removed;
	struct test_item *item;

	for (int i = 0; i < MAX_ITEMS; i++) {

		ret = min_heap_insert(&test_heap, &items[i].node);
		zassert_equal(ret, 0,
			      "min_heap insert failed %d", ret);
	}

	/* Remove Task3 (index 2, key 3) and Task6 (index 8, key 6) */
	removed = min_heap_remove(&test_heap, 2);
	item = CONTAINER_OF(removed, struct test_item, node);
	zassert_equal(3, item->key, "Expected 3 and got %d", item->key);
	removed = min_heap_remove(&test_heap, 8);
	item = CONTAINER_OF(removed, struct test_item, node);
	zassert_equal(6, item->key, "Expected 6 and got %d", item->key);

	int expected_keys[] = { 0, 1, 2, 4, 5, 7, 8, 9 };
	int count = ARRAY_SIZE(expected_keys);

	for (int i = 0; i < count; i++) {

		struct min_heap_node *node;

		node = min_heap_pop(&test_heap);
		zassert_not_null(node, "Heap returned NULL unexpectedly");
		item = CONTAINER_OF(node, struct test_item, node);
		zassert_equal(item->key, expected_keys[i],
			      "Expected key %d but got %d",
			      expected_keys[i], item->key);
	}

	zassert_equal(1, min_heap_is_empty(&test_heap),
		      "Heap not empty after removals");

}

ZTEST_SUITE(min_heap_suite, NULL, NULL, NULL, NULL, NULL);
