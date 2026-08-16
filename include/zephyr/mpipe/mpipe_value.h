/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Value: one typed scalar or range inside a capability.
 * @ingroup mpipe_value
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_VALUE_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_VALUE_H_

/**
 * @defgroup mpipe_value Value Container
 * @ingroup mpipe_framework
 * @brief One typed scalar or range, the smallest piece of a capability.
 *
 * An @ref mpipe_value is what a field of an @ref mpipe_structure holds: a
 * boolean, a signed or unsigned integer, or a `{min, max, step}` range of one
 * of those. A range is how a device says it accepts a span rather than one
 * setting - every width from 16 to 1280 in steps of 2 - and fixation later
 * picks a single value out of it.
 *
 * It is a tagged union with no pointer in any arm, so it is the same size on
 * every target, copying one is a plain assignment, and there is nothing to
 * release. That is what lets a capability be passed by value and placed in
 * `.rodata`.
 *
 * The type set is deliberately just these two shapes, because they are closed
 * under intersection: intersecting two values never yields something larger
 * than its inputs, so the result always fits in storage the caller already has.
 * A set-of-alternatives type would break that and force an allocation back in,
 * which is why alternatives live on an enumeration index instead - see
 * @ref mpipe_pad.
 *
 * Pointers passed to this API must not be NULL unless the parameter is
 * documented otherwise.
 *
 * @{
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Value types
 */
enum mpipe_value_type {
	MPIPE_TYPE_NONE = 0,   /**< No type */
	MPIPE_TYPE_BOOLEAN,    /**< Boolean value */
	MPIPE_TYPE_INT,        /**< Signed integer value */
	MPIPE_TYPE_UINT,       /**< Unsigned integer value */
	MPIPE_TYPE_INT_RANGE,  /**< Integer range value */
	MPIPE_TYPE_UINT_RANGE, /**< Unsigned integer range value */
	MPIPE_TYPE_COUNT       /**< Number of types */
};

/**
 * @brief One end of a range value, or its step
 */
union mpipe_value_bound {
	/** MPIPE_TYPE_INT_RANGE */
	int32_t v_int;
	/** MPIPE_TYPE_UINT_RANGE */
	uint32_t v_uint;
};

/**
 * @brief Value structure
 *
 * One fixed-size tagged union so that every value costs the same
 * and can be stored by value inside the structure that owns it.
 */
struct mpipe_value {
	/** Type of value, see @ref mpipe_value_type */
	enum mpipe_value_type type;
	/** Contents, read through the arm the type selects */
	union {
		/** MPIPE_TYPE_BOOLEAN */
		bool v_boolean;
		/** MPIPE_TYPE_INT */
		int32_t v_int;
		/** MPIPE_TYPE_UINT */
		uint32_t v_uint;
		/** MPIPE_TYPE_INT_RANGE, MPIPE_TYPE_UINT_RANGE */
		struct {
			/** Lower bound of the range */
			union mpipe_value_bound min;
			/** Upper bound of the range */
			union mpipe_value_bound max;
			/** Step between two consecutive values of the range */
			union mpipe_value_bound step;
		} range;
	};
};

/**
 * @name Initializers for a statically defined value
 *
 * Each expands to a braced initializer for one @ref mpipe_value, so a capability
 * known at build time can be placed in .rodata rather than built at runtime.
 * They are what @ref MPIPE_STRUCTURE_DEFINE takes for each of its fields. The
 * range forms take the bounds and the step between two consecutive values.
 * @{
 */
/** Initialize an MPIPE_TYPE_BOOLEAN value holding @p v */
#define MPIPE_VALUE_BOOLEAN(v)                {.type = MPIPE_TYPE_BOOLEAN, .v_boolean = (v)}
/** Initialize an MPIPE_TYPE_INT value holding @p v */
#define MPIPE_VALUE_INT(v)                    {.type = MPIPE_TYPE_INT, .v_int = (v)}
/** Initialize an MPIPE_TYPE_UINT value holding @p v */
#define MPIPE_VALUE_UINT(v)                   {.type = MPIPE_TYPE_UINT, .v_uint = (v)}
/** @cond INTERNAL_HIDDEN */
#define MPIPE_VALUE_RANGE_INIT(m, lo, hi, st) {.min.m = (lo), .max.m = (hi), .step.m = (st)}
/** @endcond */
/** Initialize an MPIPE_TYPE_INT_RANGE value spanning @p lo to @p hi by @p st */
#define MPIPE_VALUE_INT_RANGE(lo, hi, st)                                                          \
	{.type = MPIPE_TYPE_INT_RANGE, .range = MPIPE_VALUE_RANGE_INIT(v_int, lo, hi, st)}
/** Initialize an MPIPE_TYPE_UINT_RANGE value spanning @p lo to @p hi by @p st */
#define MPIPE_VALUE_UINT_RANGE(lo, hi, st)                                                         \
	{.type = MPIPE_TYPE_UINT_RANGE, .range = MPIPE_VALUE_RANGE_INIT(v_uint, lo, hi, st)}
