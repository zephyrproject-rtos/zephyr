/*
 * Copyright (c) 2025 Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_GCC_H_
#define ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_GCC_H_

#ifndef ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_H_
#error "Please do not include test toolchain-specific headers directly, \
use <zephyr/test_toolchain.h> instead"
#endif

#define TOOLCHAIN_WARNING_DANGLING_POINTER   "-Wdangling-pointer"
#define TOOLCHAIN_WARNING_FORMAT_TRUNCATION  "-Wformat-truncation"
#define TOOLCHAIN_WARNING_INFINITE_RECURSION "-Winfinite-recursion"
#define TOOLCHAIN_WARNING_OVERFLOW           "-Woverflow"
#define TOOLCHAIN_WARNING_PRAGMAS            "-Wpragmas"
#define TOOLCHAIN_WARNING_UNUSED_FUNCTION    "-Wunused-function"

/* GCC-specific warnings that aren't in clang. */
#if defined(__GNUC__) && !defined(__clang__)
#define TOOLCHAIN_WARNING_ALLOC_SIZE_LARGER_THAN "-Walloc-size-larger-than="
#define TOOLCHAIN_WARNING_STRINGOP_OVERFLOW      "-Wstringop-overflow"
#define TOOLCHAIN_WARNING_STRINGOP_TRUNCATION    "-Wstringop-truncation"
#endif

#endif
