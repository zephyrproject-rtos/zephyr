/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Capabilities header file.
 */

#ifndef __MP_CAPS_H__
#define __MP_CAPS_H__

#include <stddef.h>

#include "mp_object.h"
#include "mp_structure.h"

/**
 * @defgroup MpCaps
 * @brief Media Capabilities
 *
 * CAPS is an object represents the supported media formats (e.g., audio, video) and
 * data formats (e.g., codec, resolution, framerate) of an element. Each caps
 * object consists of one or more @ref MpCapStructure, where each of them describes
 * a specific capability.
 *
 * @{
 */

/**
 * @struct MpCaps
 *
 * @brief Represents a list of media capabilities.
 *
 */
typedef struct {
	MpObject object;             /**< Base object */
	sys_slist_t caps_structures; /**< List of capability structures */
} MpCaps;

/**
 * @struct MpCapStructure
 * @brief structure used to hold a single capability structure.
 *
 * Each caps structure has:
 * - A media type: Specifies the nature (e.g., video, audio) and format (e.g., raw, compressed) of
the media.
 * - A set of field–value pairs: Each pair represents a specific capability of the element.
 */
typedef struct {
	sys_snode_t node;       /**< Linked list node */
	MpStructure *structure; /**< Pointer to the capability structure */
} MpCapStructure;

/** @brief Flag indicating ANY caps type */
#define MP_CAPS_FLAG_ANY 0x1

/** @brief Cast a generic object to MpCaps */
#define MP_CAPS(obj) ((MpCaps *)(obj))

/**
 * @brief Create new @ref MpCaps of one caps structure with a media type and fields in the same way
 * as the @ref mp_structure_new
 *
 * @param media_type Media type string
 * @param ... Variadic list of field definitions:
 *            (const char *field_name, int field_type, void *field_value),
 *            terminated by NULL
 *
 * @return Pointer to newly created @ref MpCaps, or NULL on failure
 */
MpCaps *mp_caps_new(const char *media_type, ...);

/**
 * @brief Create new @ref MpCaps of type ANY.
 *
 * @return Pointer to newly created @ref MpCaps
 */
MpCaps *mp_caps_new_any(void);

/**
 * @brief Initialize a @ref MpCaps.
 *
 * @param caps Pointer to @ref MpCaps to initialize
 * @param flag Initialization flags (i.e., 0 if no flag or MP_CAPS_FLAG_ANY)
 */
void mp_caps_init(MpCaps *caps, uint32_t flag);

/**
 * @brief Append a structure to the @ref MpCaps.
 *
 * @param caps Pointer to @ref MpCaps
 * @param structure Pointer to @ref MpStructure to append
 *
 * @return true if the structure was successfully appended, false otherwise
 */
bool mp_caps_append(MpCaps *caps, MpStructure *structure);

/**
 * @brief Retrieve a @ref MpStructure from @ref Mpcaps by index.
 *
 * @param caps Pointer to @ref MpCaps
 * @param index Index of the structure to retrieve
 *
 * @return Pointer to @ref MpStructure if found, NULL otherwise
 */
MpStructure *mp_caps_get_structure(MpCaps *caps, int index);

/**
 * @brief Check if the @ref MpCaps is ANY.
 *
 * @param caps Pointer to @ref MpCaps
 *
 * @return true if @ref MpCaps type is ANY, false otherwise
 */
bool mp_caps_is_any(MpCaps *caps);

/**
 * @brief Check if the @ref MpCaps are empty
 *
 * @param caps Pointer to @ref MpCaps
 *
 * @return true if caps is empty, false otherwise
 */
bool mp_caps_is_empty(MpCaps *caps);

/**
 * @brief Check if the @ref MpCaps are fixed
 *
 * @param caps Pointer to @ref MpCaps
 *
 * @return true if caps is fixed, false otherwise
 */
bool mp_caps_is_fixed(MpCaps *caps);

/**
 * @brief Check if two @ref MpCaps can intersect.
 *
 * Two caps can intersect if there exists at least one capability structure
 * in the first caps that can be intersected with a structure in the second caps.
 *
 * @param caps1 Pointer to first @ref MpCaps
 * @param caps2 Pointer to second @ref MpCaps
 *
 * @return true if the two caps can intersect, false otherwise
 */
bool mp_caps_can_intersect(MpCaps *caps1, MpCaps *caps2);

/**
 * @brief Perform intersection of two @ref MpCaps
 *
 * The intersection is performed by comparing each caps structure in
 * @p caps1 with those in @p caps2. Only structures with compatible fields
 * will be included in the resulting caps.
 *
 * @param caps1 Pointer to first @ref MpCaps
 * @param caps2 Pointer to second @ref MpCaps
 *
 * @return Pointer to intersected @ref MpCaps, or NULL if no intersection
 */
MpCaps *mp_caps_intersect(MpCaps *caps1, MpCaps *caps2);

/**
 * @brief Fixate @ref MpCaps.
 *
 * @param caps Pointer to @ref MpCaps to fixate
 *
 * @return Pointer to new fixated @ref MpCaps, or NULL if caps is ANY or EMPTY
 */
MpCaps *mp_caps_fixate(MpCaps *caps);

/**
 * @brief Duplicate a @ref MpCaps object.
 *
 * @param caps Pointer to @ref MpCaps to duplicate
 *
 * @return Pointer to new duplicated @ref MpCaps
 */
MpCaps *mp_caps_duplicate(MpCaps *caps);

/**
 * @brief Print @ref MpCaps to the console.
 *
 * @param caps Pointer to @ref MpCaps to print
 */
void mp_caps_print(MpCaps *caps);

/**
 * @brief Create a new reference to a @ref MpCaps object.
 *
 * @param caps Pointer to @ref MpCaps
 *
 * @return Pointer to the same @ref MpCaps with increased reference count
 */
static inline MpCaps *mp_caps_ref(MpCaps *caps)
{
	return (MpCaps *)mp_object_ref(&caps->object);
}

/**
 * @brief Release a reference to a @ref MpCaps object.
 *
 * @param caps Pointer to @ref MpCaps
 */
static inline void mp_caps_unref(MpCaps *caps)
{
	mp_object_unref(&caps->object);
}

/**
 * @brief Replace a @ref MpCaps pointer with a new one.
 *
 * @param target_caps Pointer to the target @ref MpCaps pointer
 * @param new_caps Pointer to the new @ref MpCaps object
 */
void mp_caps_replace(MpCaps **target_caps, MpCaps *new_caps);

/** @} */

#endif /* __MP_CAPS_H__ */
