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
 * @brief Swap two elements in the heap.
 *
 * This function swaps the contents of two elements at the specified indices
 * within the heap.
 *
 * @param heap Pointer to the min-heap.
 * @param a Index of the first element.
 * @param b Index of the second element.
 */
static void swap(struct min_heap *heap, size_t a, size_t b)
{
	uint8_t tmp[heap->elem_size];
	void *elem_a = min_heap_get_element(heap, a);
	void *elem_b = min_heap_get_element(heap, b);

	memcpy(tmp, elem_a, heap->elem_size);
	memcpy(elem_a, elem_b, heap->elem_size);
	memcpy(elem_b, tmp, heap->elem_size);
}

/**
 * @brief Restore heap order by moving a node up the tree.
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
	void *curr, *par;

	while (index > 0) {
		parent = (index - 1) / 2;
		curr = min_heap_get_element(heap, index);
		par = min_heap_get_element(heap, parent);
		if (heap->cmp(curr, par) >= 0) {
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
	size_t left, right, smallest;

	while (true) {
		left = 2 * index + 1;
		right = 2 * index + 2;
		smallest = index;

		if (left < heap->size &&
			heap->cmp(min_heap_get_element(heap, left),
			min_heap_get_element(heap, smallest)) < 0) {
			smallest = left;
		}

		if (right < heap->size &&
			heap->cmp(min_heap_get_element(heap, right),
			min_heap_get_element(heap, smallest)) < 0) {
			smallest = right;
		}

		if (smallest == index) {
			break;
		}

		swap(heap, index, smallest);
		index = smallest;
	}
}


void min_heap_init(struct min_heap *heap, void *storage, size_t cap,
		   size_t elem_size, min_heap_cmp_t cmp)
{
	heap->storage = storage;
	heap->capacity = cap;
	heap->elem_size = elem_size;
	heap->cmp = cmp;
	heap->size = 0;
}

void *min_heap_peek(const struct min_heap *heap)
{
	if (heap->size == 0) {
		return NULL;
	}

	return min_heap_get_element(heap, 0);
}

int min_heap_push(struct min_heap *heap, const void *item)
{
	if (heap->size >= heap->capacity) {
		return -ENOMEM;
	}

	void *dest = min_heap_get_element(heap, heap->size);

	memcpy(dest, item, heap->elem_size);
	heapify_up(heap, heap->size);
	heap->size++;

	return 0;
}

bool min_heap_remove(struct min_heap *heap, size_t id, void *out_buf)
{
	if (id >= heap->size) {
		return false;
	}

	void *removed = min_heap_get_element(heap, id);

	memcpy(out_buf, removed, heap->elem_size);
	heap->size--;
	if (id != heap->size) {
		void *last = min_heap_get_element(heap, heap->size);

		memcpy(removed, last, heap->elem_size);
		heapify_down(heap, id);
		heapify_up(heap, id);
	}

	return true;
}

bool min_heap_pop(struct min_heap *heap, void *out_buf)
{
	return min_heap_remove(heap, 0, out_buf);
}

void *min_heap_find(struct min_heap *heap, min_heap_eq_t eq,
		    const void *other, size_t *out_id)
{
	void *element;

	for (size_t i = 0; i < heap->size; ++i) {

		element = min_heap_get_element(heap, i);
		if (eq(element, other)) {
			if (out_id) {
				*out_id = i;
			}
			return element;
		}
	}

	return NULL;
}
