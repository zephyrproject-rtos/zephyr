/*
 * Copyright (c) 2019 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_STDINT_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_STDINT_H_

/*
 * Some gcc versions and/or configurations as found in the Zephyr SDK
 * (questionably) define __INT32_TYPE__ and derivatives as a long int
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
#undef __INT_FAST32_TYPE__
#undef __UINT_FAST32_TYPE__
#undef __INT_LEAST32_TYPE__
#undef __UINT_LEAST32_TYPE__
#undef __INT64_TYPE__
#undef __UINT64_TYPE__
#undef __INT_FAST64_TYPE__
#undef __UINT_FAST64_TYPE__
#undef __INT_LEAST64_TYPE__
#undef __UINT_LEAST64_TYPE__

#define __INT32_TYPE__ int
#define __UINT32_TYPE__ unsigned int
#define __INT_FAST32_TYPE__ __INT32_TYPE__
#define __UINT_FAST32_TYPE__ __UINT32_TYPE__
#define __INT_LEAST32_TYPE__ __INT32_TYPE__
#define __UINT_LEAST32_TYPE__ __UINT32_TYPE__
#define __INT64_TYPE__ long long int
#define __UINT64_TYPE__ unsigned long long int
#define __INT_FAST64_TYPE__ __INT64_TYPE__
#define __UINT_FAST64_TYPE__ __UINT64_TYPE__
#define __INT_LEAST64_TYPE__ __INT64_TYPE__
#define __UINT_LEAST64_TYPE__ __UINT64_TYPE__

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

/*
 * Re-define the INTN_C(value) integer constant expression macros to match the
 * integer types re-defined above.
 */

#undef __INT32_C
#undef __UINT32_C
#undef __INT64_C
#undef __UINT64_C
#define __INT32_C(c) c
#define __UINT32_C(c) c ## U
#define __INT64_C(c) c ## LL
#define __UINT64_C(c) c ## ULL

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_STDINT_H_ */
