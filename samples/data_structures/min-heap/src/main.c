/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/min_heap.h>

struct data {
	int key;
	int value;
};

static int compare(const void *a, const void *b)
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

MIN_HEAP_DEFINE_STATIC(my_heap, HEAP_CAPACITY, sizeof(struct data),
		       __alignof__(struct data), compare);

int main(void)
{
	void *elem;
	int target_key = 5;
	size_t index;
	struct data *top, *found, removed;

	printk("Min-heap sample using static storage\n");

	struct data elements[] = {
		{ .key = 10, .value = 100 },
		{ .key = 5,  .value = 200 },
		{ .key = 30, .value = 300 },
		{ .key = 2,  .value = 400 },
	};

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {
		if (min_heap_push(&my_heap, &elements[i]) != 0) {
			printk("Insert failed at index %d\n", i);
		}
	}

	printk("Heap elements by order of priority:\n");
	MIN_HEAP_FOREACH(&my_heap, elem) {
		struct data *d = elem;

		printk("key=%d value=%d\n", d->key, d->value);
	}

	printk("Top of heap: ");
	top = min_heap_peek(&my_heap);
	if (top) {
		printk("key=%d value=%d\n", top->key, top->value);
	}

	found = min_heap_find(&my_heap, match_key, &target_key, &index);
	if (found) {
		printk("Found element with key %d at index %zu,"
			 "removing it...\n", target_key, index);
		min_heap_remove(&my_heap, index, &removed);
	}

	printk("Heap after removal:\n");
	MIN_HEAP_FOREACH(&my_heap, elem) {
		struct data *d = elem;

		printk("key=%d value=%d\n", d->key, d->value);
	}

	return 0;
}
