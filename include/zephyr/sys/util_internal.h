/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Misc utilities
 *
 * Repetitive or obscure helper macros needed by sys/util.h.
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_

#include "util_loops.h"

/* IS_ENABLED() helpers */

/* This is called from IS_ENABLED(), and sticks on a "_XXXX" prefix,
 * it will now be "_XXXX1" if config_macro is "1", or just "_XXXX" if it's
 * undefined.
 *   ENABLED:   Z_IS_ENABLED2(_XXXX1)
 *   DISABLED   Z_IS_ENABLED2(_XXXX)
 */
#define Z_IS_ENABLED1(config_macro) Z_IS_ENABLED2(_XXXX##config_macro)

/* Here's the core trick, we map "_XXXX1" to "_YYYY," (i.e. a string
 * with a trailing comma), so it has the effect of making this a
 * two-argument tuple to the preprocessor only in the case where the
 * value is defined to "1"
 *   ENABLED:    _YYYY,    <--- note comma!
 *   DISABLED:   _XXXX
 */
#define _XXXX1 _YYYY,

/* Then we append an extra argument to fool the gcc preprocessor into
 * accepting it as a varargs macro.
 *                         arg1   arg2  arg3
 *   ENABLED:   Z_IS_ENABLED3(_YYYY,    1,    0)
 *   DISABLED   Z_IS_ENABLED3(_XXXX 1,  0)
 */
#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args 1, 0)

/* And our second argument is thus now cooked to be 1 in the case
 * where the value is defined to 1, and 0 if not:
 */
#define Z_IS_ENABLED3(ignore_this, val, ...) val

/* Helper macro for IS_ENABLED_ALL */
#define Z_IS_ENABLED_ALL(...) \
	Z_IS_ENABLED_ALL_N(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__)
#define Z_IS_ENABLED_ALL_N(N, ...) UTIL_CAT(Z_IS_ENABLED_ALL_, N)(__VA_ARGS__)
#define Z_IS_ENABLED_ALL_0(...)
#define Z_IS_ENABLED_ALL_1(a, ...)  COND_CODE_1(a, (1), (0))
#define Z_IS_ENABLED_ALL_2(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_1(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_3(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_2(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_4(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_3(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_5(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_4(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_6(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_5(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_7(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_6(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_8(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_7(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_9(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_8(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_10(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_9(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_11(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_10(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_12(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_11(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_13(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_12(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_14(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_13(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_15(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_14(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_16(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_15(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_17(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_16(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_18(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_17(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_19(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_18(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_20(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_19(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_21(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_20(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_22(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_21(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_23(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_22(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_24(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_23(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_25(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_24(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_26(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_25(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_27(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_26(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_28(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_27(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_29(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_28(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_30(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_29(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_31(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_30(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_32(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_31(__VA_ARGS__,)), (0))

/* Helper macro for IS_ENABLED_ANY */
#define Z_IS_ENABLED_ANY(...) \
	Z_IS_ENABLED_ANY_N(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__)
#define Z_IS_ENABLED_ANY_N(N, ...) UTIL_CAT(Z_IS_ENABLED_ANY_, N)(__VA_ARGS__)
#define Z_IS_ENABLED_ANY_0(...)
#define Z_IS_ENABLED_ANY_1(a, ...)  COND_CODE_1(a, (1), (0))
#define Z_IS_ENABLED_ANY_2(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_1(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_3(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_2(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_4(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_3(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_5(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_4(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_6(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_5(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_7(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_6(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_8(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_7(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_9(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_8(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_10(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_9(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_11(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_10(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_12(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_11(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_13(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_12(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_14(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_13(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_15(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_14(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_16(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_15(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_17(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_16(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_18(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_17(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_19(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_18(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_20(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_19(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_21(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_20(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_22(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_21(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_23(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_22(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_24(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_23(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_25(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_24(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_26(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_25(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_27(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_26(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_28(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_27(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_29(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_28(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_30(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_29(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_31(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_30(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_32(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_31(__VA_ARGS__,)))

