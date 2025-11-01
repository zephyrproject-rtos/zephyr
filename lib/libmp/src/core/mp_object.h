/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpObject.
 */

#ifndef __MP_OBJECT_H__
#define __MP_OBJECT_H__

#include <stdint.h>

#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/dlist.h>

typedef struct _MpObject MpObject;

#define MP_OBJECT(object) ((MpObject *)object)

/** Base flag of the object */
#define OBJECT_FLAG_BASE BIT(0)

/**
 * @brief Base object structure
 *
 * This structure defines the common fields and interface for all objects
 * in the MP object system. It provides reference counting, property access,
 * and lifecycle management functionality.
 */
struct _MpObject {
	/** Object that contains this object */
	MpObject *container;
	/** Reference counter */
	atomic_t ref;
	/** Name of the object */
	const char *name;
	/** Flags of the object, bitfield inheritable */
	uint32_t flags;
	/** Object node to be used in a linked list */
	sys_dnode_t node;
	/** Function to set property */
	int (*set_property)(MpObject *self, uint32_t key, const void *val);
	/** Function to get property */
	int (*get_property)(MpObject *self, uint32_t key, void *val);
	/** Function to free the ressource using by the object */
	void (*release)(MpObject *obj);
};

/**
 * Increase the reference counter of an object
 *
 * @param obj Pointer to the object.
 * @return A new pointer to the object with its reference count increased.
 */
MpObject *mp_object_ref(MpObject *obj);

/**
 * Decrease the reference counter of an object.
 *
 * If the reference count reaches zero, the object will be released.
 *
 * @param obj Pointer to the object to unreference.
 */
void mp_object_unref(MpObject *obj);

/**
 * Replace the pointer to an object with a new object.
 *
 *
 * @param ptr Pointer to the object pointer to be replaced.
 * @param new_obj Pointer to the new object.
 */
void mp_object_replace(MpObject **ptr, MpObject *new_obj);

/**
 * @brief Set multiple properties of an MpObject.
 *
 * This function sets one or more properties on the given object.
 * The arguments must be provided as a sequence of {key, value} pairs, terminated by NULL.
 *
 * Example usage:
 *
 * @code
 * mp_object_set_properties(obj, "key1", val1, "key2", val2, NULL);
 * @endcode
 *
 * @param obj Pointer to a @ref MpObject.
 * @param ... A variable list of {uint32_t key, const void *val} pairs, terminated by NULL.
 *
 */
int mp_object_set_properties(MpObject *obj, ...);

/**
 * @brief Get multiple properties' values of an MpObject.
 *
 * This function gets one or more properties' values of the given object.
 * The arguments must be provided as a sequence of {key, value} pairs, terminated by NULL.
 *
 * Example usage:
 *
 * @code
 * mp_object_get_properties(obj, "key1", val1, "key2", val2, NULL);
 * @endcode
 *
 * @param obj Pointer to a @ref MpObject.
 * @param ... A variable list of {uint32_t key, void *val} pairs, terminated by NULL.
 *
 */
int mp_object_get_properties(MpObject *obj, ...);

#endif /* __MP_OBJECT_H__ */
