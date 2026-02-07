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
 * @defgroup mp_structure
 * @brief Dynamic structure for holding named fields and values.
 *
 *
 * mp_structure is a flexible container used to represent a set of named fields,
 * which is associated with a @ref struct mp_value.
 *
 * Each field must have an unique ID, each field is stored in single linked list.
 * The structure can be modified at runtime by appending, removing fields.
 *
 * Example an structure :
 *	video/x-raw, format=RGB565, width=1920, height=1080, framerate=30/1
 *
 * @code{.c}
 *
 *  struct mp_structure *structure = mp_structure_new(MEDIA_TYPE_VIDEO_RAW,
 * MP_CAPS_FORMAT, MP_TYPE_UINT, MP_PIXEL_FORMAT_RGB565,
 * MP_CAPS_WIDTH, MP_TYPE_INT, 1280,
 * MP_CAPS_HEIGHT, MP_TYPE_INT, 720,
 * MP_CAPS_FRAMERATE, 30, 1, MP_CAPS_END);
 *
 * @endcode
 *
 * Structure supports also int range, fraction range and list
 *
 * Example of structure with range and list:
 * video/x-dummy, format={MP_PIXEL_FORMAT_RGB565, MP_PIXEL_FORMAT_XRGB}, width=[720, 1080,
 * 720], height=[960, 1920, 960], framerate=[30/1, 60,1, 15/1]
 *
 * To create a new structure this one structure structure:
 * @code{.c}
 *
 * struct mp_value *list = mp_value_new(MP_TYPE_LIST, NULL);
 * mp_value_list_append(list, mp_value_new(MP_TYPE_UINT, MP_PIXEL_FORMAT_RGB565, NULL));
 * mp_value_list_append(list, mp_value_new(MP_TYPE_UINT, MP_PIXEL_FORMAT_XRGB32, NULL));
 *
 * struct mp_structure *structure = mp_structure_new(MEDIA_TYPE_VIDEO_RAW,
 *     MP_CAPS_FORMAT, MP_TYPE_LIST, list,
 *     MP_CAPS_WIDTH, MP_TYPE_RANGE_INT, 720, 1080, 720,
 *     MP_CAPS_HEIGHT, MP_TYPE_RANGE_INT, 960, 1920, 960,
 *     MP_CAPS_FRAMERATE, MP_TYPE_FRACTION_RANGE, 30, 1, 60, 1, 15, 1, MP_CAPS_END);
 *
 * @endcode
 *
 * Intersection rules for structures:
 * Two @ref struct mp_structure instances can intersect if all common fields between them
 * have intersecting values. The intersection operation produces a new structure
 * containing only the compatible fields and values.
 *
 * @{
 */

/**
 * @struct mp_structure
 * @brief Dynamic structure for holding named fields and values.
 */
struct mp_structure {
	/** List of fields in the structure */
	sys_slist_t fields;
	/** Media type ID of the structure */
	uint8_t media_type_id;
};

/**
 * @brief Create a new mp_structure.
 *
 * This function creates a new structure with the specified media type ID and a variadic
 * list of field definitions. Each field is defined by a sequence of arguments:
 * - `uint8_t field_id`
 * - `int type`
 * - One or more values depending on the type
 *
 * The list must be terminated by a `0` field ID. The number and type of
 * arguments for each field depend on the field's type with the same rule as @ref mp_value_new()),
 * except for the MP_TYPE_LIST which requires one argument which is a pre-created mp_value list.
 *
 * @param media_type_id Media type ID of the structure.
 * @param ... Variadic list of field definitions, terminated by 0.
 *
 * @return Pointer to the newly created @ref struct mp_structure.
 * @retval NULL If memory allocation fails or the argument list is invalid.
 */
struct mp_structure *mp_structure_new(uint8_t media_type_id, ...);

/**
 * @brief Initialize an @ref struct mp_structure.
 *
 * @param structure Structure to initialize.
 * @param media_type_id Media type ID of the structure.
 */
void mp_structure_init(struct mp_structure *structure, uint8_t media_type_id);

/**
 * @brief Append a field to an @ref struct mp_structure
 *
 * @param structure Structure to append the field to.
 * @param field_id Field ID (field ID must be unique)
 * @param value Field value (the value will be copied)
 */
void mp_structure_append(struct mp_structure *structure, uint8_t field_id, struct mp_value *value);

/**
 * @brief Clear all fields from an @ref struct mp_structure.
 *
 * @param structure Structure to clear.
 */
void mp_structure_clear(struct mp_structure *structure);

/**
 * @brief Destroy an @ref struct mp_structure.
 *
 * Clears all fields and releases resources associated with the structure.
 *
 * @param structure Pointer to the structure to destroy.
 */
void mp_structure_destroy(struct mp_structure *structure);

/**
 * @brief Check if an @ref struct mp_structure is fixed.
 *
 * A structure is considered fixed if all its fields contain single values.
 *
 * @param structure Structure to check.
 *
 * @return True if the structure is fixed, false otherwise.
 */
bool mp_structure_is_fixed(struct mp_structure *structure);

/**
 * @brief Get the value of a field in an @ref struct mp_structure.
 *
 * Retrieves the value associated with the specified field ID.
 *
 * @param structure Structure containing the field.
 * @param field_id ID of the field.
 *
 * @return Pointer to the value of the field, NULL if the field was not found.
 */
struct mp_value *mp_structure_get_value(struct mp_structure *structure, uint8_t field_id);

/**
 * @brief Remove a field from an @ref struct mp_structure.
 *
 * Deletes the field with the specified ID from the structure.
 *
 * @param structure Structure containing the field to remove.
 * @param field_id ID of the field to remove.
 *
 * @return True if the field was found and removed, false otherwise.
 */
bool mp_structure_remove_field(struct mp_structure *structure, uint8_t field_id);

/**
 * @brief Check if two @ref struct mp_structure can intersect.
 *
 * Two structures can intersect if all common fields have intersecting values.
 *
 * @param struct1 First structure.
 * @param struct2 Second structure.
 *
 * @return True if the structures can intersect, false otherwise.
 */
bool mp_structure_can_intersect(struct mp_structure *struct1, struct mp_structure *struct2);

/**
 * @brief Create a new intersected @ref struct mp_structure.
 *
 * @param structure1 First structure.
 * @param structure2 Second structure.
 *
 * @return Pointer to the new intersected structure.
 */
struct mp_structure *mp_structure_intersect(struct mp_structure *structure1,
					    struct mp_structure *structure2);

/**
 * @brief Create a new fixated @ref struct mp_structure.
 *
 * Generates a new structure containing only single values from the input structure.
 *
 * @param src Structure to fixate.
 *
 * @return Pointer to the new fixated structure.
 */
struct mp_structure *mp_structure_fixate(struct mp_structure *src);

/**
 * @brief Duplicate an @ref struct mp_structure.
 *
 * @param src Structure to duplicate.
 *
 * @return Pointer to the duplicated structure.
 */
struct mp_structure *mp_structure_duplicate(struct mp_structure *src);

/**
 * @brief Print an @ref struct mp_structure.
 *
 * Outputs the contents of the structure for debugging or inspection.
 *
 * @param structure Structure to print.
 */
void mp_structure_print(struct mp_structure *structure);

/** @} */

#endif /*__MP_STRUCTURE_H__*/
