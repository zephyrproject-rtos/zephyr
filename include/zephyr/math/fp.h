/**
 * @file zephyr/math/fp.h
 *
 * @brief A common math library for use in both floating and fixed point.
 */

/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_MATH_UTIL_H_
#define INCLUDE_MATH_UTIL_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MATH_UTIL_FLOAT
#define PRIf	    "." STRINGIFY(CONFIG_MATH_UTIL_PRIf_PRECISION) "f"
#define PRIf_ARG(n) (n)
#else
#define PRIf		      "d.%0" STRINGIFY(CONFIG_MATH_UTIL_PRIf_PRECISION) PRIu64
#define __FP_DECIMAL(n)	      ((n)&GENMASK(CONFIG_MATH_UTIL_FP_BITS - 1, 0))
#define __FP_SCALE(precision) ((uint64_t)DT_CAT(1e, precision))
#define PRIf_ARG(n)                                                                                \
	FP_TO_INT(n), ((__FP_DECIMAL((n & BIT(31)) == 0 ? n : (~n + 1)) *                          \
			__FP_SCALE(CONFIG_MATH_UTIL_PRIf_PRECISION)) /                             \
		       (1 << CONFIG_MATH_UTIL_FP_BITS))
#endif

#ifdef CONFIG_MATH_UTIL_FLOAT
typedef float fp_t;
typedef float fp_inter_t;
/* Conversion to/from fixed-point */
#define INT_TO_FP(x)   ((float)(x))
#define FP_TO_INT(x)   ((int32_t)(x))
/* Float to fixed-point, only for compile-time constants and unit tests */
#define FLOAT_TO_FP(x) ((float)(x))
/* Fixed-point to float, for unit tests */
#define FP_TO_FLOAT(x) ((float)(x))

#define FLT_MAX (3.4028234664e+38)
#define FLT_MIN (1.1754943508e-38)

#else
/* Fixed-point type */
typedef int32_t fp_t;

/* Type used during fp operation */
typedef int64_t fp_inter_t;

/* Conversion to/from fixed-point */
#define INT_TO_FP(x)   ((fp_t)(x) << CONFIG_MATH_UTIL_FP_BITS)
#define FP_TO_INT(x)   (((int32_t)((x) >> CONFIG_MATH_UTIL_FP_BITS)) + ((x & BIT(31)) == 0 ? 0 : 1))
/* Float to fixed-point, only for compile-time constants and unit tests */
#define FLOAT_TO_FP(x) ((fp_t)((x) * (float)(1 << CONFIG_MATH_UTIL_FP_BITS)))
/* Fixed-point to float, for unit tests */
#define FP_TO_FLOAT(x) ((float)(x) / (float)(1 << CONFIG_MATH_UTIL_FP_BITS))

#define FLT_MAX INT32_MAX
#define FLT_MIN INT32_MIN

#endif

/**
 * @brief Perform a floating/fixed point multiplication
 *
 * @param a The left side of the multiplication
 * @param b The right side of the multiplication
 * @return (a * b)
 */
static inline fp_t fp_mul(fp_t a, fp_t b)
{
#ifdef CONFIG_MATH_UTIL_FLOAT
	return a * b;
#else
	return (fp_t)(((fp_inter_t)a * b) >> CONFIG_MATH_UTIL_FP_BITS);
#endif
}

/**
 * @brief Perform a floating/fixed point division
 *
 * @param a The left side of the multiplication
 * @param b The right side of the multiplication
 * @return (a / b)
 */
static inline fp_t fp_div(fp_t a, fp_t b)
{
#ifdef CONFIG_MATH_UTIL_FLOAT
	return a / b;
#else
	return (fp_t)(((fp_inter_t)a << CONFIG_MATH_UTIL_FP_BITS) / b);
#endif
}

#ifdef __cplusplus
};
#endif

#endif /* INCLUDE_MATH_UTIL_H_ */
