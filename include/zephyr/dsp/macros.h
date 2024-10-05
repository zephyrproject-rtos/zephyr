/* Copyright (c) 2024 tinyVision.ai Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DSP_MACROS_H_
#define INCLUDE_ZEPHYR_DSP_MACROS_H_

#include <stdint.h>

#include <zephyr/dsp/types.h>
#include <zephyr/sys/util.h>

/**
 * @brief Definition of the minimum and maximum values of the internal representation
 *
 * @note The value represented is always between -1.0 and 1.0 for any storage size.
 *       Scaling variables imported before using them in fixed-points is required.
 *
 * @{
 */

/** Minimum internal value for Q.7. */
#define MINq7 INT8_MIN

/** Minimum internal value for Q.15. */
#define MINq15 INT16_MIN

/** Minimum internal value for Q.31. */
#define MINq31 INT32_MIN

/** Maximum internal value for Q.7. */
#define MAXq7 INT8_MAX

/** Maximum internal value for Q.15. */
#define MAXq15 INT16_MAX

/** Maximum internal value for Q.31. */
#define MAXq31 INT32_MAX

/* @}
 */

/**
 * @brief Clamp a fixed-point value to -1.0 to 1.0.
 *
 * @note Useful for internal use, or when breaking the abstraction barrier.
 *
 * @{
 */

/** Enforce the a Q.7 fixed-point to be between -1.0 and 1.0 */
#define CLAMPq7(q7) ((q7_t)CLAMP((q7), MINq7, MAXq7))

/** Enforce the a Q.15 fixed-point to be between -1.0 and 1.0 */
#define CLAMPq15(q15) ((q15_t)CLAMP((q15), MINq15, MAXq15))

/** Enforce the a Q.31 fixed-point to be between -1.0 and 1.0 */
#define CLAMPq31(q31) ((q31_t)CLAMP((q31), MINq31, MAXq31))

/* @}
 */

/**
 * @brief Construct a fixed-point number out of a floating point number.
 *
 * The input float can be an expression or a single number.
 * If it is below -1.0 or above 1.0, it is adjusted to fit the -1.0 to 1.0 range.
 *
 * @example "Q15(-1.0) // Minimum value"
 * @example "Q31(1.0) // Maximum value"
 * @example "Q15(1.3) // Will become 1.0"
 * @example "Q7(f - 1.0)"
 * @example "Q15(2.0 / 3.1415)"
 *
 * @param f Floating-point number to convert into a fixed-point number.
 *
 * @{
 */

/** Construct a Q.7 fixed-point: 1 bit for the sign, 7 bits of fractional part */
#define Q7f(f) CLAMPq7((f) * (1ll << 7))

/** Construct a Q.15 fixed-point: 1 bit for the sign, 15 bits of fractional part */
#define Q15f(f) CLAMPq15((f) * (1ll << 15))

/** Construct a Q.31 fixed-point: 1 bit for the sign, 31 bits of fractional part */
#define Q31f(f) CLAMPq31((f) * (1ll << 31))

/* @}
 */

/**
 * @brief Construct a fixed-point out of an integer and a scale.
 *
 * This permits to work with numbers above the -1.0 to 1.0 range:
 * The minimum and maximum possible value of the input number are specified.
 * The input number is then scaled so that the minimum value becomes -1.0 and
 * maximum value becomes 1.0.
 * Once the computation is done in fixed piotn, it is popssible to get an
 * unscaled value with @ref INTq7, @ref INtq15 and @rev INTq31.
 *
 * @example "Q15i(temperature, -20, 200)"
 * @example "Q15i(angle, 0, 360)"
 * @example "Q15i(voltage, V_MIN, V_MAX)"
 *
 * @param i Input number to convert to a fixed-point representation
 * @param min Minimum value that @p i can take.
 *            The same minimum value is used to convert the numbers back.
 * @param max Maximum value that @p i can take.
 *            The same maximum value is used to convert the numbers back.
 *
 * @{
 */

/** Build a Q.7 number by scaling @p i range from [min, max] (at most [ , ]) to [-1.0, 1.0] */
#define Q7i(i, min, max) CLAMPq7(SCALE((i), (min), (max), MINq7, MAXq7))

