/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MIN_HEAP_H_
#define ZEPHYR_INCLUDE_SYS_MIN_HEAP_H_

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief min_heap
 * @defgroup min_heap_apis Min-Heap service
 * @ingroup datastructure_apis
 * @{
 */

/**
 * @brief Embeddable heap position handle
 *
 * Embed in the user struct, recover the containing struct with CONTAINER_OF().
 *
 */
struct min_heap_handle {
	/** 1-based storage index; 0 means not in heap */
	uint16_t idx;
};

/**
 * @brief Comparator function type for min-heap ordering.
 *
 * This function compares two heap nodes to establish their relative order.
 * It must be implemented by the user and provided at min-heap
 * initialization.
 *
 * @param a First handle pointer for comparison.
 * @param b Second handle pointer for comparison.
 *
 * @return Negative value if @p a is less than @p b,
 *         positive value if @p a is greater than @p b,
 *         zero if they are equal.
 */
typedef int (*min_heap_cmp_t)(const struct min_heap_handle *a, const struct min_heap_handle *b);

/**
 * @brief min-heap data structure with user-provided comparator.
 */
struct min_heap {
	/** Array of handle pointers: one slot per element */
	struct min_heap_handle **storage;
	/** Maximum number of elements */
	size_t capacity;
	/** Current elements count */
	size_t size;
	/** Comparator function: returns <0, 0, >0 */
	min_heap_cmp_t cmp;
};

/**
 * @brief Define a min-heap instance.
 *
 * @param name Base name for the heap instance.
 * @param cap Capacity (number of elements).
 * @param cmp_func Comparator function used by the heap
 */
#define MIN_HEAP_DEFINE(name, cap, cmp_func)                                                       \
	static struct min_heap_handle *(name##_storage)[(cap)];                                    \
	struct min_heap name = {                                                                   \
		.storage = (name##_storage),                                                       \
		.capacity = (cap),                                                                 \
		.size = 0,                                                                         \
		.cmp = (cmp_func),                                                                 \
	}

/**
 * @brief Define a statically allocated min-heap instance.
 *
 * @param name Base name for the heap instance.
 * @param cap Capacity (number of elements).
 * @param cmp_func Comparator function used by the heap
 */
#define MIN_HEAP_DEFINE_STATIC(name, cap, cmp_func)                                                \
	static struct min_heap_handle *(name##_storage)[(cap)];                                    \
	static struct min_heap name = {                                                            \
		.storage = (name##_storage),                                                       \
		.capacity = (cap),                                                                 \
		.size = 0,                                                                         \
		.cmp = (cmp_func),                                                                 \
	}

/**
 * @brief Initialize a min-heap instance at runtime.
 *
 * Sets up the internal structure of a min heap using a user-provided
 * handle pointer array, capacity, and comparator function. This function must
 * be called before using the heap if not statically defined.
 *
 * @param heap Pointer to the min-heap structure.
 * @param storage Pointer to array of handle pointers.
 * @param capacity Maximum number of elements the heap can store.
 * @param cmp Comparator function used to order the heap elements.
 *
 * @note All arguments must be valid. This function does not allocate memory.
 *
 */
static inline void min_heap_init(struct min_heap *heap, struct min_heap_handle **storage,
				 size_t capacity, min_heap_cmp_t cmp)
{
	__ASSERT_NO_MSG(heap != NULL);
	__ASSERT_NO_MSG(storage != NULL);
	__ASSERT_NO_MSG(cmp != NULL);

	heap->storage = storage;
	heap->capacity = capacity;
	heap->size = 0;
	heap->cmp = cmp;
}

/**
 * @brief Swap two elements in the heap storage, updating their idx fields.
 *
 * @param heap Pointer to the min-heap.
 * @param a Index of the first element.
 * @param b Index of the second element.
 */
static inline void min_heap_swap(struct min_heap *heap, size_t a, size_t b)
{
	struct min_heap_handle *tmp = heap->storage[a];

	heap->storage[a] = heap->storage[b];
	heap->storage[b] = tmp;
	heap->storage[a]->idx = (uint16_t)(a + 1U);
	heap->storage[b]->idx = (uint16_t)(b + 1U);
}

/**
 * @brief Restore heap order by moving a node up the tree.
 *
 * Moves the node at the given index upward in the heap until the min-heap property is restored.
 *
 * @param heap Pointer to the min-heap.
 * @param index Index of the node to heapify upwards.
 */
static inline void min_heap_heapify_up(struct min_heap *heap, size_t index)
{
	while (index > 0) {
		size_t parent = (index - 1U) / 2U;

		if (heap->cmp(heap->storage[index], heap->storage[parent]) >= 0) {
			break;
		}
		min_heap_swap(heap, index, parent);
		index = parent;
	}
}

/**
 * @brief Restore heap order by moving a node down the tree.
 *
 * Moves the node at the sepcified index downward in the heap until the min-heap property is
 * restored.
 *
 * @param heap Pointer to the min-heap.
 * @param index Index of the node to heapify downward.
 */
static inline void min_heap_heapify_down(struct min_heap *heap, size_t index)
{
	for (size_t left = 2U * index + 1U; left < heap->size; left = 2U * index + 1U) {
		size_t right = left + 1U;
		size_t smallest = index;

		if (heap->cmp(heap->storage[left], heap->storage[smallest]) < 0) {
			smallest = left;
		}
		if (right < heap->size &&
		    heap->cmp(heap->storage[right], heap->storage[smallest]) < 0) {
			smallest = right;
		}
		if (smallest == index) {
			break;
		}
		min_heap_swap(heap, index, smallest);
		index = smallest;
	}
}

/**
 * @brief Push a handle into the min-heap.
 *
 * Adds a new handle to the min-heap and restores the heap order by moving it
 * upward as necessary. Insert operation will fail if the min-heap
 * has reached full capacity.
 *
 * @param heap Pointer to the min-heap.
 * @param handle Pointer to the handle to insert.
 *
 * @return 0 on Success, -1 if the heap is full.
 */
static inline int min_heap_push(struct min_heap *heap, struct min_heap_handle *handle)
{
	__ASSERT_NO_MSG(heap != NULL);
	__ASSERT_NO_MSG(handle != NULL);
	__ASSERT_NO_MSG(handle->idx == 0U);
	__ASSERT(heap->size < UINT16_MAX, "heap idx overflow");

	if (heap->size >= heap->capacity) {
		return -1;
	}

	heap->storage[heap->size] = handle;
	handle->idx = (uint16_t)(heap->size + 1U);
	heap->size++;
	min_heap_heapify_up(heap, heap->size - 1U);

	return 0;
}

/**
 * @brief Peek at the top element of the min-heap.
 *
 * The function will not remove the element from the min-heap.
 *
 * @param heap Pointer to the min-heap.
 *
 * @return Pointer to the top handle, or NULL if the heap is empty.
 */
static inline const struct min_heap_handle *min_heap_peek(const struct min_heap *heap)
{
	__ASSERT_NO_MSG(heap != NULL);
	return heap->size == 0U ? NULL : heap->storage[0];
}

/**
 * @brief Remove a specific element from the min-heap by handle.
 *
 * The handle's idx field gives the O(1) location, no linear scan needed.
 *
 * @param heap Pointer to the min-heap.
 * @param handle Handle of the element to remove (must currently be in heap).
 *
 * @return true in success, false otherwise.
 */
static inline bool min_heap_remove(struct min_heap *heap, struct min_heap_handle *handle)
{
	__ASSERT_NO_MSG(heap != NULL);
	__ASSERT_NO_MSG(handle != NULL);

	if (handle->idx == 0U || (size_t)handle->idx > heap->size) {
		return false;
	}

	size_t id = (size_t)(handle->idx - 1U);

	handle->idx = 0U;
	heap->size--;

	if (id != heap->size) {
		struct min_heap_handle *last = heap->storage[heap->size];

		heap->storage[heap->size] = NULL;
		heap->storage[id] = last;
		last->idx = (uint16_t)(id + 1U);
		min_heap_heapify_down(heap, id);
		min_heap_heapify_up(heap, id);
	} else {
		heap->storage[heap->size] = NULL;
	}

	return true;
}

/**
 * @brief Check if the min heap is empty.
 *
 * This function checks whether the heap contains any elements.
 *
 * @param heap Pointer to the min heap.
 *
 * @return true if heap is empty, false otherwise.
 */
static inline bool min_heap_is_empty(struct min_heap *heap)
{
	__ASSERT_NO_MSG(heap != NULL);
	return (heap->size == 0);
}

/**
 * @brief Remove and return the highest priority element's handle.
 *
 * @param heap Pointer to heap.
 *
 * @return Handle of the removed minimum element, or NULL if empty.
 *	   Use CONTAINER_OF() to recover the containing struct.
 */
static inline struct min_heap_handle *min_heap_pop(struct min_heap *heap)
{
	__ASSERT_NO_MSG(heap != NULL);

	if (heap->size == 0U) {
		return NULL;
	}

	struct min_heap_handle *min = heap->storage[0];
	bool ok = min_heap_remove(heap, min);

	__ASSERT_NO_MSG(ok);
	(void)ok;
	return min;
}

/**
 * @brief Iterate over every element currently in the min-heap.
 *
 * Visits all elements in internal storage-array order (BFS level order).
 * This is NOT the sorted order
 *
 * Usage:
 * @code
 *   struct min_heap_handle *h;
 *   MIN_HEAP_FOREACH(&heap, h) {
 *	struct data *t = CONTAINER_OF(h, struct data, handle);
 *   }
 * @endcode
 *
 * @param __heap Pointer to the min-heap.
 * @param __handle min_heap_handle pointer.
 */
#define MIN_HEAP_FOREACH(__heap, __handle)                                                         \
	for (size_t __mh_i = 0;                                                                    \
	     (__mh_i < (__heap)->size) && ((__handle) = (__heap)->storage[__mh_i], true);          \
	     __mh_i++)
/**
 * @brief Iterate over every element in the min-heap yielding the embedding container struct
 *
 * Usage:
 * @code
 *   struct data *t;
 *   MIN_HEAP_FOREACH_CONTAINER(&heap, t, handle) {
 *	printk("key=%d\n", t->key);
 *   }
 * @endcode
 *
 * @param __heap Pointer to the min-heap
 * @param __cn Caller-declared pointer to the container struct type
 * @param __field Name of the min_heap_handle member in the container
 */
#define MIN_HEAP_FOREACH_CONTAINER(__heap, __cn, __field)                                          \
	for (size_t __mh_i = 0;                                                                    \
	     (__mh_i < (__heap)->size) &&                                                          \
	     ((__cn) = CONTAINER_OF((__heap)->storage[__mh_i], __typeof__(*(__cn)), __field),      \
	     true);                                                                                \
	     __mh_i++)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MIN_HEAP_H_ */
