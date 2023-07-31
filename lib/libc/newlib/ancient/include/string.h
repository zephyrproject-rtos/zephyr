/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_NEWLIB_ANCIENT_INCLUDE_STRING_H_
#define ZEPHYR_LIB_LIBC_NEWLIB_ANCIENT_INCLUDE_STRING_H_

#include <newlib.h>

#if defined(__NEWLIB_H__) && defined(__STRICT_ANSI__)
/* When using C standard without GNU extensions (e.g. --std=c99),
 * __STRICT_ANSI__ is defined, and various functions are then not
 * defined in the corresponding header file in ancient newlib.
 * This is due to compatibility with POSIX. Newer version of newlib
 * uses _POSIX_SOURCES instead of __STRICT_ANSI__ to avoid
 * re-declaring these functions.
 *
 * Workaround the issue by undefining __STRICT_ANSI__ to allow
 * those function prototypes to exist. This is definitely
 * a hack to get things working. But if ons is so determined
 * to use an ancient newlib, one needs to prepare to deal with
 * all nuisance.
 */
#undef __STRICT_ANSI__
#define _NEED_REDEFINE_STRICT_ANSI
#endif /* __NEWLIB__ && __STRICT_ANSI__ */

/* This should work on GCC and clang.
 *
 * If we need to support a toolchain without #include_next the CMake
 * infrastructure should be used to identify it and provide an
 * alternative solution.
 */
#include_next <string.h>

#ifdef _NEED_REDEFINE_STRICT_ANSI
/* Need to restore __STRICT_ANSI__ to, hopefully, avoid
 * any bigger side effects that we have already gotten
 * ourselves into.
 */
#define __STRICT_ANSI__ 1
#undef _NEED_REDEFINE_STRICT_ANSI
#endif

#endif /* ZEPHYR_LIB_LIBC_NEWLIB_ANCIENT_INCLUDE_STRING_H_ */