/** Build a Q.15 number by scaling @p i range from [min, max] to [-1.0, 1.0] */
#define Q15i(i, min, max) CLAMPq15(SCALE((i), (min), (max), MINq15, MAXq15))

/** Build a Q.31 number by scaling @p i range from [min, max] to [-1.0, 1.0] */
#define Q31i(i, min, max) CLAMPq31(SCALE((i), (min), (max), MINq31, MAXq31))

/* @}
 */

/**
 * @brief Convert fixed-points from one size to another
 *
 * This permits to increase or decrease the precision by switching between
 * smaller and larger representation.
 * The values represented does not change except due to loss of precision.
 * The minimum value remains -1.0 and maximum value remains 1.0.
 * Only the internal representation is scaled.
 *
 * @{
 */

/** Build a Q.7 number out of a Q.15 number, losing 8 bits of precision */
#define Q7q15(q15) (q7_t)((q15) / (1 << 8))

/** Build a Q.7 number out of a Q.31 number, losing 24 bits of precision */
#define Q7q31(q31) (q7_t)((q31) / (1 << 24))

/** Build a Q.15 number out of a Q.7 number, gaining 8 bits of precision */
#define Q15q7(q7) CLAMPq15((q15_t)(q7) * (1 << 8))

/** Build a Q.15 number out of a Q.31 number, losing 16 bits of precision */
#define Q15q31(q31) (q15_t)((q31) / (1 << 16))

/** Build a Q.31 number out of a Q.7 number, gaining 24 bits of precision */
#define Q31q7(q7) CLAMPq31((q31_t)(q7) * (1 << 24))

/** Build a Q.31 number out of a Q.15 number, gaining 16 bits of precision */
#define Q31q15(q15) CLAMPq31((q31_t)(q15) * (1 << 16))

/* @}
 */

/**
 * @brief Convert a fixed-point number back to an natural representation.
 *
 * This permits to extract a result value out of a fixed-point number,
 * to reverse the effect of @ref Q7i, @ref Q15i and @ref Q31i.
 *
 * @param i The fixed-point number to convert to a natural number.
 * @param min The minimum value specified to create the fixed-point.
 * @param max The maximum value specified to create the fixed-point.
 *
 * @{
 */

/** Convert a Q.7 fixed-point number to a natural integer */
#define INTq7(i, min, max) SCALE((i), MINq7, MAXq7, (min), (max))

/** Convert a Q.15 fixed-point number to a natural integer */
#define INTq15(i, min, max) SCALE((i), MINq15, MAXq15, (min), (max))

/** Convert a Q.31 fixed-point number to a natural integer */
#define INTq31(i, min, max) SCALE((i), MINq31, MAXq31, (min), (max))

/* @}
 */

/**
 * @brief Add two fixed-point numbers together.
 *
 * Saturation logic is applied, so number out of ranges will be converted
 * to the minimum -1.0 or maximum 1.0 value instead of overflowing.
 *
 * @note If two fixed-point numbers are to be subtracted into a larger type
 *       such as Q.7 + Q.7 = Q.15, the C operator @c + can be used instead.
 *
 * @param a First number to add.
 * @param b Second number to add.
 *
 * @{
 */

/** Sum two Q.7 numbers and produce a Q.7 result */
#define ADDq7(a, b) CLAMPq7((int16_t)(a) + (int16_t)(b))

/** Sum two Q.15 numbers and produce a Q.15 result */
#define ADDq15(a, b) CLAMPq15((int32_t)(a) + (int32_t)(b))

/** Sum two Q.31 numbers and produce a Q.31 result */
#define ADDq31(a, b) CLAMPq31((int64_t)(a) + (int64_t)(b))

/* @}
 */

/**
 * @brief Subtract a fixed-point number to another.
 *
 * Saturation logic is applied, so number out of ranges will be converted
 * to the minimum 1.0 or maximum -1.0 value instead of overflowing.
 *
 * @note If two fixed-point numbers are to be subtracted into a larger type
 *       such as Q.7 - Q.7 = Q.15, the C operator @c - can be used instead.
 *
 * @param a First number to add.
 * @param a Second number to add.
 *
 * @{
 */