/** @} */

/**
 * @brief Set a value's type and contents.
 *
 * BOOLEAN, INT and UINT take one argument; the range types take min, max
 * and step.
 *
 * @param value Pointer to the value to set.
 * @param type Type of the value, see @ref mpipe_value_type. It is an int and not
 *             the enum itself because va_start() starts reading right after
 *             this parameter, so the parameter cannot be of a type narrower
 *             than int, which an enum is allowed to be.
 * @param ... Arguments initializing the value, per the rules above.
 *
 * @return 0 on success, -EINVAL if @p type is invalid
 */
int mpipe_value_set(struct mpipe_value *value, int type, ...);

/**
 * @brief Set a value from a va_list.
 *
 * Same as @ref mpipe_value_set but accepts a va_list pointer instead of variadic
 * arguments, so a caller parsing its own list can build a value in place.
 *
 * @param value Pointer to the value to set.
 * @param type Type of the value, see @ref mpipe_value_type.
 * @param args Pointer to a va_list positioned at this value's arguments.
 *
 * @return 0 on success, -EINVAL if @p type is invalid
 */
int mpipe_value_set_va_list(struct mpipe_value *value, enum mpipe_value_type type, va_list *args);

/**
 * @brief Get boolean value of @ref MPIPE_TYPE_BOOLEAN type
 *
 * The getters read the requested member without checking the type, so the
 * caller is responsible for passing a value of the matching type.
 *
 * @param value Pointer to the value.
 *
 * @return The boolean the value holds.
 */
bool mpipe_value_get_boolean(const struct mpipe_value *value);

/**
 * @brief Get int value of @ref MPIPE_TYPE_INT type
 *
 * @param value Pointer to the value.
 *
 * @return The integer the value holds.
 */
int32_t mpipe_value_get_int(const struct mpipe_value *value);

/**
 * @brief Get uint value of @ref MPIPE_TYPE_UINT type
 *
 * @param value Pointer to the value.
 *
 * @return The unsigned integer the value holds.
 */
uint32_t mpipe_value_get_uint(const struct mpipe_value *value);

/**
 * @brief Get minimum value of a @ref MPIPE_TYPE_INT_RANGE type
 *
 * @param range Pointer to the range value.
 *
 * @return The lower bound of the range.
 */
int32_t mpipe_value_get_int_range_min(const struct mpipe_value *range);

/**
 * @brief Get maximum value of a @ref MPIPE_TYPE_INT_RANGE type
 *
 * @param range Pointer to the range value.
 *
 * @return The upper bound of the range.
 */
int32_t mpipe_value_get_int_range_max(const struct mpipe_value *range);

/**
 * @brief Get step value of a @ref MPIPE_TYPE_INT_RANGE type
 *
 * @param range Pointer to the range value.
 *
 * @return The step between two consecutive values of the range.
 */
int32_t mpipe_value_get_int_range_step(const struct mpipe_value *range);

/**
 * @brief Get minimum value of a @ref MPIPE_TYPE_UINT_RANGE type
 *
 * @param range Pointer to the range value.
 *
 * @return The lower bound of the range.
 */
uint32_t mpipe_value_get_uint_range_min(const struct mpipe_value *range);

/**
 * @brief Get maximum value of a @ref MPIPE_TYPE_UINT_RANGE type
 *
 * @param range Pointer to the range value.
 *
 * @return The upper bound of the range.
 */
uint32_t mpipe_value_get_uint_range_max(const struct mpipe_value *range);

/**
 * @brief Get step value of a @ref MPIPE_TYPE_UINT_RANGE type
 *
 * @param range Pointer to the range value.
 *
 * @return The step between two consecutive values of the range.
 */
uint32_t mpipe_value_get_uint_range_step(const struct mpipe_value *range);

/**
 * @brief Intersect two values, put the result into caller-provided storage
 *
 * @param val1 Pointer to the first value.
 * @param val2 Pointer to the second value.
 * @param out Pointer to storage for the result, untouched unless 0 is returned.
 *
 * @retval 0 Success.
 * @retval -ENOENT The intersection is empty
 */
int mpipe_value_intersect(const struct mpipe_value *val1, const struct mpipe_value *val2,
			  struct mpipe_value *out);

/**
 * @brief Check if a value is of a primitive type
 *
 * @param value Pointer to the value to check.
 *
 * @return true if the value is primitive, false otherwise or if @p value is NULL
 */
bool mpipe_value_is_primitive(const struct mpipe_value *value);

/**
 * @brief Print a value
 *
 * @param value Pointer to the value to print.
 * @param new_line Add newline after printing.
 */
void mpipe_value_print(const struct mpipe_value *value, bool new_line);

/** @} */

#endif /*ZEPHYR_INCLUDE_MPIPE_MPIPE_VALUE_H_*/
