/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_element_factory.
 */

#ifndef __MP_ELEMENT_FACTORY_H__
#define __MP_ELEMENT_FACTORY_H__

#include <stddef.h>
#include <stdint.h>

#include "mp_plugin.h"

struct mp_element;

/**
 * @brief Element Factory structure
 *
 * Defines a factory for creating specific types of elements.
 */
struct mp_element_factory {
	/** Unique ID of the element factory */
	enum mp_element_factory_id id;
	/** Size in bytes of the element structure */
	size_t size;
	/** Initialization function for the element */
	void (*init)(struct mp_element *element);
};

/**
 * @brief Define and register an element factory
 *
 * This macro creates and registers an element factory.
 *
 * @param eid Unique ID of the element factory, see @ref mp_element_factory_id
 * @param sz Size of the element structure in bytes
 * @param init_func Initialization function pointer
 *
 */
#define MP_ELEMENT_FACTORY_DEFINE(eid, sz, init_func)                                              \
	static const STRUCT_SECTION_ITERABLE(mp_element_factory, eid##_FACTORY) = {                \
		.id = eid,                                                                         \
		.size = sz,                                                                        \
		.init = init_func,                                                                 \
	}

/**
 * @brief Create a new element
 *
 * Creates and initializes a new element instance using the specified factory.
 * The element is allocated dynamically and must be freed when no longer needed.
 *
 * @param eid Element factory ID from which element is created, see @ref mp_element_factory_id
 * @param id Unique instance ID given to the created element
 *
 * @return Pointer to the created @ref mp_element, or NULL if the factory not found
 *         or allocation failed
 *
 * @note The caller is responsible for freeing the returned element when done
 */
struct mp_element *mp_element_factory_create(enum mp_element_factory_id eid, uint8_t id);

#endif /* __MP_ELEMENT_FACTORY_H__ */