/** Subtract two Q.7 numbers and produce a Q.7 result */
#define SUBq7(a, b) CLAMPq7((int16_t)(a) - (int16_t)(b))

/** Subtract two Q.15 numbers and produce a Q.15 result */
#define SUBq15(a, b) CLAMPq15((int32_t)(a) - (int32_t)(b))

/** Subtract two Q.31 numbers and produce a Q.31 result */
#define SUBq31(a, b) CLAMPq31((int64_t)(a) - (int64_t)(b))

/* @}
 */

/**
 * @brief Multiply two fixed-point numbers together.
 *
 * Saturation logic is applied, so number out of range will be converted
 * to handle the edge case Q#f(-1.0) * Q#f(-1.0) = Q#f(1.0)
 *
 * @note This implementation does not perform rounding.
 *
 * @param a First number to add.
 * @param b Second number to add.
 *
 * @{
 */

/** Multiply two Q.7 numbers and produce a Q.7 result */
#define MULq7(a, b) CLAMPq7(((int16_t)(a) * (int16_t)(b)) / (1 << 7))

/** Multiply two Q.15 numbers and produce a Q.15 result */
#define MULq15(a, b) CLAMPq15(((int32_t)(a) * (int32_t)(b)) / (1 << 15))

/** Multiply two Q.31 numbers and produce a Q.31 result */
#define MULq31(a, b) CLAMPq31(((int64_t)(a) * (int64_t)(b)) / (1 << 31))

/* @}
 */

/**
 * @brief Divide two fixed-point numbers together.
 *
 * Saturation logic is applied, so number out of ranges will be converted
 * to the minimum -1.0 or maximum 1.0 value instead of overflowing.
 *
 * @note This implementation does not perform rounding.
 *
 * @param a Numerator of the division.
 * @param b Denominator of the division.
 *
 * @{
 */

/** Divide a Q.7 number and produce a Q.7 result */
#define DIVq7(a, b) CLAMPq7((a) * (1 << 7) / (b))

/** Divide a Q.15 number and produce a Q.15 result */
#define DIVq15(a, b) CLAMPq15((a) * (1 << 15) / (b))

/** Divide a Q.31 number and produce a Q.31 result */
#define DIVq31(a, b) CLAMPq31((a) * (1 << 31) / (b))

/* @}
 */

/**
 * @brief Apply the opposite value of the fixed-point number.
 *
 * Saturation logic is applied, as the most negative number could
 * overflow internally if converted to positive.
 *
 * @param a Number to get the opposite value for
 *
 * @{
 */

/** Get the negation of a Q.7 number and produce a Q.7 result */
#define NEGq7(a) (q7_t)((a) == MINq7 ? MAXq7 : -(a))

/** Get the negation of a Q.15 number and produce a Q.15 result */
#define NEGq15(a) (q15_t)((a) == MINq15 ? MAXq15 : -(a))

/** Get the negation of a Q.15 number and produce a Q.15 result */
#define NEGq31(a) (q31_t)((a) == MINq31 ? MAXq31 : -(a))

/* @}
 */

/**
 * @brief Get the absolute value of a fixed-point number.
 *
 * Saturation logic is applied, as the most negative number overflows if
 * converted to positive.
 *
 * @param a Number to get the absolute value for.
 *
 * @{
 */

/** Get the absolute value of a Q.7 number and produce a Q.7 result */
#define ABSq7(a) (q7_t)((a) < 0 ? NEGq7(a) : (a))

/** Get the absolute value of a Q.15 number and produce a Q.15 result */
#define ABSq15(a) (q15_t)((a) < 0 ? NEGq15(a) : (a))

/** Get the absolute value of a Q.31 number and produce a Q.31 result */
#define ABSq31(a) (q31_t)((a) < 0 ? NEGq31(a) : (a))

/* @}
 */

#endif /* INCLUDE_ZEPHYR_DSP_MACROS_H_ */
