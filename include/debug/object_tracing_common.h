/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief APIs used to add or remove an object in a debug tracing list.
 */

#ifndef _OBJECT_TRACING_COMMON_H_
#define _OBJECT_TRACING_COMMON_H_

#ifndef CONFIG_OBJECT_TRACING

#define SYS_TRACING_OBJ_INIT(name, obj) do { } while ((0))
#define SYS_TRACING_OBJ_INIT_DLL(name, obj) do { } while ((0))
#define SYS_TRACING_OBJ_REMOVE_DLL(name, obj) do { } while ((0))

#else

/**
 * @def SYS_TRACING_OBJ_INIT
 *
 * @brief Adds a new object into the trace list
 *
 * @details The object is added for tracing into a trace list. This is usually
 * called at the moment of object initialization.
 *
 * @param name Name of the trace list.
 * @param obj Object to be added in the trace list.
 */
#define SYS_TRACING_OBJ_INIT(name, obj)		       \
	do {					       \
		unsigned int key;		       \
						       \
		key = irq_lock();		       \
		(obj)->__next =  _trace_list_ ## name; \
		_trace_list_ ## name = obj;	       \
		irq_unlock(key);		       \
	}					       \
	while (0)

/**
 * @def SYS_TRACING_OBJ_INIT_DLL
 *
 * @brief Adds a new object into the trace list as a double linked list.
 *
 * @details The object is added for tracing into a trace list. This is usually
 * called at the moment of object initialization. This list is used for objects
 * that can be removed from the tracing list dynamically.
 *
 * @param name Name of the trace list.
 * @param obj Object to be added in the trace list.
 */
#define SYS_TRACING_OBJ_INIT_DLL(name, obj)		      \
	do {						      \
		unsigned int key;			      \
							      \
		key = irq_lock();			      \
		if (_trace_list_ ## name) {		      \
			_trace_list_ ## name->__prev = (obj); \
		}					      \
		(obj)->__next = _trace_list_ ## name;	      \
		(obj)->__prev = NULL;			      \
		_trace_list_ ## name = obj;		      \
		irq_unlock(key);			      \
	}						      \
	while (0)

/**
 * @def SYS_TRACING_OBJ_REMOVE_DLL
 *
 * @brief Removes an object from a double linked trace list.
 *
 * @details The object is remove from the trace list.
 * It needs to be used with DEBUG_TRACING_OBJ_INIT_DLL as a pair.
 *
 * @param name Name of the trace list.
 * @param obj Object to be removed from the trace list.
 */
#define SYS_TRACING_OBJ_REMOVE_DLL(name, obj)		      \
	do {						      \
		unsigned int key;			      \
							      \
		key = irq_lock();			      \
		if (obj->__next) {			      \
			obj->__next->__prev = (obj)->__prev;  \
		}					      \
		if (obj->__prev) {			      \
			obj->__prev->__next = (obj)->__next;  \
		} else {				      \
			_trace_list_ ## name = (obj)->__next; \
		}					      \
		irq_unlock(key);			      \
	}						      \
	while (0)

struct ring_buf;

extern struct ring_buf   *_trace_list_sys_ring_buf;

#endif  /*CONFIG_OBJECT_TRACING*/
#endif  /*_OBJECT_TRACING_COMMON_H_*/
