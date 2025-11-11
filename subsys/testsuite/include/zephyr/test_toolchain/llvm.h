/*
 * Copyright (c) 2025 Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_LLVM_H_
#define ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_LLVM_H_

#ifndef ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_H_
#error "Please do not include test toolchain-specific headers directly, \
use <zephyr/test_toolchain.h> instead"
#endif

#include <zephyr/test_toolchain/gcc.h>

#define TOOLCHAIN_WARNING_INTEGER_OVERFLOW "-Winteger-overflow"

#endif
