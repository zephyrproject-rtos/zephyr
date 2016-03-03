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
 * @brief Kernel object tracing common structures.
 */

#ifndef _OBJECT_TRACING_COMMON_H_
#define _OBJECT_TRACING_COMMON_H_

#ifndef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS

#define SYS_TRACING_OBJ_INIT(name, obj) do { } while ((0))

#else

/**
 * @def SYS_TRACING_OBJ_INIT
 *
 * @brief Adds a new object into the trace list
 *
 * @details The object is added for tracing into
 * a trace list. This is usually called at the
 * moment of object initialization.
 *
 * @param name Name of the trace list.
 * @param obj Object to be added in the trace list.
 */
#define SYS_TRACING_OBJ_INIT(name, obj)	\
	do {					\
		unsigned int key;		\
						\
		key = irq_lock();		\
		(obj)->__next =  _trace_list_##name;\
		_trace_list_##name = obj;       \
		irq_unlock(key);		\
	}					\
	while (0)

/*
 * Lists for object tracing.
 */
#include <nanokernel.h>
struct nano_fifo  *_trace_list_nano_fifo;
struct nano_lifo  *_trace_list_nano_lifo;
struct nano_sem   *_trace_list_nano_sem;
struct nano_timer *_trace_list_nano_timer;
struct nano_stack *_trace_list_nano_stack;

#ifdef CONFIG_MICROKERNEL
#include <microkernel/base_api.h>
struct _k_mbox_struct  *_trace_list_micro_mbox;
struct _k_mutex_struct *_trace_list_micro_mutex;
struct _k_sem_struct   *_trace_list_micro_sem;
struct _k_fifo_struct  *_trace_list_micro_fifo;
struct _k_pipe_struct  *_trace_list_micro_pipe;
struct pool_struct     *_trace_list_micro_mem_pool;
struct _k_mem_map_struct *_trace_list_micro_mem_map;
#endif /*CONFIG_MICROKERNEL*/


#endif /*CONFIG_DEBUG_TRACING_KERNEL_OBJECTS*/
#endif /*_OBJECT_TRACING_COMMON_H_*/
