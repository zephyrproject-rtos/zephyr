/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpValue.
 */

#ifndef __MP_VALUE_H__
#define __MP_VALUE_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "mp_object.h"

/**
 * @defgroup MpValue
 * @brief A generic container for values for different @ref MpValueType
 *
 * @{
 */

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
 * @brief MpValue type enumeration
 */
typedef enum {
	MP_TYPE_NONE = 0,       /**< No type */
	MP_TYPE_BOOLEAN,        /**< Boolean value */
	MP_TYPE_ENUM,           /**< Enumeration value */
	MP_TYPE_INT,            /**< Signed integer value */
	MP_TYPE_UINT,           /**< Unsigned integer value */
	MP_TYPE_FRACTION,       /**< Fraction value */
	MP_TYPE_STRING,         /**< String value */
	MP_TYPE_INT_RANGE,      /**< Integer range value */
	MP_TYPE_FRACTION_RANGE, /**< Fraction range value */
	MP_TYPE_LIST,           /**< List of values */
	MP_TYPE_OBJECT,         /**< Object reference */
	MP_TYPE_PTR,            /**< Pointer type */
	MP_TYPE_COUNT           /**< Number of types */
} MpValueType;

/**
 * @brief Base MpValue structure
 */
typedef struct {
	/** Type of value, see @ref MpValueType */
	MpValueType type;
} MpValue;

/**
 * @brief Create a new MpValue with the specified type and initialization arguments.
 *
 * This function creates a new MpValue instance based on the provided type.
 * The number and type of variadic arguments depend on the specified MpValueType:
 *
 * - MP_TYPE_BOOLEAN, MP_TYPE_ENUM, MP_TYPE_INT, MP_TYPE_UINT, MP_TYPE_STRING,
 *   MP_TYPE_OBJECT, MP_TYPE_PTR: Require one initialization value.
 * - MP_TYPE_INT_RANGE: Requires three integer values (min, max, and step).
 * - MP_TYPE_FRACTION: Requires two values (numerator and denominator).
 * - MP_TYPE_FRACTION_RANGE: Requires six values (min numerator, min denominator,
 *   max numerator, max denominator, step numerator, and step denominator).
 * - MP_TYPE_LIST: Requires a sequence of MpValue elements, terminated with NULL
 *   to indicate the end of the list.
 *
 * @param type The type of the value to create.
 * @param ... Variadic arguments used to initialize the value, depending on the specified type.
 *
 * @return Pointer to the newly created MpValue,  NULL if memory allocation fails or an invalid type
 * or argument list is provided.
 */
MpValue *mp_value_new(MpValueType type, ...);

MpValue *mp_value_new_va_list(MpValueType type, va_list *args);

/**
 * Create an empty value with given type
 *
 * @param type type of value
 * @return empty value with given type
 */
MpValue *mp_value_new_empty(MpValueType type);

/**
 * Destroy value
 *
 * @param value value to destroy
 */
void mp_value_destroy(MpValue *value);

/**
 * Get list size
 *
 * @param list list of values
 * @return size of list
 */
size_t mp_value_list_get_size(const MpValue *list);

/**
 * Return true if list is empty
 *
 * @param list: list of values
 */
bool mp_value_list_is_empty(const MpValue *list);

/**
 * Append value to list
 *
 * @param list list to append to
 * @param append_value value to append
 */
void mp_value_list_append(MpValue *list, MpValue *append_value);

/**
 * Set values to type
 * @param value value to set
 * @param type type of value
 * @param ... Variadic arguments used to initialize the value, same
 * rule as @ref mp_value_new()
 */
void mp_value_set(MpValue *value, int type, ...);

/**
 * Get value at index in list
 * @param list list of value
 * @param index index of value to get from list
 * @return (nullable) value at given index in list
 */
MpValue *mp_value_list_get(const MpValue *list, int index);

/** Get boolean value of MP_TYPE_BOOLEAN */
bool mp_value_get_boolean(const MpValue *value);

/** Get int value of MP_TYPE_INT */
int mp_value_get_int(const MpValue *value);

/** Get uint value of MP_TYPE_UINT */
uint32_t mp_value_get_uint(const MpValue *value);

/** Get string value of MP_TYPE_STRING */
const char *mp_value_get_string(const MpValue *value);

/** Get pointer value of MP_TYPE_PTR */
void *mp_value_get_ptr(const MpValue *value);

