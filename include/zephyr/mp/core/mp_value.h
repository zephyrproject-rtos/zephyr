/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_value.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_VALUE_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_VALUE_H_

/**
 * @defgroup mp_value Value Container
 * @ingroup mp_core
 * @brief A generic container for values for different @ref mp_value_type
 *
 * @{
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/mp/core/mp_object.h>

/** @brief Value comparison result: first value is less than second */
#define MP_VALUE_LESS_THAN      -1
/** @brief Value comparison result: values are equal */
#define MP_VALUE_EQUAL          0
/** @brief Value comparison result: first value is greater than second */
#define MP_VALUE_GREATER_THAN   1
/** @brief Value comparison result: values cannot be ordered */
#define MP_VALUE_UNORDERED      2
/** @brief Value comparison failed due to error */
#define MP_VALUE_COMPARE_FAILED 3

/**
 * @brief mp_value type enumeration
 */
enum mp_value_type {
	MP_TYPE_NONE = 0,            /**< No type */
	MP_TYPE_BOOLEAN,             /**< Boolean value */
	MP_TYPE_ENUM,                /**< Enumeration value */
	MP_TYPE_INT,                 /**< Signed integer value */
	MP_TYPE_UINT,                /**< Unsigned integer value */
	MP_TYPE_UINT_FRACTION,       /**< Unsigned integer fraction value */
	MP_TYPE_INT_FRACTION,        /**< Fraction value */
	MP_TYPE_STRING,              /**< String value */
	MP_TYPE_INT_RANGE,           /**< Integer range value */
	MP_TYPE_INT_FRACTION_RANGE,  /**< Integer fraction range value */
	MP_TYPE_UINT_RANGE,          /**< Unsigned integer range value */
	MP_TYPE_UINT_FRACTION_RANGE, /**< Unsigned integer fraction range value */
	MP_TYPE_LIST,                /**< List of values */
	MP_TYPE_OBJECT,              /**< Object reference */
	MP_TYPE_PTR,                 /**< Pointer type */
	MP_TYPE_COUNT                /**< Number of types */
};

/**
 * @brief Base mp_value structure
 */
struct mp_value {
	/** Type of value, see @ref mp_value_type */
	enum mp_value_type type;
};

/**
 * @brief Create a new mp_value with the specified type and initialization arguments.
 *
 * This function creates a new mp_value instance based on the provided type.
 * The number and type of variadic arguments depend on the specified enum mp_value_type:
 *
 * - MP_TYPE_BOOLEAN, MP_TYPE_ENUM, MP_TYPE_INT, MP_TYPE_UINT, MP_TYPE_STRING,
 *   MP_TYPE_OBJECT, MP_TYPE_PTR: Require one initialization value.
 * - MP_TYPE_INT_RANGE: Requires three integer values (min, max, and step).
 * - MP_TYPE_FRACTION: Requires two values (numerator and denominator).
 * - MP_TYPE_FRACTION_RANGE: Requires six values (min numerator, min denominator,
 *   max numerator, max denominator, step numerator, and step denominator).
 * - MP_TYPE_LIST: Requires a sequence of mp_value elements, terminated with NULL
 *   to indicate the end of the list.
 *
 * @param type The type of the value to create.
 * @param ... Variadic arguments used to initialize the value, depending on the specified type.
 *
 * @return Pointer to the newly created mp_value, NULL if memory allocation fails or an invalid
 *         type or argument list is provided.
 */
struct mp_value *mp_value_new(enum mp_value_type type, ...);

/**
 * @brief Create a new mp_value from a va_list.
 *
 * Same as @ref mp_value_new but accepts a va_list pointer instead of variadic arguments.
 *
 * @param type The type of the value to create.
 * @param args Pointer to a va_list containing the initialization arguments.
 *
 * @return Pointer to the newly created mp_value, or NULL on failure.
 */
struct mp_value *mp_value_new_va_list(enum mp_value_type type, va_list *args);

/**
 * @brief Create an empty value with given type.
 *
 * @param type type of value
 *
 * @return Pointer to the newly created mp_value, or NULL on failure.
 */
struct mp_value *mp_value_new_empty(enum mp_value_type type);

/**
 * @brief Destroy a value and release its resources.
 *
 * @param value value to destroy
 *
 * @return 0 on success, -EINVAL if value is NULL
 */
int mp_value_destroy(struct mp_value *value);

/**
 * @brief Get list size.
 *
 * @param list list of values
 *
 * @return size of list
 */
size_t mp_value_list_get_size(const struct mp_value *list);

/**
 * @brief Return true if list is empty.
 *
 * @param list list of values
 *
 * @return true if list is empty, false otherwise
 */
bool mp_value_list_is_empty(const struct mp_value *list);

/**
 * @brief Append value to list.
 *
 * @param list list to append to
 * @param append_value value to append
 *
 * @return 0 on success, -EINVAL if arguments are invalid, -ENOMEM on allocation failure
 */
int mp_value_list_append(struct mp_value *list, struct mp_value *append_value);

/**
 * @brief Set values to type.
 *
 * @param value value to set
 * @param type type of value
 * @param ... Variadic arguments used to initialize the value,
 *            same rule as @ref mp_value_new()
 *
 * @return 0 on success, -EINVAL if value is NULL or type/arguments are invalid
 */
int mp_value_set(struct mp_value *value, int type, ...);

/**
 * @brief Get value at index in list.
 *
 * @param list list of value
 * @param index index of value to get from list
 *
 * @return value at given index in list, or NULL if not found
 */
struct mp_value *mp_value_list_get(const struct mp_value *list, int index);

/** Get boolean value of MP_TYPE_BOOLEAN */
bool mp_value_get_boolean(const struct mp_value *value);

