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

#define _CONCAT_0(arg, ...) arg
#define _CONCAT_1(arg, ...) UTIL_CAT(arg, _CONCAT_0(__VA_ARGS__))
#define _CONCAT_2(arg, ...) UTIL_CAT(arg, _CONCAT_1(__VA_ARGS__))
#define _CONCAT_3(arg, ...) UTIL_CAT(arg, _CONCAT_2(__VA_ARGS__))
#define _CONCAT_4(arg, ...) UTIL_CAT(arg, _CONCAT_3(__VA_ARGS__))
#define _CONCAT_5(arg, ...) UTIL_CAT(arg, _CONCAT_4(__VA_ARGS__))
#define _CONCAT_6(arg, ...) UTIL_CAT(arg, _CONCAT_5(__VA_ARGS__))
#define _CONCAT_7(arg, ...) UTIL_CAT(arg, _CONCAT_6(__VA_ARGS__))
#define _CONCAT_8(arg, ...)  UTIL_CAT(arg, _CONCAT_7(__VA_ARGS__))
#define _CONCAT_9(arg, ...)  UTIL_CAT(arg, _CONCAT_8(__VA_ARGS__))
#define _CONCAT_10(arg, ...) UTIL_CAT(arg, _CONCAT_9(__VA_ARGS__))
#define _CONCAT_11(arg, ...) UTIL_CAT(arg, _CONCAT_10(__VA_ARGS__))
#define _CONCAT_12(arg, ...) UTIL_CAT(arg, _CONCAT_11(__VA_ARGS__))

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

#define Z_UTIL_INC_0  1
#define Z_UTIL_INC_1  2
#define Z_UTIL_INC_2  3
#define Z_UTIL_INC_3  4
#define Z_UTIL_INC_4  5
#define Z_UTIL_INC_5  6
#define Z_UTIL_INC_6  7
#define Z_UTIL_INC_7  8
#define Z_UTIL_INC_8  9
#define Z_UTIL_INC_9  10
#define Z_UTIL_INC_10 11
#define Z_UTIL_INC_11 12
#define Z_UTIL_INC_12 13
#define Z_UTIL_INC_13 14
#define Z_UTIL_INC_14 15
#define Z_UTIL_INC_15 16
#define Z_UTIL_INC_16 17
#define Z_UTIL_INC_17 18
#define Z_UTIL_INC_18 19
#define Z_UTIL_INC_19 20
#define Z_UTIL_INC_20 21
#define Z_UTIL_INC_21 22
#define Z_UTIL_INC_22 23
#define Z_UTIL_INC_23 24
#define Z_UTIL_INC_24 25
#define Z_UTIL_INC_25 26
#define Z_UTIL_INC_26 27
#define Z_UTIL_INC_27 28
#define Z_UTIL_INC_28 29
#define Z_UTIL_INC_29 30
#define Z_UTIL_INC_30 31
#define Z_UTIL_INC_31 32
#define Z_UTIL_INC_32 33
#define Z_UTIL_INC_33 34
#define Z_UTIL_INC_34 35
#define Z_UTIL_INC_35 36
#define Z_UTIL_INC_36 37
#define Z_UTIL_INC_37 38
#define Z_UTIL_INC_38 39
#define Z_UTIL_INC_39 40
#define Z_UTIL_INC_40 41
#define Z_UTIL_INC_41 42
#define Z_UTIL_INC_42 43
#define Z_UTIL_INC_43 44
#define Z_UTIL_INC_44 45
#define Z_UTIL_INC_45 46
#define Z_UTIL_INC_46 47
#define Z_UTIL_INC_47 48
#define Z_UTIL_INC_48 49
#define Z_UTIL_INC_49 50
#define Z_UTIL_INC_50 51
#define Z_UTIL_INC_51 52
#define Z_UTIL_INC_52 53
#define Z_UTIL_INC_53 54
#define Z_UTIL_INC_54 55
#define Z_UTIL_INC_55 56
#define Z_UTIL_INC_56 57
#define Z_UTIL_INC_57 58
#define Z_UTIL_INC_58 59
#define Z_UTIL_INC_59 60
#define Z_UTIL_INC_60 61
#define Z_UTIL_INC_61 62
#define Z_UTIL_INC_62 63
#define Z_UTIL_INC_63 64
#define Z_UTIL_INC_64 65