/* Helper macro for IF_ENABLED_ALL */
#define Z_IF_ENABLED_ALL(_code, ...) \
	Z_IF_ENABLED_ALL_N(NUM_VA_ARGS(__VA_ARGS__), _code, __VA_ARGS__)
#define Z_IF_ENABLED_ALL_N(N, _code, ...) \
	UTIL_CAT(Z_IF_ENABLED_ALL_, N)(_code, __VA_ARGS__)
#define IF_ENABLED_ALL_0(_code, ...)
#define Z_IF_ENABLED_ALL_1(_code, a, ...) \
	COND_CODE_1(a, _code, ())
#define Z_IF_ENABLED_ALL_2(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_1(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_3(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_2(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_4(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_3(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_5(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_4(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_6(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_5(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_7(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_6(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_8(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_7(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_9(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_8(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_10(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_9(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_11(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_10(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_12(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_11(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_13(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_12(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_14(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_13(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_15(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_14(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_16(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_15(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_17(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_16(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_18(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_17(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_19(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_18(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_20(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_19(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_21(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_20(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_22(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_21(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_23(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_22(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_24(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_23(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_25(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_24(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_26(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_25(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_27(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_26(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_28(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_27(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_29(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_28(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_30(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_29(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_31(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_30(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_32(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_31(_code, __VA_ARGS__,)), ())

/* Helper macro for IF_ENABLED_ANY */
#define Z_IF_ENABLED_ANY(_code, ...) \
	Z_IF_ENABLED_ANY_N(NUM_VA_ARGS(__VA_ARGS__), _code, __VA_ARGS__)
#define Z_IF_ENABLED_ANY_N(N, _code, ...) \
	UTIL_CAT(Z_IF_ENABLED_ANY_, N)(_code, __VA_ARGS__)
#define Z_IF_ENABLED_ANY_0(_code, ...)
#define Z_IF_ENABLED_ANY_1(_code, a, ...) \
	COND_CODE_1(a, _code, ())
#define Z_IF_ENABLED_ANY_2(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_1(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_3(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_2(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_4(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_3(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_5(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_4(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_6(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_5(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_7(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_6(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_8(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_7(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_9(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_8(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_10(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_9(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_11(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_10(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_12(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_11(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_13(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_12(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_14(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_13(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_15(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_14(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_16(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_15(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_17(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_16(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_18(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_17(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_19(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_18(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_20(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_19(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_21(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_20(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_22(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_21(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_23(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_22(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_24(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_23(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_25(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_24(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_26(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_25(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_27(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_26(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_28(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_27(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_29(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_28(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_30(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_29(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_31(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_30(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_32(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_31(_code, __VA_ARGS__,)))

/* Implementation of IS_EQ(). Returns 1 if _0 and _1 are the same integer from
 * 0 to 4096, 0 otherwise.
 */
#define Z_IS_EQ(_0, _1) Z_HAS_COMMA(Z_CAT4(Z_IS_, _0, _EQ_, _1)())

