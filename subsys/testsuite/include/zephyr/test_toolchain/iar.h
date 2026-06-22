/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_IAR_H_
#define ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_IAR_H_

#ifndef ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_H_
#error "Please do not include test toolchain-specific headers directly, \
use <zephyr/test_toolchain.h> instead"
#endif

#define TOOLCHAIN_WARNING_PRAGMAS            Pe161
#define TOOLCHAIN_WARNING_INFINITE_RECURSION Pe2440

#endif
