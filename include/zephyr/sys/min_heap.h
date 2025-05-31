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
 * @defgroup min_heap Min-Heap service
 * @ingroup datastructure_apis
 * @{
 */

/**
 * @brief Invasive node used inside user structs for min-heap support.
 */
struct min_heap_node {
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
	{ .nodes = (storage), .capacity = (cap), .cmp = (cmp_func) }

/**
 * @brief Define and initialize a min-heap instance statically
 *
 * @param name Name of the heap instance
 * @param cap  Capacity (number of nodes)
 * @param cmp_func Comparison function
 */
#define MIN_HEAP_DEFINE(name, cap, cmp_func) \
	static struct min_heap_node *name##_buf[cap]; \
	struct min_heap name = MIN_HEAP_INIT(name##_buf, cap, cmp_func)

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
 */
static void min_heap_init(struct min_heap *heap,
				struct min_heap_node **storage,
				size_t capacity, min_heap_cmp_fn cmp)
{
	*heap = (struct min_heap)MIN_HEAP_INIT(storage, capacity, cmp);
}

/**
 * @brief Insert a node into the min-heap.
 *
 * Adds a new node to the min-heap and restores the heap order by moving it
 * upward as necessary. Insert operation will fail if the min-heap
 * has reached full capacity.
 *
 * @param heap Pointer to the min-heap.
 * @param node Pointer to the node to insert.
 *
 * @return 0 on Success, -ENOMEM if the heap is full.
 */
int min_heap_insert(struct min_heap *heap, struct min_heap_node *node);

/**
 * @brief Remove and return the minimum node from the min-heap.
 *
 * Removes the root node (the minimum element) from the min heap, restores
 * heap order and returns the removed node.
 * The caller gains ownership of the returned node and is responsible for
 * any further management of its memory or reuse.
 *
 * @param heap Pointer to the min-heap.
 *
 * @return Pointer to the removed node, or NULL if the heap is empty.
 */
struct min_heap_node *min_heap_pop(struct min_heap *heap);

/**
 * @brief Remove a specific node from the min-heap.
 *
 * Removes the specified node from the min-heap based on the ID it stores
 * internally. The min-heap is rebalanced after removal to ensure
 * proper ordering.
 * The caller gains ownership of the returned node and is responsible for
 * any further management of its memory or reuse.
 *
 * @param heap Pointer to the min-heap.
 * @param id Node ID to be removed.
 *
 * @return Pointer to the removed node in success, NULL if ID is invalid.
 */
struct min_heap_node *min_heap_remove(struct min_heap *heap, size_t id);

/**
 * @brief Check if the min heap is empty.
 *
 * This function checks whether the heap contains any elements.
 *
 * @param heap Pointer to the min heap.
 *
 * @return true if heap is empty, false if the heap is non-empty.
 */
bool min_heap_is_empty(struct min_heap *heap);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MIN_HEAP_H_ */
