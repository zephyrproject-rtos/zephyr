/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Structure header file.
 */

#ifndef __MP_STRUCTURE_H__
#define __MP_STRUCTURE_H__

#include <zephyr/sys/slist.h>

#include "mp_value.h"

/**
 * @defgroup MpStructure
 * @brief Dynamic structure for holding named fields and values.
 *
 *
 * MpStructure is a flexible container used to represent a set of named fields,
 * which is associated with a @ref MpValue.
 *
 * Each field must have an unique name, each field is stored in single linked list.
 * The structure can be modified at runtime by appending, removing fields.
 *
 * Example an structure :
 * 	video/x-raw, format=RGB565, width=1920, height=1080, framerate=30/1
 *
 * @code{.c}
 *
 *  MpStructure *structure = mp_structure_new("video/x-raw",
 * "format", MP_TYPE_UINT, MP_PIXEL_FORMAT_RGB565,
 * "width", MP_TYPE_INT, 1280,
 * "height", MP_TYPE_INT, 720,
 * "framerate", 30, 1, NULL);
 *
 * @endcode
 *
 * Structure supports also int range, fraction range and list
 *
 * Example of structure with range and list:
 * video/x-dummy, fieldA={RGB565,XRGB}, fieldB=[720, 1080, 720], fieldC=[960, 1920, 960],
 * fieldD=[30/1, 60,1, 15/1]
 *
 * To create a new structure this one structure structure:
 * @code{.c}
 *
 * MpValue *list = mp_value_new(MP_TYPE_LIST, NULL);
 * mp_value_list_append(list, mp_value_new(MP_TYPE_UINT, MP_PIXEL_FORMAT_RGB565, NULL));
 * mp_value_list_append(list, mp_value_new(MP_TYPE_UINT, MP_PIXEL_FORMAT_XRGB32, NULL));
 *
 * MpStructure *structure = mp_structure_new("video/x-dummy",
 *     "fieldA", MP_TYPE_LIST, list,
 *     "fieldB", MP_TYPE_RANGE_INT, 720, 1080, 720,
 *     "fieldC", MP_TYPE_RANGE_INT, 960, 1920, 960,
 *     "fieldD", MP_TYPE_FRACTION_RANGE, 30, 1, 60, 1, 15, 1, NULL);
 *
 * @endcode
 *
 * Intersection rules for structures:
 * Two @ref MpStructure instances can intersect if all common fields between them
 * have intersecting values. The intersection operation produces a new structure
 * containing only the compatible fields and values.
 *
 * @{
 */

/**
 * @struct MpStructure
 * @brief Dynamic structure for holding named fields and values.
 */
typedef struct {
	/** List of fields in the structure */
	sys_slist_t fields;
	/** Optional name of the structure (can be NULL) */
	const char *name;
} MpStructure;

/**
 * @brief Create a new MpStructure.
 *
 * This function creates a new structure with the specified name and a variadic
 * list of field definitions. Each field is defined by a sequence of arguments:
 * - `const char *field_name`
 * - `int type`
 * - One or more values depending on the type
 *
 * The list must be terminated by a `NULL` field name. The number and type of
 * arguments for each field depend on the field's type with the same rule as @ref mp_value_new()),
 * except for the MP_TYPE_LIST which requires one arguement which is a pre-created MpValue list.
 *
 * @param name Name of the structure. If NULL, the structure will be unnamed.
 * @param ... Variadic list of field definitions, terminated by NULL.
 *
 * @return Pointer to the newly created @ref MpStructure.
 * @retval NULL If memory allocation fails or the argument list is invalid.
 */
MpStructure *mp_structure_new(const char *name, ...);

/**
 * @brief Initialize an @ref MpStructure.
 *
 * @param structure Structure to initialize.
 * @param name Name of the structure. Can be NULL for unnamed structures.
 */
void mp_structure_init(MpStructure *structure, const char *name);

/**
 * @brief Append a field to an @ref MpStructure
 *
 * @param structure Structure to append the field to.
 * @param name Field name (field name must be unique)
 * @param value Field value (the value will be copied)
 */
void mp_structure_append(MpStructure *structure, const char *name, MpValue *value);

/**
 * @brief Clear all fields from an @ref MpStructure.
 *
 * @param structure Structure to clear.
 */
void mp_structure_clear(MpStructure *structure);

/**
 * @brief Destroy an @ref MpStructure.
 *
 * Clears all fields and releases resources associated with the structure.
 *
 * @param structure Pointer to the structure to destroy.
 */
void mp_structure_destroy(MpStructure *structure);

/**
 * @brief Check if an @ref MpStructure is fixed.
 *
 * A structure is considered fixed if all its fields contain single values.
 *
 * @param structure Structure to check.
 *
 * @return True if the structure is fixed, false otherwise.
 */
bool mp_structure_is_fixed(MpStructure *structure);

/**
 * @brief Get the value of a field in an @ref MpStructure.
 *
 * Retrieves the value associated with the specified field name.
 *
 * @param structure Structure containing the field.
 * @param string Name of the field.
 *
 * @return Pointer to the value of the field, NULL if the field was not found.
 */
MpValue *mp_structure_get_value(MpStructure *structure, const char *string);

/**
 * @brief Remove a field from an @ref MpStructure.
 *
 * Deletes the field with the specified name from the structure.
 *
 * @param structure Structure containing the field to remove.
 * @param name Name of the field to remove.
 *
 * @return True if the field was found and removed, false otherwise.
 */
bool mp_structure_remove_field(MpStructure *structure, const char *name);

/**
 * @brief Check if two @ref MpStructures can intersect.
 *
 * Two structures can intersect if all common fields have intersecting values.
 *
 * @param struct1 First structure.
 * @param struct2 Second structure.
 *
 * @return True if the structures can intersect, false otherwise.
 */
bool mp_structure_can_intersect(MpStructure *struct1, MpStructure *struct2);

/**
 * @brief Create a new intersected @ref MpStructure.
 *
 * @param structure1 First structure.
 * @param structure2 Second structure.
 *
 * @return Pointer to the new intersected structure.
 */
MpStructure *mp_structure_intersect(MpStructure *structure1, MpStructure *structure2);

/**
 * @brief Create a new fixated @ref MpStructure.
 *
 * Generates a new structure containing only single values from the input structure.
 *
 * @param src Structure to fixate.
 *
 * @return Pointer to the new fixated structure.
 */
MpStructure *mp_structure_fixate(MpStructure *src);

/**
 * @brief Duplicate an @ref MpStructure.
 *
 * @param src Structure to duplicate.
 *
 * @return Pointer to the duplicated structure.
 */
MpStructure *mp_structure_duplicate(MpStructure *src);

/**
 * @brief Print an @ref MpStructure.
 *
 * Outputs the contents of the structure for debugging or inspection.
 *
 * @param structure Structure to print.
 */
void mp_structure_print(MpStructure *structure);

/** @} */

#endif /*__MP_STRUCTURE_H__*/
