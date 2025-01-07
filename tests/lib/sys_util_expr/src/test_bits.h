/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_LIB_SYS_UTIL_EXPR_SRC_TEST_BITS_H__
#define TESTS_LIB_SYS_UTIL_EXPR_SRC_TEST_BITS_H__

#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#pragma GCC diagnostic ignored "-Woverflow"

#define TEST_BITS_0        EXPR_BITS_0X(0)
#define TEST_BITS_1        EXPR_BITS_0X(1)
#define TEST_BITS_2        EXPR_BITS_0X(2)
#define TEST_BITS_3        EXPR_BITS_0X(3)
#define TEST_BITS_7        EXPR_BITS_0X(7)
#define TEST_BITS_1E       EXPR_BITS_0X(1, e)
#define TEST_BITS_1F       EXPR_BITS_0X(1, f)
#define TEST_BITS_20       EXPR_BITS_0X(2, 0)
#define TEST_BITS_FFE      EXPR_BITS_0X(f, f, e)
#define TEST_BITS_FFF      EXPR_BITS_0X(f, f, f)
#define TEST_BITS_7FFFFFFF EXPR_BITS_0X(7, F, F, F, F, F, F, F)
#define TEST_BITS_FFFFFFFE EXPR_BITS_0X(F, F, F, F, F, F, F, E)
#define TEST_BITS_FFFFFFFF EXPR_BITS_0X(F, F, F, F, F, F, F, F)

#define Z_EXPR_BITS_2147483647U                                                                    \
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  \
		1, 1
#define Z_EXPR_BITS_2147483648U                                                                    \
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  \
		0, 0
#define Z_EXPR_BITS_2147483649U                                                                    \
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  \
		0, 1
#define Z_EXPR_BITS_4294967288U                                                                    \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,  \
		0, 0
#define Z_EXPR_BITS_4294967289U                                                                    \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,  \
		0, 1
#define Z_EXPR_BITS_4294967293U                                                                    \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  \
		0, 1
#define Z_EXPR_BITS_4294967294U                                                                    \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  \
		1, 0
#define Z_EXPR_BITS_4294967295U                                                                    \
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  \
		1, 1

#define Z_EXPR_BIN_TO_DEC_0B00000000000000000001000110010100 4500
#define Z_EXPR_BIN_TO_DEC_0B00000000000000000110110101100000 28000
#define Z_EXPR_BIN_TO_DEC_0B00000000000000000110111111010110 28630
#define Z_EXPR_BIN_TO_DEC_0B00000000000111101000010010000000 2000000
#define Z_EXPR_BIN_TO_DEC_0B00000000001111111111100000000000 4192256
#define Z_EXPR_BIN_TO_DEC_0B01111111111111111111111111111111 2147483647
#define Z_EXPR_BIN_TO_DEC_0B10000000000000000000000000000000 2147483648
#define Z_EXPR_BIN_TO_DEC_0B10000000000000000000000000000001 2147483649
#define Z_EXPR_BIN_TO_DEC_0B11111111111111111111000000001101 4294963213
#define Z_EXPR_BIN_TO_DEC_0B11111111111111111111100000000000 4294965248
#define Z_EXPR_BIN_TO_DEC_0B11111111111111111111111111111000 4294967288
#define Z_EXPR_BIN_TO_DEC_0B11111111111111111111111111111001 4294967289
#define Z_EXPR_BIN_TO_DEC_0B11111111111111111111111111111101 4294967293
#define Z_EXPR_BIN_TO_DEC_0B11111111111111111111111111111110 4294967294
#define Z_EXPR_BIN_TO_DEC_0B11111111111111111111111111111111 4294967295

#endif /* TESTS_LIB_SYS_UTIL_EXPR_SRC_TEST_BITS_H__ */
