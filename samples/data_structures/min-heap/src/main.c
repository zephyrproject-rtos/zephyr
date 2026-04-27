/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/min_heap.h>

struct data {
	struct min_heap_handle handle;
	int key;
	int value;
};

static int compare(const struct min_heap_handle *a, const struct min_heap_handle *b)
{
	const struct data *da = CONTAINER_OF(a, struct data, handle);
	const struct data *db = CONTAINER_OF(b, struct data, handle);

	return da->key - db->key;
}

#define HEAP_CAPACITY 8

MIN_HEAP_DEFINE_STATIC(my_heap, HEAP_CAPACITY, compare);

int main(void)
{
	int target_key = 5;
	struct data *found = NULL;
	struct data *d;

	printk("Min-heap sample using static storage\n");

	struct data elements[] = {
		{ .key = 10, .value = 100 },
		{ .key = 5,  .value = 200 },
		{ .key = 30, .value = 300 },
		{ .key = 2,  .value = 400 },
	};

	for (int i = 0; i < ARRAY_SIZE(elements); i++) {
		if (min_heap_push(&my_heap, &elements[i].handle) != 0) {
			printk("Insert failed at index %d\n", i);
		}
	}

	printk("Heap elements by order of priority:\n");
	MIN_HEAP_FOREACH_CONTAINER(&my_heap, d, handle)
	{
		printk("key=%d value=%d\n", d->key, d->value);
	}

	printk("Top of heap: ");
	const struct min_heap_handle *top_handle = min_heap_peek(&my_heap);

	if (top_handle) {
		const struct data *top = CONTAINER_OF(top_handle, struct data, handle);

		printk("key=%d value=%d\n", top->key, top->value);
	}

	MIN_HEAP_FOREACH_CONTAINER(&my_heap, d, handle)
	{
		if (d->key == target_key) {
			found = d;
			break;
		}
	}

	if (found) {
		printk("Found element with key %d, removing it...\n", target_key);
		min_heap_remove(&my_heap, &found->handle);
	}

	printk("Heap after removal:\n");
	MIN_HEAP_FOREACH_CONTAINER(&my_heap, d, handle)
	{
		printk("key=%d value=%d\n", d->key, d->value);
	}

	return 0;
}
