/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_UTILS_H
#define NSI_COMMON_SRC_INCL_NSI_UTILS_H

/* Remove brackets from around a single argument: */
#define NSI_DEBRACKET(...) __VA_ARGS__

#define _NSI_STRINGIFY(x) #x
#define NSI_STRINGIFY(s) _NSI_STRINGIFY(s)

/* concatenate the values of the arguments into one */
#define NSI_DO_CONCAT(x, y) x ## y
#define NSI_CONCAT(x, y) NSI_DO_CONCAT(x, y)

#define NSI_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define NSI_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define NSI_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#ifndef NSI_ARG_UNUSED
#define NSI_ARG_UNUSED(x) (void)(x)
#endif

#define NSI_CODE_UNREACHABLE __builtin_unreachable()

#define NSI_FUNC_NORETURN __attribute__((__noreturn__))
#define NSI_WEAK __attribute__((__weak__))

#if defined(__clang__)
  /* The address sanitizer in llvm adds padding (redzones) after data
   * But for those we are re-grouping using the linker script
   * we cannot have that extra padding as we intend to iterate over them
   */
#define NSI_NOASAN __attribute__((no_sanitize("address")))
#else
#define NSI_NOASAN
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_UTILS_H */
