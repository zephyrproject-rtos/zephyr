/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/sys/min_heap.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(min_heap);

/**
 * @brief Swap two nodes in the min-heap.
 *
 * Exchanges the positions of two nodes at the given indices in the min-heap's
 * internal node array, and updates their respective index fields accordingly.
 *
 * @param index_a index of the first node.
 * @param index_b index of the second node.
 */
static inline void swap(struct min_heap *heap, size_t index_a, size_t index_b)
{
	struct min_heap_node *tmp;

	tmp = heap->nodes[index_a];
	heap->nodes[index_a] = heap->nodes[index_b];
	heap->nodes[index_b] = tmp;
	heap->nodes[index_a]->index = index_a;
	heap->nodes[index_b]->index = index_b;
}

/**
 * @brief @brief Restore heap order by moving a node up the tree.
 *
 * Moves the node at the given index upward in the heap until the min-heap
 * property is restored.
 *
 * @param heap Pointer to the min-heap.
 * @param index Index of the node to heapify upwards.
 */
static void heapify_up(struct min_heap *heap, size_t index)
{
	size_t parent;

	while (index > 0) {
		parent = (index - 1) / 2;
		if (heap->cmp(heap->nodes[index], heap->nodes[parent]) >= 0) {
			break;
		}
		swap(heap, index, parent);
		index = parent;
	}
}

/**
 * @brief Restore heap order by moving a node down the tree.
 *
 * Moves the node at the specified index downward in the heap until the
 * min-heap property is restored.
 *
 * @param heap Pointer to the min-heap.
 * @param index Index of the node to heapify downward.
 */

static void heapify_down(struct min_heap *heap, size_t index)
{
	size_t left, right, small;

	while (1) {
		left = 2 * index + 1;
		right = 2 * index + 2;
		small = index;
		if (left < heap->size &&
		    heap->cmp(heap->nodes[left], heap->nodes[small]) < 0) {
			small = left;
		}
		if (right < heap->size &&
		    heap->cmp(heap->nodes[right], heap->nodes[small]) < 0) {
			small = right;
		}
		if (small == index) {
			break;
		}
		swap(heap, index, small);
		index = small;
	}
}

int min_heap_insert(struct min_heap *heap, struct min_heap_node *node)
{
	if (heap->size >= heap->capacity) {
		return -ENOMEM;
	}

	node->index = heap->size;
	heap->nodes[heap->size++] = node;
	heapify_up(heap, node->index);

	return 0;
}

struct min_heap_node *min_heap_remove(struct min_heap *heap, size_t id)
{
	if (id >= heap->size) {
		return NULL;
	}

	struct min_heap_node *removed = heap->nodes[id];

	heap->nodes[id] = heap->nodes[--heap->size];
	heap->nodes[heap->size] = NULL;
	if (id < heap->size) {
		heap->nodes[id]->index = id;
		heapify_down(heap, id);
		heapify_up(heap, id);
	}

	return removed;
}

struct min_heap_node *min_heap_pop(struct min_heap *heap)
{
	if (heap->size == 0) {
		return NULL;
	}

	return min_heap_remove(heap, 0);
}

bool min_heap_is_empty(struct min_heap *heap)
{
	return (heap->size == 0);
}