#define Z_UTIL_INC(x) UTIL_PRIMITIVE_CAT(Z_UTIL_INC_, x)

#define Z_UTIL_DEC_0  0
#define Z_UTIL_DEC_1  0
#define Z_UTIL_DEC_2  1
#define Z_UTIL_DEC_3  2
#define Z_UTIL_DEC_4  3
#define Z_UTIL_DEC_5  4
#define Z_UTIL_DEC_6  5
#define Z_UTIL_DEC_7  6
#define Z_UTIL_DEC_8  7
#define Z_UTIL_DEC_9  8
#define Z_UTIL_DEC_10 9
#define Z_UTIL_DEC_11 10
#define Z_UTIL_DEC_12 11
#define Z_UTIL_DEC_13 12
#define Z_UTIL_DEC_14 13
#define Z_UTIL_DEC_15 14
#define Z_UTIL_DEC_16 15
#define Z_UTIL_DEC_17 16
#define Z_UTIL_DEC_18 17
#define Z_UTIL_DEC_19 18
#define Z_UTIL_DEC_20 19
#define Z_UTIL_DEC_21 20
#define Z_UTIL_DEC_22 21
#define Z_UTIL_DEC_23 22
#define Z_UTIL_DEC_24 23
#define Z_UTIL_DEC_25 24
#define Z_UTIL_DEC_26 25
#define Z_UTIL_DEC_27 26
#define Z_UTIL_DEC_28 27
#define Z_UTIL_DEC_29 28
#define Z_UTIL_DEC_30 29
#define Z_UTIL_DEC_31 30
#define Z_UTIL_DEC_32 31
#define Z_UTIL_DEC_33 32
#define Z_UTIL_DEC_34 33
#define Z_UTIL_DEC_35 34
#define Z_UTIL_DEC_36 35
#define Z_UTIL_DEC_37 36
#define Z_UTIL_DEC_38 37
#define Z_UTIL_DEC_39 38
#define Z_UTIL_DEC_40 39
#define Z_UTIL_DEC_41 40
#define Z_UTIL_DEC_42 41
#define Z_UTIL_DEC_43 42
#define Z_UTIL_DEC_44 43
#define Z_UTIL_DEC_45 44
#define Z_UTIL_DEC_46 45
#define Z_UTIL_DEC_47 46
#define Z_UTIL_DEC_48 47
#define Z_UTIL_DEC_49 48
#define Z_UTIL_DEC_50 49
#define Z_UTIL_DEC_51 50
#define Z_UTIL_DEC_52 51
#define Z_UTIL_DEC_53 52
#define Z_UTIL_DEC_54 53
#define Z_UTIL_DEC_55 54
#define Z_UTIL_DEC_56 55
#define Z_UTIL_DEC_57 56
#define Z_UTIL_DEC_58 57
#define Z_UTIL_DEC_59 58
#define Z_UTIL_DEC_60 59
#define Z_UTIL_DEC_61 60
#define Z_UTIL_DEC_62 61
#define Z_UTIL_DEC_63 62
#define Z_UTIL_DEC_64 63

#define Z_UTIL_DEC(x) UTIL_PRIMITIVE_CAT(Z_UTIL_DEC_, x)

#define Z_UTIL_INC(x) UTIL_PRIMITIVE_CAT(Z_UTIL_INC_, x)

#define Z_UTIL_DEC(x) UTIL_PRIMITIVE_CAT(Z_UTIL_DEC_, x)

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_ */
