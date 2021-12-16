/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_HEAP_LISTENER_H
#define ZEPHYR_INCLUDE_SYS_HEAP_LISTENER_H

#include <stdint.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif


struct heap_listener {
	/** Singly linked list node */
	sys_snode_t node;

	/**
	 * Identifier of the heap whose events are listened.
	 *
	 * It can be a heap pointer, if the heap is represented as an object,
	 * or 0 in the case of the global libc heap.
	 */
	uintptr_t heap_id;

	/** Function called when the listened heap is resized */
	void (*resize_cb)(void *old_heap_end, void *new_heap_end);
};

/**
 * @brief Register heap event listener
 *
 * Add the listener to the global list of heap listeners that can be notified by
 * different heap implementations upon certain events related to the heap usage.
 *
 * @param listener Pointer to the heap_listener object
 */
void heap_listener_register(struct heap_listener *listener);

/**
 * @brief Unregister heap event listener
 *
 * Remove the listener from the global list of heap listeners that can be
 * notified by different heap implementations upon certain events related to the
 * heap usage.
 *
 * @param listener Pointer to the heap_listener object
 */
void heap_listener_unregister(struct heap_listener *listener);

/**
 * @brief Notify listeners of heap resize event
 *
 * Notify registered heap event listeners with matching heap identifier that the
 * heap has been resized.
 *
 * @param heap_id Heap identifier
 * @param old_heap_end Address of the heap end before the change
 * @param new_heap_end Address of the heap end after the change
 */
void heap_listener_notify_resize(uintptr_t heap_id, void *old_heap_end, void *new_heap_end);

/**
 * @brief Construct heap identifier from heap pointer
 *
 * Construct a heap identifer from a pointer to the heap object, such as
 * sys_heap.
 *
 * @param heap_pointer Pointer to the heap object
 */
#define HEAP_ID_FROM_POINTER(heap_pointer) ((uintptr_t)heap_pointer)

/**
 * @brief Libc heap identifier
 *
 * Identifier of the global libc heap.
 */
#define HEAP_ID_LIBC ((uintptr_t)0)

/**
 * @brief Define heap event listener object
 *
 * Sample usage:
 * @code
 * void on_heap_resized(void *old_heap_end, void *new_heap_end)
 * {
 *   LOG_INF("Libc heap end moved from %p to %p", old_heap_end, new_heap_end);
 * }
 *
 * HEAP_LISTENER_DEFINE(my_listener, HEAP_ID_LIBC, on_heap_resized);
 * @endcode
 *
 * @param name		Name of the heap event listener object
 * @param _heap_id	Identifier of the heap to be listened
 * @param _resize_cb	Function to be called when the listened heap is resized
 */
#define HEAP_LISTENER_DEFINE(name, _heap_id, _resize_cb) \
	struct heap_listener name = { .heap_id = _heap_id, .resize_cb = _resize_cb }

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HEAP_LISTENER_H */
