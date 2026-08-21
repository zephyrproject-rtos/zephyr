/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Object: the base that every mpipe type embeds.
 * @ingroup mpipe_object
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_OBJECT_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_OBJECT_H_

/**
 * @defgroup mpipe_object Objects
 * @ingroup mpipe_framework
 * @brief The common base that gives every type an identity and properties.
 *
 * Every mpipe type embeds an @ref mpipe_object as its first member, which is
 * what makes upcasting a plain C cast. The base carries what the framework
 * needs of anything it holds: an id, a pointer to the container that holds it,
 * a list node so a container can keep its children on one list, and a flag
 * field a walker reads to decide whether it may descend.
 *
 * It also carries the property callbacks. Properties are how an element is
 * configured without the caller knowing its concrete type - a file path, a
 * device, a queue depth - and they are reached through
 * @ref mpipe_object_set_properties and @ref mpipe_object_get_properties rather
 * than by calling the callbacks directly.
 *
 * A property is not a capability. A property configures one element and
 * nothing else has to agree on it; a capability describes the data crossing a
 * link, so both ends must agree before anything can flow. See
 * @ref mpipe_structure.
 *
 * @{
 */

#include <stdint.h>

#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util_macro.h>

/** Base flag of the object */
#define MPIPE_OBJECT_FLAG_BASE BIT(0)
/**
 * @brief The object is a @ref mpipe_bin and holds children of its own.
 *
 * Set by mpipe_bin_init(). This is to tell a walker that an element can be
 * descended into.
 */
#define MPIPE_OBJECT_FLAG_BIN  BIT(1)

/** Sentinel value to mark the end of property lists */
#define MPIPE_PROP_LIST_END -1

/**
 * @brief Base object structure
 *
 * This structure defines the common fields and interface for all objects
 * in the mpipe object system: identity, list membership, and property access.
 */
struct mpipe_object {
	/** Parent element that contains this object */
	struct mpipe_object *container;
	/** Unique ID given to an object instance. The max value (UINT8_MAX) is reserved and
	 * should not be used
	 */
	uint8_t id;
	/** Flags of the object, an inheritable bitfield */
	uint32_t flags;
	/** Object node to be used in a linked list */
	sys_dnode_t node;
	/** Function to set property */
	int (*set_property)(struct mpipe_object *self, uint32_t key, const void *val);
	/** Function to get property */
	int (*get_property)(struct mpipe_object *self, uint32_t key, void *val);
};

/**
 * @brief Initialize all fields of mpipe_object to zero.
 *
 * Must be called first when initializing any structure that embeds mpipe_object.
 *
 * @param obj Pointer to the mpipe_object to initialize.
 */
void mpipe_object_init(struct mpipe_object *obj);

/**
 * @brief Set multiple properties of an mpipe_object.
 *
 * This function sets one or more properties on the given object.
 * The arguments must be provided as a sequence of {key, value} pairs, terminated by
 * MPIPE_PROP_LIST_END.
 *
 * Example usage:
 *
 * @code
 * mpipe_object_set_properties(obj, "key1", val1, "key2", val2, MPIPE_PROP_LIST_END);
 * @endcode
 *
 * @param obj Pointer to a @ref mpipe_object.
 * @param ... A variable list of {uint32_t key, const void *val} pairs, terminated by
 * MPIPE_PROP_LIST_END.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP The object exposes no property setter
 * @retval -errno the first failing setter's error, leaving the remaining pairs
 *         unapplied
 */
int mpipe_object_set_properties(struct mpipe_object *obj, ...);

/**
 * @brief Get multiple properties' values of an mpipe_object.
 *
 * This function gets one or more properties' values of the given object.
 * The arguments must be provided as a sequence of {key, value} pairs, terminated by
 * MPIPE_PROP_LIST_END.
 *
 * Example usage:
 *
 * @code
 * mpipe_object_get_properties(obj, "key1", val1, "key2", val2, MPIPE_PROP_LIST_END);
 * @endcode
 *
 * @param obj Pointer to a @ref mpipe_object.
 * @param ... A variable list of {uint32_t key, void *val} pairs, terminated by MPIPE_PROP_LIST_END.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP The object exposes no property getter
 * @retval -errno the first failing getter's error, leaving the remaining pairs
 *         unread
 */
int mpipe_object_get_properties(struct mpipe_object *obj, ...);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_OBJECT_H_ */
