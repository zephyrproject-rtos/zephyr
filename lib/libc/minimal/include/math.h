/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_MATH_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_MATH_H_

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(FLT_EVAL_METHOD) && defined(__FLT_EVAL_METHOD__)
#define FLT_EVAL_METHOD __FLT_EVAL_METHOD__
#endif

#if defined FLT_EVAL_METHOD
	#if FLT_EVAL_METHOD == 0
		typedef float  float_t;
		typedef double double_t;
	#elif FLT_EVAL_METHOD == 1
		typedef double float_t;
		typedef double double_t;
	#elif FLT_EVAL_METHOD == 2
		typedef long double float_t;
		typedef long double double_t;
	#else
		/* Implementation-defined.  Assume float_t and double_t have
		 * already been defined */
	#endif
#else
	typedef float  float_t;
	typedef double double_t;
#endif

/* Useful constants.  */
#define MAXFLOAT    3.40282347e+38F

#define M_E         2.7182818284590452354
#define M_LOG2E     1.4426950408889634074
#define M_LOG10E    0.43429448190325182765
#define M_LN2       0.693147180559945309417
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_1_PI      0.31830988618379067154
#define M_2_PI      0.63661977236758134308
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440

/* Classification macros, for the compilers that have them as builtins */
#if HAS_BUILTIN(__builtin_fpclassify)
#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4
#define fpclassify(x)                                                                              \
	__builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x)
#endif
#if HAS_BUILTIN(__builtin_isfinite)
#define isfinite(x) __builtin_isfinite(x)
#endif
#if HAS_BUILTIN(__builtin_isinf)
#define isinf(x) __builtin_isinf(x)
#endif
#if HAS_BUILTIN(__builtin_isnan)
#define isnan(x) __builtin_isnan(x)
#endif
#if HAS_BUILTIN(__builtin_isnormal)
#define isnormal(x) __builtin_isnormal(x)
#endif
#if HAS_BUILTIN(__builtin_signbit)
#define signbit(x) __builtin_signbit(x)
#endif

/* C23 classification macros */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ > 201710L)
#if HAS_BUILTIN(__builtin_issignaling)
#define issignaling(x) __builtin_issignaling(x)
#endif
#if HAS_BUILTIN(__builtin_issubnormal)
#define issubnormal(x) __builtin_issubnormal(x)
#endif
#if HAS_BUILTIN(__builtin_iszero)
#define iszero(x) __builtin_iszero(x)
#endif
#endif

float sqrtf(float square);
double sqrt(double square);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_MATH_H_ */
