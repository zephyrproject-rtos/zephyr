/*
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MIN_HEAP_H_
#define ZEPHYR_INCLUDE_MIN_HEAP_H_

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief min_heap
 * @defgroup min_heap Min-heap service
 * @ingroup os_services
 * @{
 */

/**
 * @brief Invasive node used inside user structs for min-heap support.
 */
struct min_heap_node {
	/** Key for comparison (optional) */
	int key;
	/** Index within heap array */
	size_t index;
};

/**
 * @brief Comparator function type for min-heap ordering.
 *
 * This function compares two heap nodes to establish their relative order.
 * It must be implemented by the user and provided at min-heap
 * initialization.
 *
 * @param a Pointer to the first node.
 * @param b Pointer to the second node.
 *
 * @return Negative value if @p a is less than @p b,
 *         positive value if @p a is greater than @p b,
 *         zero if they are equal.
 */
typedef int (*min_heap_cmp_fn)(const struct min_heap_node *a,
			       const struct min_heap_node *b);

/**
 * @brief min-heap data structure with user-provided comparator.
 */
struct min_heap {
	/** Pointer to preallocated node pointer array */
	struct min_heap_node **nodes;
	/** Maximum number of elements */
	size_t capacity;
	/** Current elements count */
	size_t size;
	/** Spinlock for thread safety */
	struct k_spinlock lock;
	/** Comparator function */
	min_heap_cmp_fn cmp;
};

/**
 * @brief Static initializer macro for min-heap.
 *
 * This macro provides a convenient way to statically initialize a min_heap
 * structure with a given node storage array, capacity,
 * and comparator function.
 *
 * @param storage Preallocated array of node pointers
 * @param cap Number of elements in the array
 * @param cmp_func Comparator function
 */
#define MIN_HEAP_INIT(storage, cap, cmp_func) \
	{ .nodes = (storage), .capacity = (cap), .size = 0, .lock = {}, \
	.cmp = (cmp_func) }

/**
 * @brief Initialize a min-heap instance.
 *
 * Sets up the internal structure of a min heap using a user-provided
 * storage array, capacity, and comparator function. This function must
 * be called before using the heap.
 *
 * @param heap Pointer to the min-heap structure to initialize.
 * @param storage Pointer to a preallocated array of pointers to heap nodes.
 * @param capacity Maximum number of elements the heap can store.
 * @param cmp Comparator function used to order the heap elements.
 *
 * @note All arguments must be valid. This function does not allocate memory.
 *
 * @return 0 on success, -EINVAL on passing invalid argument.
 */
int min_heap_init(struct min_heap *heap, struct min_heap_node **storage,
		   size_t capacity, min_heap_cmp_fn cmp);

/**
 * @brief Insert a node into the min-heap.
 *
 * Adds a new node to the min-heap and restores the heap order by moving it
 * upward as necessary. This function is thread-safe and uses a spinlock
 * to protect heap state.
 * Insert operation will fail if the min-heap has reached full capacity.
 *
 * @param heap Pointer to the min-heap.
 * @param node Pointer to the node to insert.
 *
 * @return 0 on Success, -EINVAL if the arugments are invalid,
 *         -ENOMEM if the heap is full.
 */
int min_heap_insert(struct min_heap *heap, struct min_heap_node *node);

/**
 * @brief Remove and return the minimum node from the min-heap.
 *
 * Removes the root node (the minimum element) from the min heap, restores
 * heap order using heapify-down, and returns the removed node.
 * The caller gains ownership of the returned node and is responsible for
 * any further management of its memory or reuse.
 *
 * @param heap Pointer to the min-heap.
 *
 * @return Pointer to the removed node, or NULL if the heap is empty or invalid.
 */
struct min_heap_node *min_heap_pop(struct min_heap *heap);

/**
 * @brief Remove a node from the min-heap using its index.
 *
 * Removes the specified node from the min-heap based on the index it stores
 * internally. The min-heap is rebalanced after removal using both heapify-down
 * and heapify-up to ensure proper ordering. The function is thread-safe.
 *
 * @param heap Pointer to the min-heap.
 * @param node Pointer to the node to remove.
 *
 * @return 0 in success, -EINVAL if argument is invalid.
 */
int min_heap_remove_by_idx(struct min_heap *heap, struct min_heap_node *node);

/**
 * @brief Destroy min-heap.
 *
 * Resets the internal state of the min heap, clearing its size, capacity,
 * comparator function, and node pointer. This operation does not free memory,
 * as the heap and its storage are assumed to be statically allocated or
 * externally managed.
 * After calling this function, the heap must be reinitialized using
 * min_heap_init() before reuse.
 * The function is thread safe.
 *
 * @param heap Pointer to the min-heap to destroy
 *
 * @return 0 on success, -EINVAL if the argument is invalid.
 */
int min_heap_destroy(struct min_heap *heap);

/**
 * @brief Check if the min heap is empty.
 *
 * This function checks whether the heap contains any elements.
 * It is thread-safe.
 *
 * @param heap Pointer to the min heap.
 *
 * @return 1 if heap is empty, 0 if the heap is non-empty,
 *         -EINVAL if the argument is invalid.
 */
int min_heap_is_empty(struct min_heap *heap);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MIN_HEAP_H_ */
