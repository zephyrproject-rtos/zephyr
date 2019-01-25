/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief APIs used when examining the objects in a debug tracing list.
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_OBJECT_TRACING_H_
#define ZEPHYR_INCLUDE_DEBUG_OBJECT_TRACING_H_

#ifdef CONFIG_OBJECT_TRACING

#include <kernel.h>
extern struct k_timer    *_trace_list_k_timer;
extern struct k_mem_slab *_trace_list_k_mem_slab;
extern struct k_mem_pool *_trace_list_k_mem_pool;
extern struct k_sem      *_trace_list_k_sem;
extern struct k_mutex    *_trace_list_k_mutex;
extern struct k_fifo     *_trace_list_k_fifo;
extern struct k_lifo     *_trace_list_k_lifo;
extern struct k_stack    *_trace_list_k_stack;
extern struct k_msgq     *_trace_list_k_msgq;
extern struct k_mbox     *_trace_list_k_mbox;
extern struct k_pipe     *_trace_list_k_pipe;
extern struct k_queue	 *_trace_list_k_queue;

/**
 * @def SYS_TRACING_HEAD
 *
 * @brief Head element of a trace list.
 *
 * @details Access the head element of a given trace list.
 *
 * @param type Data type of the trace list to get the head from.
 * @param name Name of the trace list to get the head from.
 */
#define SYS_TRACING_HEAD(type, name) ((type *) _CONCAT(_trace_list_, name))

/**
 * @def SYS_TRACING_NEXT
 *
 * @brief Gets a node's next element.
 *
 * @details Given a node in a trace list, gets the next element
 * in the list.
 *
 * @param type Data type of the trace list
 * @param name Name of the trace list to get the head from.
 * @param obj  Object to get next element from.
 */
#define SYS_TRACING_NEXT(type, name, obj) (((type *)obj)->__next)

#endif /*CONFIG_OBJECT_TRACING*/

#ifdef CONFIG_THREAD_MONITOR

#include <kernel_structs.h>

/**
 * @def SYS_THREAD_MONITOR_HEAD
 *
 * @brief Head element of the thread monitor list.
 *
 * @details Access the head element of the thread monitor list.
 *
 */
#define SYS_THREAD_MONITOR_HEAD ((struct k_thread *)(_kernel.threads))

/**
 * @def SYS_THREAD_MONITOR_NEXT
 *
 * @brief Gets a thread node's next element.
 *
 * @details Given a node in a thread monitor list, gets the next
 * element in the list.
 *
 * @param obj Object to get the next element from.
 */
#define SYS_THREAD_MONITOR_NEXT(obj) (((struct k_thread *)obj)->next_thread)

#endif /*CONFIG_THREAD_MONITOR*/

#endif /*ZEPHYR_INCLUDE_DEBUG_OBJECT_TRACING_H_*/