/** Get int value of MP_TYPE_INT */
int mp_value_get_int(const struct mp_value *value);

/** Get uint value of MP_TYPE_UINT */
uint32_t mp_value_get_uint(const struct mp_value *value);

/** Get string value of MP_TYPE_STRING */
const char *mp_value_get_string(const struct mp_value *value);

/** Get pointer value of MP_TYPE_PTR */
void *mp_value_get_ptr(const struct mp_value *value);

/** Get numerator of @ref mp_value with MP_TYPE_FRACTION*/
int mp_value_get_fraction_numerator(const struct mp_value *frac);

/** Get denominator of @ref mp_value with MP_TYPE_FRACTION */
int mp_value_get_fraction_denominator(const struct mp_value *frac);

/** Get minimum value of @ref mp_value with MP_TYPE_INT_RANGE */
int mp_value_get_int_range_min(const struct mp_value *range);

/** Get maximum value of @ref mp_value with MP_TYPE_INT_RANGE */
int mp_value_get_int_range_max(const struct mp_value *range);

/** Get step value of @ref mp_value with MP_TYPE_INT_RANGE */
int mp_value_get_int_range_step(const struct mp_value *range);

/** Get minimum value of @ref mp_value with MP_TYPE_UINT_RANGE */
uint32_t mp_value_get_uint_range_min(const struct mp_value *range);

/** Get maximum value of @ref mp_value with MP_TYPE_UINT_RANGE */
uint32_t mp_value_get_uint_range_max(const struct mp_value *range);

/** Get step value of @ref mp_value with MP_TYPE_UINT_RANGE */
uint32_t mp_value_get_uint_range_step(const struct mp_value *range);

/** Get the min value of a mp_value with type MP_TYPE_FRACTION_RANGE, returning a mp_value with
 * MP_TYPE_FRACTION
 */
const struct mp_value *mp_value_get_fraction_range_min(const struct mp_value *fraction_range);

/** Get the max value of a mp_value with type MP_TYPE_FRACTION_RANGE, returning a mp_value with
 * MP_TYPE_FRACTION
 */
const struct mp_value *mp_value_get_fraction_range_max(const struct mp_value *fraction_range);

/** Get the step value of a mp_value with type MP_TYPE_FRACTION_RANGE, returning a mp_value
 * with MP_TYPE_FRACTION
 */
const struct mp_value *mp_value_get_fraction_range_step(const struct mp_value *fraction_range);

/** Get the object reference of a mp_value with MP_TYPE_OBJECT */
struct mp_object *mp_value_get_object(struct mp_value *value);

/**
 * Comparison between two primitive values
 *
 * @param val1 first value
 * @param val2 second value
 * @return MP_VALUE_GREATER_THAN if val1 > val2
 *	MP_VALUE_LESS_THAN if val1 < val2
 *	MP_VALUE_EQUAL if val1 == val2
 *	MP_VALUE_UNORDERED if val1 and val2 are not comparable
 *	MP_VALUE_COMPARE_FAILED if val1 and val2 are not same type
 */
int mp_value_compare(const struct mp_value *val1, const struct mp_value *val2);

/**
 * Intersect between two values
 *
 * @param val1 reference value to compare with
 * @param val2 value to compare with
 * @return NULL if intersect is empty
 */
struct mp_value *mp_value_intersect(const struct mp_value *val1, const struct mp_value *val2);

/**
 * Comparison between two fractions
 *
 * @param frac1 first fraction
 * @param frac2 second fraction
 * @return MP_VALUE_GREATER_THAN if frac1 > frac2
 * MP_VALUE_LESS_THAN if frac1 < frac2
 * MP_VALUE_EQUAL if frac1 == frac2
 */
int mp_value_compare_fraction(const struct mp_value *frac1, const struct mp_value *frac2);

/**
 * Intersect between value and range
 *
 * @param ref_val reference value to compare with
 * @param compare_val value to compare with
 * @return NULL if intersect is empty
 */
struct mp_value *mp_value_intersect_int_range(const struct mp_value *ref_val,
					      const struct mp_value *compare_val);

/**
 * Intersect between franction range and fraction
 *
 * @param ref_val reference value to compare with, reference value should be
 * fraction range
 * @param compare_val value to compare with
 * @return NULL if intersect is empty
 */
struct mp_value *mp_value_intersect_fraction_range(const struct mp_value *ref_val,
						   const struct mp_value *compare_val);

/**
 * Intersect between list with value, range or list
 *
 * @param list reference value to compare with
 * @param compare_val value to compare with
 * @return NULL if intersect is empty
 */
struct mp_value *mp_value_intersect_list(const struct mp_value *list,
					 const struct mp_value *compare_val);

/**
 * Check if two values can intersect
 *
 * @param val1 first value
 * @param val2 second value
 * @return true if two values can intersect
 */
bool mp_value_can_intersect(const struct mp_value *val1, const struct mp_value *val2);

/**
 * Duplicate value
 *
 * @param value value to duplicate
 * @return new value with same type and data as original value, or NULL on failure
 * @note For string only pointer is copied, not string itself.
 */
struct mp_value *mp_value_duplicate(const struct mp_value *value);

/**
 * @brief Check if a value is a primitive type
 *
 * @param value Value to check, must not be NULL
 *
 * @return true if value is primitive, false otherwise
 */
bool mp_value_is_primitive(const struct mp_value *value);

/**
 * @brief Print a value
 *
 * @param value Value to print, may be NULL
 * @param new_line Add newline after printing
 */
void mp_value_print(const struct mp_value *value, bool new_line);

/** @} */

#endif /*ZEPHYR_INCLUDE_MP_CORE_MP_VALUE_H_*/
