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
 * @brief Node Structure to hold heap data
 */
struct heap_node {
	/** Pointer to the data held. */
	void *data;
	/** Priority value of the data. */
	uint32_t priority;
	/** Length of data. */
	size_t length;
};

/**
 * @brief Represents a fixed-size min-heap data structure.
 */
struct min_heap {
	/** Holds the list of minimum heap nodes. */
	struct heap_node *nodes;
	/** Total Capacity of the minimum heap. */
	size_t capacity;
	/** The used size of the minimum heap. */
	size_t size;
	/** Lock */
	struct k_spinlock lock;
};

/**
 * @brief Initialize the minimum heap.
 *
 * Initializes the minimum heap with the required capacity.
 *
 * @param heap Pointer to heap structure.
 * @param buffer Pointer to heap buffer.
 * @param capacity Total Capacity of the minimum heap.
 *
 * @return Error value at failure, 0 at success.
 */
int min_heap_init(struct min_heap *heap, struct heap_node *buffer,
		   size_t capacity);

/**
 * @brief Insert to minimum heap.
 *
 * Inserts a node to the minimum heap and re-arranges the priority
 * if the elements in the buffer. Insert operation will fail if
 * the heap is full.
 *
 * @param heap Pointer to heap structure.
 * @param data Data to be stored in the buffer.
 * @param length length of the data to be stored.
 * @param priority priority of the data.
 *
 * @return 0 if successful, error number if it fails to insert.
 */
int min_heap_insert(struct min_heap *heap, void *data, size_t length,
		    uint32_t priority);

/**
 * @brief Return the highest priority node in the heap
 *
 * Returns the top most element of the minimum heap.
 *
 * @param heap Pointer to the heap structure.
 * @param length length of the top node data.
 *
 * @return NULL if the buffer is empty. Node Data if the list is non-empty.
 */
void *min_heap_peek_min(struct min_heap *heap, size_t *length);

/**
 * @brief Remove and return the highest priority Node.
 *
 * Remove and return the top most Node in th minimum heap and re-arrange
 * the buffer according to the priority.
 * NOTE: It is the user's responsibility to free the pointer after use.
 *
 * @param heap Pointer to the heap structure.
 * @param length length of the extracted data.
 *
 * @return NULL if the buffer is empty. Node Data if the list is non-empty.
 */
void *min_heap_extract_min(struct min_heap *heap, size_t *length);

/**
 * @brief Check if the minimum heap is empty
 *
 * If the size of the minimum heap is zero, return empty.
 *
 * @param heap Pointer to the heap structure.
 *
 * @return true if buffer is non-empty, else false.
 */
bool min_heap_is_empty(struct min_heap *heap);

/**
 * @brief Deinitialize the heap and clear node metadata.
 *
 * This function resets all heap nodes and clears their metadata.
 * It is safe for statically allocated memory pools and does not
 * free node data.
 *
 * @param heap Pointer to the minimum heap.
 */
void min_heap_destroy(struct min_heap *heap);

/**
 * @brief Statically define and initialize a min-heap instance.
 *
 * This macro declares a min-heap and its backing buffer, then
 * calls min_heap_init() to initialize the structure.
 *
 * @param name Base name for the heap instance. This macro will declare:
 *			   - `struct heap_node name##_nodes[]`
 *			   - `struct min_heap name`
 * @param capacity Maximum number of elements the heap can hold.
 */
#define MIN_HEAP_DEFINE(name, capacity) \
	struct heap_node name##_nodes[capacity]; \
	struct min_heap name;

#ifdef __cplusplus
}
#endif

#endif
