/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_HEAP_LISTENER_H
#define ZEPHYR_INCLUDE_SYS_HEAP_LISTENER_H

#include <stdint.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_HEAP_LISTENER) || defined(__DOXYGEN__)

/**
 * @defgroup heap_listener_apis Heap Listener APIs
 * @ingroup heaps
 * @{
 */

enum heap_event_types {
	/*
	 * Dummy event so an un-initialized but zero-ed listener node
	 * will not trigger any callbacks.
	 */
	HEAP_EVT_UNKNOWN = 0,

	HEAP_RESIZE,
	HEAP_ALLOC,
	HEAP_FREE,
	HEAP_REALLOC,

	HEAP_MAX_EVENTS
};

/**
 * @typedef heap_listener_resize_cb_t
 * @brief Callback used when heap is resized
 *
 * @note Minimal C library does not emit this event.
 *
 * @param heap_id Identifier of heap being resized
 * @param old_heap_end Pointer to end of heap before resize
 * @param new_heap_end Pointer to end of heap after resize
 */
typedef void (*heap_listener_resize_cb_t)(uintptr_t heap_id,
					  void *old_heap_end,
					  void *new_heap_end);

/**
 * @typedef heap_listener_alloc_cb_t
 * @brief Callback used when there is heap allocation
 *
 * @note Heaps managed by libraries outside of code in
 *       Zephyr main code repository may not emit this event.
 *
 * @note The number of bytes allocated may not match exactly
 *       to the request to the allocation function. Internal
 *       mechanism of the heap may allocate more than
 *       requested.
 *
 * @param heap_id Heap identifier
 * @param mem Pointer to the allocated memory
 * @param bytes Size of allocated memory
 */
typedef void (*heap_listener_alloc_cb_t)(uintptr_t heap_id,
					 void *mem, size_t bytes);

/**
 * @typedef heap_listener_free_cb_t
 * @brief Callback used when memory is freed from heap
 *
 * @note Heaps managed by libraries outside of code in
 *       Zephyr main code repository may not emit this event.
 *
 * @note The number of bytes freed may not match exactly to
 *       the request to the allocation function. Internal
 *       mechanism of the heap dictates how memory is
 *       allocated or freed.
 *
 * @param heap_id Heap identifier
 * @param mem Pointer to the freed memory
 * @param bytes Size of freed memory
 */
typedef void (*heap_listener_free_cb_t)(uintptr_t heap_id,
					void *mem, size_t bytes);

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

	/**
	 * The heap event to be notified.
	 */
	enum heap_event_types event;

	union {
		heap_listener_alloc_cb_t alloc_cb;
		heap_listener_free_cb_t free_cb;
		heap_listener_resize_cb_t resize_cb;
	};
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
 * @brief Notify listeners of heap allocation event
 *
 * Notify registered heap event listeners with matching heap identifier that an
 * allocation has been done on heap
 *
 * @param heap_id Heap identifier
 * @param mem Pointer to the allocated memory
 * @param bytes Size of allocated memory
 */
void heap_listener_notify_alloc(uintptr_t heap_id, void *mem, size_t bytes);

/**
 * @brief Notify listeners of heap free event
 *
 * Notify registered heap event listeners with matching heap identifier that
 * memory is freed on heap
 *
 * @param heap_id Heap identifier
 * @param mem Pointer to the freed memory
 * @param bytes Size of freed memory
 */
void heap_listener_notify_free(uintptr_t heap_id, void *mem, size_t bytes);

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
 * Construct a heap identifier from a pointer to the heap object, such as
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
 * @brief Define heap event listener node for allocation event
 *
 * Sample usage:
 * @code
 * void on_heap_alloc(uintptr_t heap_id, void *mem, size_t bytes)
 * {
 *   LOG_INF("Memory allocated at %p, size %ld", mem, bytes);
 * }
 *
 * HEAP_LISTENER_ALLOC_DEFINE(my_listener, HEAP_ID_LIBC, on_heap_alloc);
 * @endcode
 *
 * @param name		Name of the heap event listener object
 * @param _heap_id	Identifier of the heap to be listened
 * @param _alloc_cb	Function to be called for allocation event
 */
#define HEAP_LISTENER_ALLOC_DEFINE(name, _heap_id, _alloc_cb) \
	struct heap_listener name = { \
		.heap_id = _heap_id, \
		.event = HEAP_ALLOC, \
		{ \
			.alloc_cb = _alloc_cb \
		}, \
	}

/**
 * @brief Define heap event listener node for free event
 *
 * Sample usage:
 * @code
 * void on_heap_free(uintptr_t heap_id, void *mem, size_t bytes)
 * {
 *   LOG_INF("Memory freed at %p, size %ld", mem, bytes);
 * }
 *
 * HEAP_LISTENER_FREE_DEFINE(my_listener, HEAP_ID_LIBC, on_heap_free);
 * @endcode
 *
 * @param name		Name of the heap event listener object
 * @param _heap_id	Identifier of the heap to be listened
 * @param _free_cb	Function to be called for free event
 */
#define HEAP_LISTENER_FREE_DEFINE(name, _heap_id, _free_cb) \
	struct heap_listener name = { \
		.heap_id = _heap_id, \
		.event = HEAP_FREE, \
		{ \
			.free_cb = _free_cb \
		}, \
	}

/**
 * @brief Define heap event listener node for resize event
 *
 * Sample usage:
 * @code
 * void on_heap_resized(uintptr_t heap_id, void *old_heap_end, void *new_heap_end)
 * {
 *   LOG_INF("Libc heap end moved from %p to %p", old_heap_end, new_heap_end);
 * }
 *
 * HEAP_LISTENER_RESIZE_DEFINE(my_listener, HEAP_ID_LIBC, on_heap_resized);
 * @endcode
 *
 * @param name		Name of the heap event listener object
 * @param _heap_id	Identifier of the heap to be listened
 * @param _resize_cb	Function to be called when the listened heap is resized
 */
#define HEAP_LISTENER_RESIZE_DEFINE(name, _heap_id, _resize_cb) \
	struct heap_listener name = { \
		.heap_id = _heap_id, \
		.event = HEAP_RESIZE, \
		{ \
			.resize_cb = _resize_cb \
		}, \
	}

/** @} */

#else /* CONFIG_HEAP_LISTENER */

#define HEAP_ID_FROM_POINTER(heap_pointer) ((uintptr_t)NULL)

static inline void heap_listener_notify_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	ARG_UNUSED(heap_id);
	ARG_UNUSED(mem);
	ARG_UNUSED(bytes);
}

static inline void heap_listener_notify_free(uintptr_t heap_id, void *mem, size_t bytes)
{
	ARG_UNUSED(heap_id);
	ARG_UNUSED(mem);
	ARG_UNUSED(bytes);
}

static inline void heap_listener_notify_resize(uintptr_t heap_id, void *old_heap_end,
					       void *new_heap_end)
{
	ARG_UNUSED(heap_id);
	ARG_UNUSED(old_heap_end);
	ARG_UNUSED(new_heap_end);
}

#endif /* CONFIG_HEAP_LISTENER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HEAP_LISTENER_H */