/* Used internally by COND_CODE_1 and COND_CODE_0. */
#define Z_COND_CODE_1(_flag, _if_1_code, _else_code) \
	__COND_CODE(_XXXX##_flag, _if_1_code, _else_code)
#define Z_COND_CODE_0(_flag, _if_0_code, _else_code) \
	__COND_CODE(_ZZZZ##_flag, _if_0_code, _else_code)
#define _ZZZZ0 _YYYY,
#define __COND_CODE(one_or_two_args, _if_code, _else_code) \
	__GET_ARG2_DEBRACKET(one_or_two_args _if_code, _else_code)

/* Gets second argument and removes brackets around that argument. It
 * is expected that the parameter is provided in brackets/parentheses.
 */
#define __GET_ARG2_DEBRACKET(ignore_this, val, ...) __DEBRACKET val

/* Used to remove brackets from around a single argument. */
#define __DEBRACKET(...) __VA_ARGS__

/* Used by IS_EMPTY() */
/* reference: https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments/ */
#define Z_HAS_COMMA(...) \
	NUM_VA_ARGS_LESS_1_IMPL(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, \
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define Z_TRIGGER_PARENTHESIS_(...) ,
#define Z_IS_EMPTY_(...) \
	Z_IS_EMPTY__( \
		Z_HAS_COMMA(__VA_ARGS__), \
		Z_HAS_COMMA(Z_TRIGGER_PARENTHESIS_ __VA_ARGS__), \
		Z_HAS_COMMA(__VA_ARGS__ (/*empty*/)), \
		Z_HAS_COMMA(Z_TRIGGER_PARENTHESIS_ __VA_ARGS__ (/*empty*/)))
#define Z_CAT4(_0, _1, _2, _3) _0 ## _1 ## _2 ## _3
#define Z_CAT5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define Z_IS_EMPTY__(_0, _1, _2, _3) \
	Z_HAS_COMMA(Z_CAT5(Z_IS_EMPTY_CASE_, _0, _1, _2, _3))
#define Z_IS_EMPTY_CASE_0001 ,

/* Used by LIST_DROP_EMPTY() */
/* Adding ',' after each element would add empty element at the end of
 * list, which is hard to remove, so instead precede each element with ',',
 * this way first element is empty, and this one is easy to drop.
 */
#define Z_LIST_ADD_ELEM(e) EMPTY, e
#define Z_LIST_DROP_FIRST(...) GET_ARGS_LESS_N(1, __VA_ARGS__)
#define Z_LIST_NO_EMPTIES(e) \
	COND_CODE_1(IS_EMPTY(e), (), (Z_LIST_ADD_ELEM(e)))

#define UTIL_CAT(a, ...) UTIL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define UTIL_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__
#define UTIL_CHECK_N(x, n, ...) n
#define UTIL_CHECK(...) UTIL_CHECK_N(__VA_ARGS__, 0,)
#define UTIL_NOT(x) UTIL_CHECK(UTIL_PRIMITIVE_CAT(UTIL_NOT_, x))
#define UTIL_NOT_0 ~, 1,
#define UTIL_COMPL(b) UTIL_PRIMITIVE_CAT(UTIL_COMPL_, b)
#define UTIL_COMPL_0 1
#define UTIL_COMPL_1 0
#define UTIL_BOOL(x) UTIL_COMPL(UTIL_NOT(x))

#define UTIL_EVAL(...) __VA_ARGS__
#define UTIL_EXPAND(...) __VA_ARGS__
#define UTIL_REPEAT(...) UTIL_LISTIFY(__VA_ARGS__)

#define UTIL_AND_CAT(a, b) a && b
/* Used by UTIL_CONCAT_AND */
#define Z_UTIL_CONCAT_AND(...) \
	(Z_UTIL_CONCAT_AND_N(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__))
#define Z_UTIL_CONCAT_AND_N(N, ...) UTIL_CAT(Z_UTIL_CONCAT_AND_, N)(__VA_ARGS__)
#define Z_UTIL_CONCAT_AND_0
#define Z_UTIL_CONCAT_AND_1(a, ...)  a
#define Z_UTIL_CONCAT_AND_2(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_1(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_3(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_2(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_4(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_3(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_5(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_4(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_6(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_5(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_7(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_6(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_8(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_7(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_9(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_8(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_10(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_9(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_11(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_10(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_12(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_11(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_13(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_12(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_14(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_13(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_15(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_14(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_16(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_15(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_17(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_16(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_18(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_17(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_19(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_18(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_20(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_19(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_21(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_20(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_22(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_21(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_23(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_22(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_24(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_23(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_25(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_24(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_26(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_25(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_27(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_26(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_28(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_27(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_29(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_28(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_30(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_29(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_31(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_30(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_32(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_31(__VA_ARGS__,))

#define UTIL_OR_CAT(a, b) a || b
/* Used by UTIL_CONCAT_OR */
#define Z_UTIL_CONCAT_OR(...) \
	(Z_UTIL_CONCAT_OR_N(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__))
#define Z_UTIL_CONCAT_OR_N(N, ...) UTIL_CAT(Z_UTIL_CONCAT_OR_, N)(__VA_ARGS__)
#define Z_UTIL_CONCAT_OR_0
#define Z_UTIL_CONCAT_OR_1(a, ...)  a
#define Z_UTIL_CONCAT_OR_2(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_1(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_3(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_2(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_4(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_3(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_5(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_4(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_6(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_5(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_7(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_6(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_8(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_7(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_9(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_8(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_10(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_9(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_11(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_10(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_12(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_11(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_13(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_12(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_14(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_13(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_15(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_14(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_16(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_15(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_17(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_16(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_18(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_17(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_19(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_18(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_20(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_19(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_21(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_20(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_22(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_21(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_23(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_22(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_24(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_23(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_25(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_24(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_26(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_25(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_27(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_26(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_28(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_27(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_29(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_28(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_30(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_29(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_31(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_30(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_32(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_31(__VA_ARGS__,))

#define _CONCAT_0(arg, ...) arg
#define _CONCAT_1(arg, ...) UTIL_CAT(arg, _CONCAT_0(__VA_ARGS__))
#define _CONCAT_2(arg, ...) UTIL_CAT(arg, _CONCAT_1(__VA_ARGS__))
#define _CONCAT_3(arg, ...) UTIL_CAT(arg, _CONCAT_2(__VA_ARGS__))
#define _CONCAT_4(arg, ...) UTIL_CAT(arg, _CONCAT_3(__VA_ARGS__))
#define _CONCAT_5(arg, ...) UTIL_CAT(arg, _CONCAT_4(__VA_ARGS__))
#define _CONCAT_6(arg, ...) UTIL_CAT(arg, _CONCAT_5(__VA_ARGS__))
#define _CONCAT_7(arg, ...) UTIL_CAT(arg, _CONCAT_6(__VA_ARGS__))

/* Implementation details for NUM_VA_ARGS_LESS_1 */
#define NUM_VA_ARGS_LESS_1_IMPL(				\
	_ignored,						\
	_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,		\
	_11, _12, _13, _14, _15, _16, _17, _18, _19, _20,	\
	_21, _22, _23, _24, _25, _26, _27, _28, _29, _30,	\
	_31, _32, _33, _34, _35, _36, _37, _38, _39, _40,	\
	_41, _42, _43, _44, _45, _46, _47, _48, _49, _50,	\
	_51, _52, _53, _54, _55, _56, _57, _58, _59, _60,	\
	_61, _62, N, ...) N

/* Used by MACRO_MAP_CAT */
#define MACRO_MAP_CAT_(...)						\
	/* To make sure it works also for 2 arguments in total */	\
	MACRO_MAP_CAT_N(NUM_VA_ARGS_LESS_1(__VA_ARGS__), __VA_ARGS__)
#define MACRO_MAP_CAT_N_(N, ...) UTIL_CAT(MACRO_MC_, N)(__VA_ARGS__,)
#define MACRO_MC_0(...)
#define MACRO_MC_1(m, a, ...)  m(a)
#define MACRO_MC_2(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_1(m, __VA_ARGS__,))
#define MACRO_MC_3(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_2(m, __VA_ARGS__,))
#define MACRO_MC_4(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_3(m, __VA_ARGS__,))
#define MACRO_MC_5(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_4(m, __VA_ARGS__,))
#define MACRO_MC_6(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_5(m, __VA_ARGS__,))
#define MACRO_MC_7(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_6(m, __VA_ARGS__,))
#define MACRO_MC_8(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_7(m, __VA_ARGS__,))
#define MACRO_MC_9(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_8(m, __VA_ARGS__,))
#define MACRO_MC_10(m, a, ...) UTIL_CAT(m(a), MACRO_MC_9(m, __VA_ARGS__,))
#define MACRO_MC_11(m, a, ...) UTIL_CAT(m(a), MACRO_MC_10(m, __VA_ARGS__,))
#define MACRO_MC_12(m, a, ...) UTIL_CAT(m(a), MACRO_MC_11(m, __VA_ARGS__,))
#define MACRO_MC_13(m, a, ...) UTIL_CAT(m(a), MACRO_MC_12(m, __VA_ARGS__,))
#define MACRO_MC_14(m, a, ...) UTIL_CAT(m(a), MACRO_MC_13(m, __VA_ARGS__,))
#define MACRO_MC_15(m, a, ...) UTIL_CAT(m(a), MACRO_MC_14(m, __VA_ARGS__,))

/* Used by CASE_IF_ENABLED */
#define Z_CASE_IF_ENABLED(_flag, _label, ...) \
	Z_CASE_IF_ENABLED_ALL((_flag), _label, __VA_ARGS__)

/* Used by CASE_IF_ENABLED_ALL */
#define Z_CASE_IF_ENABLED_ALL(_flags, _label, ...) \
	Z_IF_ENABLED_ALL((case _label: { __VA_ARGS__ }), __DEBRACKET _flags)

/* Used by CASE_IF_ENABLED_ANY */
#define Z_CASE_IF_ENABLED_ANY(_flags, _label, ...) \
	Z_IF_ENABLED_ANY((case _label: { __VA_ARGS__ }), __DEBRACKET _flags)

/* Used by Z_IS_EQ */
#include "util_internal_is_eq.h"

/*
 * Generic sparse list of odd numbers (check the implementation of
 * GPIO_DT_RESERVED_RANGES_NGPIOS as a usage example)
 */
#define Z_SPARSE_LIST_ODD_NUMBERS		\
	EMPTY,  1, EMPTY,  3, EMPTY,  5, EMPTY,  7, \
	EMPTY,  9, EMPTY, 11, EMPTY, 13, EMPTY, 15, \
	EMPTY, 17, EMPTY, 19, EMPTY, 21, EMPTY, 23, \
	EMPTY, 25, EMPTY, 27, EMPTY, 29, EMPTY, 31, \
	EMPTY, 33, EMPTY, 35, EMPTY, 37, EMPTY, 39, \
	EMPTY, 41, EMPTY, 43, EMPTY, 45, EMPTY, 47, \
	EMPTY, 49, EMPTY, 51, EMPTY, 53, EMPTY, 55, \
	EMPTY, 57, EMPTY, 59, EMPTY, 61, EMPTY, 63

/*
 * Generic sparse list of even numbers (check the implementation of
 * GPIO_DT_RESERVED_RANGES_NGPIOS as a usage example)
 */
#define Z_SPARSE_LIST_EVEN_NUMBERS		\
	 0, EMPTY,  2, EMPTY,  4, EMPTY,  6, EMPTY, \
	 8, EMPTY, 10, EMPTY, 12, EMPTY, 14, EMPTY, \
	16, EMPTY, 18, EMPTY, 20, EMPTY, 22, EMPTY, \
	24, EMPTY, 26, EMPTY, 28, EMPTY, 30, EMPTY, \
	32, EMPTY, 34, EMPTY, 36, EMPTY, 38, EMPTY, \
	40, EMPTY, 42, EMPTY, 44, EMPTY, 46, EMPTY, \
	48, EMPTY, 50, EMPTY, 52, EMPTY, 54, EMPTY, \
	56, EMPTY, 58, EMPTY, 60, EMPTY, 62, EMPTY

/* Used by UTIL_INC */
#include "util_internal_util_inc.h"

/* Used by UTIL_DEC */
#include "util_internal_util_dec.h"

/* Used by UTIL_X2 */
#include "util_internal_util_x2.h"

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_ */
