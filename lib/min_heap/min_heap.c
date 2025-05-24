/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/min_heap.h>
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

	if (heap == NULL || index_a == index_b) {
		return;
	}

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

	if (heap == NULL || heap->size == 0) {
		return;
	}

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

	if (heap == NULL || heap->size == 0) {
		return;
	}

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

int min_heap_init(struct min_heap *heap, struct min_heap_node **storage,
		  size_t capacity, min_heap_cmp_fn cmp)
{
	if (heap == NULL || storage == NULL || cmp == NULL || capacity == 0) {
		LOG_ERR("Invalid Parameters passed to initialise min-heap");
		return -EINVAL;
	}

	heap->nodes = storage;
	heap->capacity = capacity;
	heap->size = 0;
	heap->cmp = cmp;
	heap->lock = (struct k_spinlock){};

	return 0;
}

int min_heap_insert(struct min_heap *heap, struct min_heap_node *node)
{
	if (heap == NULL || node == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	if (heap->size >= heap->capacity) {
		k_spin_unlock(&heap->lock, key);
		return -ENOMEM;
	}

	node->index = heap->size;
	heap->nodes[heap->size++] = node;
	heapify_up(heap, node->index);
	k_spin_unlock(&heap->lock, key);

	return 0;
}

struct min_heap_node *min_heap_pop(struct min_heap *heap)
{
	if (heap == NULL) {
		return NULL;
	}

	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	if (heap->size == 0) {
		k_spin_unlock(&heap->lock, key);
		return NULL;
	}

	struct min_heap_node *min = heap->nodes[0];

	heap->nodes[0] = heap->nodes[--heap->size];
	heap->nodes[heap->size] = NULL;

	if (heap->size > 0) {
		heap->nodes[0]->index = 0;
		heapify_down(heap, 0);
	}
	k_spin_unlock(&heap->lock, key);

	return min;
}

int min_heap_remove_by_idx(struct min_heap *heap, struct min_heap_node *node)
{
	if (heap == NULL || node == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	if (node->index >= heap->size) {
		k_spin_unlock(&heap->lock, key);
		return -EINVAL;
	}

	size_t i = node->index;

	heap->nodes[i] = heap->nodes[--heap->size];
	heap->nodes[heap->size] = NULL;
	if (i < heap->size) {
		heap->nodes[i]->index = i;
		heapify_down(heap, i);
		heapify_up(heap, i);
	}
	k_spin_unlock(&heap->lock, key);

	return 0;
}

int min_heap_destroy(struct min_heap *heap)
{
	if (heap == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	heap->nodes = NULL;
	heap->capacity = 0;
	heap->size = 0;
	heap->cmp = NULL;
	k_spin_unlock(&heap->lock, key);

	return 0;
}

int min_heap_is_empty(struct min_heap *heap)
{
	if (heap == NULL) {
		return -EINVAL;
	}

	int is_empty;

	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	is_empty = (heap->size == 0);
	k_spin_unlock(&heap->lock, key);

	return is_empty;
}
