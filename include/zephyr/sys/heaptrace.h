/*
 *  Copyright (c) 2026 Picoheart Inc.
 *  Copyright (c) 2026 LiuQian.andy <liuqian.andy@picoheart.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Heap trace public API
 *
 * The heap trace module listens to heap alloc/free events through the
 * Zephyr heap_listener API, records outstanding allocations, supports
 * per-thread / per-CPU filtering and optional stack backtrace to locate
 * the original caller, and exposes the results through shell commands.
 *
 * Typical usage:
 * @code
 * // 1. heap_listener callback path invokes heaptrace_alloc/free
 * //    automatically (registered internally, no user action needed)
 *
 * // 2. Set an acquisition filter to only record allocations of a thread
 * heaptrace_set_filter_name("app_thread");
 *
 * // 3. Iterate outstanding records for custom processing
 * heaptrace_foreach_record(my_cb, &ctx);
 * @endcode
 */

#ifndef ZEPHYR_INCLUDE_SYS_HEAP_TRACE_H
#define ZEPHYR_INCLUDE_SYS_HEAP_TRACE_H

#include <zephyr/kernel.h>
#include <zephyr/sys/heap_listener.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_HEAPTRACE)

/** Maximum thread name length including the terminating '\0' */
#define HEAPTRACE_THREAD_NAME_LEN 16

/**
 * @brief Acquisition-time filter type
 *
 * Checked at the entry of alloc/free to decide whether the event matches
 * the filter. Non-matching events are dropped before being stored, which
 * reduces the amount of irrelevant records.
 */
enum heaptrace_filter_type {
	HEAPTRACE_FILTER_NONE = 0, /**< No filter, record all events */
	HEAPTRACE_FILTER_TID,      /**< Only record allocations of a given thread */
	HEAPTRACE_FILTER_NAME,     /**< Only record allocations whose name contains a substring */
	HEAPTRACE_FILTER_CPU,      /**< Only record allocations on a given CPU */
};

/**
 * @brief Read-only snapshot of one outstanding allocation record
 *
 * Copied under the lock by heaptrace_foreach_record() and then handed to
 * the callback. It can be accessed safely without holding the lock. The
 * backtrace data pointed to by bt_buf lives in an internal stack buffer
 * and remains valid for the duration of the callback.
 */
struct heaptrace_record_info {
	void *ptr;         /**< Address returned by the allocator */
	size_t size;       /**< Number of bytes allocated */
	uintptr_t heap_id; /**< Heap identifier, HEAP_ID_LIBC or a sys_heap-derived value */

	k_tid_t alloc_tid; /**< Thread that performed the allocation */
	char alloc_thread_name[HEAPTRACE_THREAD_NAME_LEN]; /**< Name of the allocating thread */
	uint32_t alloc_time_ms; /**< Allocation timestamp (k_uptime_get_32, ms) */
	unsigned int alloc_cpu; /**< CPU id where the allocation happened */

	int bt_depth;                /**< Backtrace depth, 0 means no backtrace */
	const unsigned long *bt_buf; /**< Backtrace frame array, length is bt_depth */
};

/**
 * @brief Record iteration callback prototype
 *
 * @param info  Read-only snapshot of the current record (accessed outside
 *              the lock, data is stable during the callback)
 * @param user_data User data passed through from heaptrace_foreach_record()
 * @return true  Continue to the next record
 * @return false Stop iterating
 */
typedef bool (*heaptrace_record_cb_t)(const struct heaptrace_record_info *info, void *user_data);

/**
 * @brief Initialize the heaptrace module
 *
 * Clears the record table and filter state, and registers heap_listener
 * entries for libc and sys_heap. Invoked automatically by SYS_INIT at
 * PRE_KERNEL_1 priority; usually no manual call is required.
 *
 * @return 0
 */
int heaptrace_init(void);

/**
 * @brief Clear all outstanding records
 *
 * Only clears the record table; the currently configured filter is kept.
 */
void heaptrace_reset(void);

/**
 * @brief Record one allocation
 *
 * Usually called from a heap_listener callback; user code rarely needs to
 * call it directly. If the event does not match the current filter it is
 * dropped.
 *
 * @param ptr     Address returned by the allocator, ignored if NULL
 * @param size    Number of bytes allocated, ignored if 0
 * @param heap_id Heap identifier (HEAP_ID_LIBC or a sys_heap-derived value)
 */
void heaptrace_alloc(void *ptr, size_t size, uintptr_t heap_id);

/**
 * @brief Record one free
 *
 * Looks up the matching ptr in the record table and removes it. Unlike
 * heaptrace_alloc(), the free path is not gated by the acquisition filter:
 * a block allocated by a filtered thread may be freed by another thread,
 * and dropping such frees would leave phantom leaks in the table. If the
 * ptr is not found a warning is emitted.
 *
 * @param ptr     Address being freed, ignored if NULL
 * @param size    Number of bytes freed (may be 0)
 * @param heap_id Heap identifier
 */
void heaptrace_free(void *ptr, size_t size, uintptr_t heap_id);

/**
 * @brief Iterate over all outstanding allocation records
 *
 * For each record a snapshot is copied under the lock and the callback is
 * invoked after releasing the lock. The callback may safely call blocking
 * output APIs such as shell_print / LOG. Returning false stops the walk.
 *
 * @param cb        Callback function
 * @param user_data User data passed to the callback
 */
void heaptrace_foreach_record(heaptrace_record_cb_t cb, void *user_data);

/**
 * @brief Total bytes of outstanding allocations
 * @return Number of bytes
 */
size_t heaptrace_outstanding_bytes(void);

