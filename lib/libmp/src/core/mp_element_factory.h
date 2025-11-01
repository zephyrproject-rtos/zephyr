/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpElementFactory.
 */

#ifndef __MP_ELEMENT_FACTORY_H__
#define __MP_ELEMENT_FACTORY_H__

#include <stddef.h>

typedef struct _MpElement MpElement;

/**
 * @brief Element Factory structure
 *
 * Defines a factory for creating specific types of elements.
 */
typedef struct _MpElementFactory {
	/** Factory name identifier */
	const char *name;
	/** Size in bytes of the element structure */
	size_t size;
	/** Initialization function for the element */
	void (*init)(MpElement *);
} MpElementFactory;

/**
 * @brief Define and register an element factory
 *
 * This macro creates and registers an element factory.
 *
 * @param fname Factory name (will be stringified)
 * @param sz Size of the element structure in bytes
 * @param initfunc Initialization function pointer
 *
 */
#define MP_ELEMENTFACTORY_DEFINE(fname, sz, initfunc)                                              \
	static const STRUCT_SECTION_ITERABLE(_MpElementFactory, fname) = {                         \
		.name = STRINGIFY(fname), .size = sz, .init = initfunc,                            \
	}

/**
 * @brief Create a new element
 *
 * Creates and initializes a new element instance using the specified factory.
 * The element is allocated dynamically and must be freed when no longer needed.
 *
 * @param fname Factory name to use for element creation
 * @param ename Instance name for the created element
 *
 * @return Pointer to the created @ref MpElement, or NULL if factory not found
 *         or allocation failed
 *
 * @note The caller is responsible for freeing the returned element when done
 */
MpElement *mp_element_factory_create(const char *fname, const char *ename);

#endif /* __MP_ELEMENT_FACTORY_H__ */
