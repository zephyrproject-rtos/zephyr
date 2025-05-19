/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/min_heap.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(min_heap);

int min_heap_init(struct min_heap *heap, struct heap_node *buffer,
				   size_t capacity)
{
	if (heap == NULL || buffer == NULL) {
		return -EINVAL;
	}

	heap->nodes = buffer;
	heap->capacity = capacity;
	heap->size = 0;

	return 0;
}

/**
 * @brief Restores the minimum heap property after insertion of New Node
 *
 * Arranges the nodes according to the priotity of nodes
 *
 * @param heap Pointer to heap structure.
 * @param index index of the insertion of node.
 */
static void heapify_up(struct min_heap *heap, size_t index)
{
	struct heap_node temp;
	size_t parent;

	if (heap == NULL || heap->size == 0) {
		return;
	}

	while (index > 0) {
		parent = (index - 1) / 2;
		if (heap->nodes[index].priority >= heap->nodes[parent].priority) {
			break;
		}
		temp = heap->nodes[index];
		heap->nodes[index] = heap->nodes[parent];
		heap->nodes[parent] = temp;
		index = parent;
	}
}

int min_heap_insert(struct min_heap *heap, void *data, size_t length,
					uint32_t priority)
{
	k_spinlock_key_t key = k_spin_lock(&heap->lock);
	int ret = 0;
	void *copied_data;

	if (heap == NULL && data == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (heap->size >= heap->capacity) {
		LOG_ERR("Heap is full. Max Size %d", heap->size);
		ret = -ENOMEM;
		goto out;
	}

	copied_data = malloc(length);
	if (copied_data == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	memcpy(copied_data, data, length);
	heap->nodes[heap->size].data = copied_data;
	heap->nodes[heap->size].priority = priority;
	heap->nodes[heap->size].length = length;
	heapify_up(heap, heap->size);
	heap->size++;

out:
	k_spin_unlock(&heap->lock, key);

	return ret;
}

void *min_heap_peek_min(struct min_heap *heap, size_t *length)
{
	void *data = NULL;
	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	*length = 0;
	if (heap != NULL && heap->size > 0) {
		data = heap->nodes[0].data;
		if (length != NULL) {
			*length = heap->nodes[0].length;
		}
	}
	k_spin_unlock(&heap->lock, key);

	return data;
}

/**
 * @brief Restores the minimum heap property after deletion of node
 *
 * Re-arrange the Nodes according to the priority after a node gets
 * removed from the minimum heap.
 *
 * @param heap Pointer to the heap structure.
 * @param index Index of deleted node.
 *
 * @return No return.
 */
static void heapify_down(struct min_heap *heap, size_t index)
{
	struct heap_node temp;
	size_t smallest, left, right;

	if (heap == NULL || heap->size == 0) {
		return;
	}

	while (1) {
		left = 2 * index + 1;
		right = 2 * index + 2;
		smallest = index;
		if (left < heap->size &&
			heap->nodes[left].priority < heap->nodes[smallest].priority) {
			smallest = left;
		}
		if (right < heap->size &&
			heap->nodes[right].priority < heap->nodes[smallest].priority) {
			smallest = right;
		}
		if (smallest == index) {
			break;
		}
		temp = heap->nodes[index];
		heap->nodes[index] = heap->nodes[smallest];
		heap->nodes[smallest] = temp;
		index = smallest;
	}
}

void *min_heap_extract_min(struct min_heap *heap, size_t *length)
{
	void *min_data;
	struct heap_node *min_node;
	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	if (heap == NULL || heap->size == 0) {
		k_spin_unlock(&heap->lock, key);
		if (length != NULL) {
			*length = 0;
		}
		LOG_ERR("No Nodes available in the heap");
		return NULL;
	}

	min_node = &heap->nodes[0];
	min_data = malloc(min_node->length);
	if (min_data == NULL) {
		k_spin_unlock(&heap->lock, key);
		if (length != NULL) {
			*length = 0;
		}
		LOG_ERR("Failed to allocate memory to extract");
		return NULL;
	}

	memcpy(min_data, min_node->data, min_node->length);
	if (length != NULL) {
		*length = min_node->length;
	}
	heap->nodes[0] = heap->nodes[heap->size - 1];
	heap->size--;
	heapify_down(heap, 0);
	k_spin_unlock(&heap->lock, key);

	return min_data;
}

bool min_heap_is_empty(struct min_heap *heap)
{
	bool empty = false;
	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	if (heap != NULL) {
		empty = (heap->size == 0);
	}
	k_spin_unlock(&heap->lock, key);

	return empty;
}

void min_heap_destroy(struct min_heap *heap)
{
	size_t i;

	if (heap == NULL || heap->nodes == NULL) {
		return;
	}

	for (i = 0; i < heap->size; i++) {
		heap->nodes[i].data = NULL;
		heap->nodes[i].length = 0;
		heap->nodes[i].priority = 0;
	}

	heap->size = 0;
}