/**
 * @brief Number of outstanding allocation blocks
 * @return Number of blocks
 */
size_t heaptrace_outstanding_blocks(void);

/**
 * @brief Find a thread tid by name substring
 *
 * Iterates all threads in the system and returns the first one whose name
 * contains @p name. Useful to convert a shell-provided thread name into a
 * tid before setting a filter.
 *
 * @param name Thread name substring
 * @return Matching tid, NULL if not found
 */
k_tid_t heaptrace_find_tid_by_name(const char *name);

/**
 * @brief Set acquisition filter: only record allocations of the given thread
 * @param tid Target thread
 */
void heaptrace_set_filter_tid(k_tid_t tid);

/**
 * @brief Set acquisition filter: only record allocations whose thread name contains @p name
 * @param name Thread name substring; ignored if empty or NULL
 */
void heaptrace_set_filter_name(const char *name);

/**
 * @brief Set acquisition filter: only record allocations on the given CPU
 * @param cpu_id Target CPU id
 */
void heaptrace_set_filter_cpu(unsigned int cpu_id);

/**
 * @brief Clear the acquisition filter and resume recording all events
 */
void heaptrace_clear_filter(void);

/**
 * @brief Get the current filter type
 * @return Filter type enum value
 */
enum heaptrace_filter_type heaptrace_get_filter_type(void);

/**
 * @brief Get the tid of the current filter (only meaningful when type is HEAPTRACE_FILTER_TID)
 * @return tid
 */
k_tid_t heaptrace_get_filter_tid(void);

/**
 * @brief Get the name substring of the current filter (only meaningful when type is
 * HEAPTRACE_FILTER_NAME)
 * @return Pointer to an internal buffer, no need to free
 */
const char *heaptrace_get_filter_name(void);

/**
 * @brief Get the CPU id of the current filter (only meaningful when type is HEAPTRACE_FILTER_CPU)
 * @return cpu id
 */
unsigned int heaptrace_get_filter_cpu(void);

/**
 * @brief Format the current filter state as a human-readable string
 *
 * For example "tid=0x20001234", "name=\"app\"", "cpu=1", "none".
 *
 * @param buf  Output buffer
 * @param len  Buffer length
 * @return Pointer to @p buf
 */
const char *heaptrace_filter_str(char *buf, size_t len);

/**
 * @brief Read-only snapshot of one heap resize event
 *
 * Copied under the lock by heaptrace_foreach_resize() and handed to the
 * callback. Safe to access without holding the lock.
 */
struct heaptrace_resize_info {
	uintptr_t heap_id;                           /**< Heap identifier */
	void *old_end;                               /**< Heap end before the resize */
	void *new_end;                               /**< Heap end after the resize */
	k_tid_t tid;                                 /**< Thread that triggered the resize */
	char thread_name[HEAPTRACE_THREAD_NAME_LEN]; /**< Name of the triggering thread */
	uint32_t time_ms;                            /**< Resize timestamp (k_uptime_get_32, ms) */
	unsigned int cpu;                            /**< CPU id where the resize happened */
};

/**
 * @brief Current boundary snapshot of one tracked heap
 *
 * heap_start is the first observed heap end (captured when the first resize
 * event for this heap arrives), and heap_end is the latest heap end. The
 * difference heap_end - heap_start is the total growth observed since
 * tracking started.
 */
struct heaptrace_heap_info {
	uintptr_t heap_id; /**< Heap identifier */
	void *heap_start;  /**< First observed heap end */
	void *heap_end;    /**< Latest heap end */
	bool valid;        /**< Whether this slot is in use */
};

/**
 * @brief Resize history iteration callback prototype
 *
 * @param info      Read-only snapshot of the current resize event
 * @param user_data User data passed through from heaptrace_foreach_resize()
 * @return true  Continue to the next entry
 * @return false Stop iterating
 */
typedef bool (*heaptrace_resize_cb_t)(const struct heaptrace_resize_info *info, void *user_data);

/**
 * @brief Heap boundary iteration callback prototype
 *
 * @param info      Read-only snapshot of the current heap boundary
 * @param user_data User data passed through from heaptrace_foreach_heap()
 * @return true  Continue to the next heap
 * @return false Stop iterating
 */
typedef bool (*heaptrace_heap_cb_t)(const struct heaptrace_heap_info *info, void *user_data);

/**
 * @brief Record one heap resize event
 *
 * Usually called from a heap_listener resize callback; user code rarely
 * needs to call it directly. Updates the heap boundary and appends an entry
 * to the resize history ring buffer. Also emits a LOG_INF line describing
 * the change. Resize events are not subject to the alloc/free filter.
 *
 * @param heap_id Heap identifier
 * @param old_end Heap end before the resize
 * @param new_end Heap end after the resize
 */
void heaptrace_resize(uintptr_t heap_id, void *old_end, void *new_end);

/**
 * @brief Iterate over resize event history (oldest first)
 *
 * For each entry a snapshot is copied under the lock and the callback is
 * invoked after releasing the lock. The callback may safely call blocking
 * output APIs such as shell_print / LOG. Returning false stops the walk.
 *
 * @param cb        Callback function
 * @param user_data User data passed to the callback
 */
void heaptrace_foreach_resize(heaptrace_resize_cb_t cb, void *user_data);

/**
 * @brief Iterate over all tracked heap boundaries
 *
 * @param cb        Callback function, return false to stop
 * @param user_data User data passed to the callback
 */
void heaptrace_foreach_heap(heaptrace_heap_cb_t cb, void *user_data);

#endif /* CONFIG_HEAPTRACE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HEAP_TRACE_H */
