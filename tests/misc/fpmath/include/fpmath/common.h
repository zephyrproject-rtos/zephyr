/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_MISC_FPMATH_INCLUDE_FPMATH_COMMON_H_
#define TESTS_MISC_FPMATH_INCLUDE_FPMATH_COMMON_H_

#define qassert(expected, qval)                                                                    \
	zassert_equal((uint32_t)(expected), (uint32_t)((qval).value),                              \
		      "Expected 0x%08x, but got 0x%08x", (uint32_t)(expected),                     \
		      (uint32_t)((qval).value))

#endif /* TESTS_MISC_FPMATH_INCLUDE_FPMATH_COMMON_H_ */
