/*
 * Copyright (c) 2019 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_STDINT_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_STDINT_H_

/*
 * Some gcc versions and/or configurations as found in the Zephyr SDK
 * (questionably) define __INT32_TYPE__ and derrivatives as a long int
 * which makes the printf format checker to complain about long vs int
 * mismatch when %u is given a uint32_t argument, and uint32_t pointers not
 * being compatible with int pointers. Let's redefine them to follow
 * common expectations and usage.
 */

#if __SIZEOF_INT__ != 4
#error "unexpected int width"
#endif

#undef __INT32_TYPE__
#undef __UINT32_TYPE__
#undef __INT_LEAST32_TYPE__
#undef __UINT_LEAST32_TYPE__
#undef __INT64_TYPE__
#undef __UINT64_TYPE__
#define __INT32_TYPE__ int
#define __UINT32_TYPE__ unsigned int
#define __INT_LEAST32_TYPE__ __INT32_TYPE__
#define __UINT_LEAST32_TYPE__ __UINT32_TYPE__
#define __INT64_TYPE__ long long int
#define __UINT64_TYPE__ unsigned long long int

/*
 * The confusion also exists with __INTPTR_TYPE__ which is either an int
 * (even when __INT32_TYPE__ is a long int) or a long int. Let's redefine
 * it to a long int to get some uniformity. Doing so also makes it compatible
 * with LP64 (64-bit) targets where a long is always 64-bit wide.
 */

#if __SIZEOF_POINTER__ != __SIZEOF_LONG__
#error "unexpected size difference between pointers and long ints"
#endif

#undef __INTPTR_TYPE__
#undef __UINTPTR_TYPE__
#define __INTPTR_TYPE__ long int
#define __UINTPTR_TYPE__ long unsigned int

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_STDINT_H_ */
