/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_object.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_OBJECT_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_OBJECT_H_

/**
 * @defgroup mp_object Objects
 * @brief Reference-counted base object APIs.
 * @ingroup mp_core
 * @{
 */

#include <stdint.h>

#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util_macro.h>

/** Base flag of the object */
#define OBJECT_FLAG_BASE BIT(0)

/**
 * @brief Base object structure
 *
 * This structure defines the common fields and interface for all objects
 * in the MP object system. It provides reference counting, property access,
 * and lifecycle management functionality.
 */
struct mp_object {
	/** Parent element that contains this object */
	struct mp_object *container;
	/** Reference counter */
	atomic_t ref;
	/** Unique ID given to an object instance. The max value (UINT8_MAX) is reserved and
	 * should not be used
	 */
	uint8_t id;
	/** Flags of the object, bitfield inheritable */
	uint32_t flags;
	/** Object node to be used in a linked list */
	sys_dnode_t node;
	/** Function to set property */
	int (*set_property)(struct mp_object *self, uint32_t key, const void *val);
	/** Function to get property */
	int (*get_property)(struct mp_object *self, uint32_t key, void *val);
	/** Function to free the resource using by the object */
	void (*release)(struct mp_object *obj);
};

/**
 * Increase the reference counter of an object
 *
 * @param obj Pointer to the object.
 * @return A new pointer to the object with its reference count increased.
 */
struct mp_object *mp_object_ref(struct mp_object *obj);

/**
 * Decrease the reference counter of an object.
 *
 * If the reference count reaches zero, the object will be released.
 *
 * @param obj Pointer to the object to unreference.
 */
void mp_object_unref(struct mp_object *obj);

/**
 * Replace the pointer to an object with a new object.
 *
 *
 * @param ptr Pointer to the object pointer to be replaced.
 * @param new_obj Pointer to the new object.
 */
void mp_object_replace(struct mp_object **ptr, struct mp_object *new_obj);

/**
 * @brief Set multiple properties of an mp_object.
 *
 * This function sets one or more properties on the given object.
 * The arguments must be provided as a sequence of {key, value} pairs, terminated by PROP_LIST_END.
 *
 * Example usage:
 *
 * @code
 * mp_object_set_properties(obj, "key1", val1, "key2", val2, PROP_LIST_END);
 * @endcode
 *
 * @param obj Pointer to a @ref mp_object.
 * @param ... A variable list of {uint32_t key, const void *val} pairs, terminated by PROP_LIST_END.
 *
 */
int mp_object_set_properties(struct mp_object *obj, ...);

/**
 * @brief Get multiple properties' values of an mp_object.
 *
 * This function gets one or more properties' values of the given object.
 * The arguments must be provided as a sequence of {key, value} pairs, terminated by PROP_LIST_END.
 *
 * Example usage:
 *
 * @code
 * mp_object_get_properties(obj, "key1", val1, "key2", val2, PROP_LIST_END);
 * @endcode
 *
 * @param obj Pointer to a @ref mp_object.
 * @param ... A variable list of {uint32_t key, void *val} pairs, terminated by PROP_LIST_END.
 *
 */
int mp_object_get_properties(struct mp_object *obj, ...);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_OBJECT_H_ */
