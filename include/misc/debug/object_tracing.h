/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Kernel object tracing support.
 */

#ifndef _OBJECT_TRACING_H_
#define _OBJECT_TRACING_H_

#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
#include <nanokernel.h>
extern struct nano_fifo  *_trace_list_nano_fifo;
extern struct nano_lifo  *_trace_list_nano_lifo;
extern struct nano_sem   *_trace_list_nano_sem;
extern struct nano_timer *_trace_list_nano_timer;
extern struct nano_stack *_trace_list_nano_stack;
extern struct ring_buf *_trace_list_sys_ring_buf;


#ifdef CONFIG_MICROKERNEL
#include <microkernel/base_api.h>
extern struct _k_mbox_struct  *_trace_list_micro_mbox;
extern struct _k_mutex_struct *_trace_list_micro_mutex;
extern struct _k_sem_struct   *_trace_list_micro_sem;
extern struct _k_fifo_struct  *_trace_list_micro_fifo;
extern struct _k_pipe_struct  *_trace_list_micro_pipe;
extern struct pool_struct     *_trace_list_micro_mem_pool;
extern struct _k_mem_map_struct *_trace_list_micro_mem_map;
extern struct _k_event_struct *_trace_list_micro_event;
#endif /*CONFIG_MICROKERNEL*/

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

#endif /*CONFIG_DEBUG_TRACING_KERNEL_OBJECTS*/

#ifdef CONFIG_THREAD_MONITOR

#include <nano_private.h>

/**
 * @def SYS_THREAD_MONITOR_HEAD
 *
 * @brief Head element of the thread monitor list.
 *
 * @details Access the head element of the thread monitor list.
 *
 */
#define SYS_THREAD_MONITOR_HEAD ((struct tcs *)(_nanokernel.threads))

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
#define SYS_THREAD_MONITOR_NEXT(obj) (((struct tcs *)obj)->next_thread)

#endif /*CONFIG_THREAD_MONITOR*/

#endif /*_OBJECT_TRACING_H_*/
