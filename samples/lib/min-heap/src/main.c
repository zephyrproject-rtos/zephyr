/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/min_heap.h>
#include <zephyr/logging/log.h>

#define MAX_ITEMS 3

LOG_MODULE_REGISTER(min_heap_app);

struct data {
	struct min_heap_node node;
	const char *name;
	int key;
};

static struct min_heap_node *heap_buf[MAX_ITEMS];
static struct min_heap heap;

static struct data items[MAX_ITEMS] = {
	{ .key = 7, .name = "Task1" },
	{ .key = 2, .name = "Task2" },
	{ .key = 3, .name = "Task3" }
};

static int compare(const struct min_heap_node *a, const struct min_heap_node *b)
{
	const struct data *item_a = CONTAINER_OF(a, struct data, node);
	const struct data *item_b = CONTAINER_OF(b, struct data, node);

	return item_a->key - item_b->key;
}

int main(void)
{
	int ret = 0;
	struct min_heap_node *node;
	const struct data *item;

	min_heap_init(&heap, heap_buf, MAX_ITEMS, compare);

	for (int i = 0; i < MAX_ITEMS; i++) {

		ret = min_heap_insert(&heap, &items[i].node);
		if (ret != 0) {
			LOG_ERR("min-heap insert failure %d", ret);
			return ret;
		}
	}

	node = min_heap_pop(&heap);
	if (node == NULL) {
		LOG_ERR("Min-Heap empty");
		return -1;
	}

	item = CONTAINER_OF(node, struct data, node);
	LOG_INF("Top node key value %d", item->key);

	node = min_heap_remove(&heap, 1);
	item = CONTAINER_OF(node, struct data, node);
	LOG_INF("Removed node key value %d", item->key);

	ret = min_heap_is_empty(&heap);
	if (ret == 0) {
		LOG_INF("Heap is not empty");
	}

	return 0;
}
