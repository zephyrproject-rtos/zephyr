/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_MATH_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_MATH_H_

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_MATH_H_ */