/** Get numerator of @ref MpValue with MP_TYPE_FRACTION*/
int mp_value_get_fraction_numerator(const MpValue *frac);

/** Get denominator of @ref MpValue with MP_TYPE_FRACTION */
int mp_value_get_fraction_denominator(const MpValue *frac);

/** Get minimum value of @ref MpValue with MP_TYPE_INT_RANGE */
int mp_value_get_int_range_min(const MpValue *range);

/** Get maximum value of @ref MpValue with MP_TYPE_INT_RANGE */
int mp_value_get_int_range_max(const MpValue *range);

/** Get step value of @ref MpValue with MP_TYPE_INT_RANGE */
int mp_value_get_int_range_step(const MpValue *range);

/** Get the min value of a MpValue with type MP_TYPE_FRACTION_RANGE, returning a MpValue with
 * MP_TYPE_FRACTION */
const MpValue *mp_value_get_fraction_range_min(const MpValue *fraction_range);

/** Get the max value of a MpValue with type MP_TYPE_FRACTION_RANGE, returning a MpValue with
 * MP_TYPE_FRACTION */
const MpValue *mp_value_get_fraction_range_max(const MpValue *fraction_range);

/** Get the step value of a MpValue with type MP_TYPE_FRACTION_RANGE, returning a MpValue
 * with MP_TYPE_FRACTION */
const MpValue *mp_value_get_fraction_range_step(const MpValue *fraction_range);

/** Get the object reference of a MpValue with MP_TYPE_OBJECT */
MpObject *mp_value_get_object(MpValue *value);

/**
 * Comparison between two primitive values
 *
 * @param val1 first value
 * @param val2 second value
 * @return MP_VALUE_GREATER_THAN if val1 > val2
 * 	MP_VALUE_LESS_THAN if val1 < val2
 * 	MP_VALUE_EQUAL if val1 == val2
 * 	MP_VALUE_UNORDERED if val1 and val2 are not comparable
 * 	MP_VALUE_COMPARE_FAILED if val1 and val2 are not same type
 */
int mp_value_compare(const MpValue *val1, const MpValue *val2);

/**
 * Intersect between two values
 *
 * @param val1 reference value to compare with
 * @param val2 value to compare with
 * @return NULL if intersect is empty
 */
MpValue *mp_value_intersect(const MpValue *val1, const MpValue *val2);

/**
 * Comparison between two fractions
 *
 * @param frac1 first fraction
 * @param frac2 second fraction
 * @return MP_VALUE_GREATER_THAN if frac1 > frac2
 * 	MP_VALUE_LESS_THAN if frac1 < frac2
 * 	MP_VALUE_EQUAL if frac1 == frac2
 */
int mp_value_compare_fraction(const MpValue *frac1, const MpValue *frac2);

/**
 * Intersect between value and range
 *
 * @param ref_val reference value to compare with
 * @param compare_val value to compare with
 * @return NULL if intersect is empty
 */
MpValue *mp_value_intersect_int_range(const MpValue *ref_val, const MpValue *compare_val);

/**
 * Intersect between franction range and fraction
 *
 * @param ref_val reference value to compare with, reference value should be
 * fraction range
 * @param compare_val value to compare with
 * @return NULL if intersect is empty
 */
MpValue *mp_value_intersect_fraction_range(const MpValue *ref_val, const MpValue *compare_val);

/**
 * Intersect between list with value, range or list
 *
 * @param list reference value to compare with
 * @param compare_val value to compare with
 * @return NULL if intersect is empty
 */
MpValue *mp_value_intersect_list(const MpValue *list, const MpValue *compare_val);

/**
 * Check if two values can intersect
 *
 * @param val1 first value
 * @param val2 second value
 * @return true if two values can intersect
 */
bool mp_value_can_intersect(const MpValue *val1, const MpValue *val2);

/**
 * Duplicate value
 *
 * @param value value to duplicate
 * @return new value with same type and data as original value
 * @note For string only pointer is copied, not string itself.
 */
MpValue *mp_value_duplicate(const MpValue *value);

/**
 * @brief Check if a value is a primitive type
 *
 * @param value Value to check, must not be NULL
 *
 * @return true if value is primitive, false otherwise
 */
bool mp_value_is_primitive(const MpValue *value);

/**
 * @brief Print a value
 *
 * @param value Value to print, may be NULL
 * @param new_line Add newline after printing
 */
void mp_value_print(const MpValue *value, bool new_line);

#endif /*__MP_VALUE_H__*/
