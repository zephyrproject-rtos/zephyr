/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Misc utilities
 *
 * Misc utilities usable by the kernel and application code.
 */

#ifndef ZEPHYR_INCLUDE_MISC_UTIL_H_
#define ZEPHYR_INCLUDE_MISC_UTIL_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

/* Helper to pass a int as a pointer or vice-versa.
 * Those are available for 32 bits architectures:
 */
#define POINTER_TO_UINT(x) ((u32_t) (x))
#define UINT_TO_POINTER(x) ((void *) (x))
#define POINTER_TO_INT(x)  ((s32_t) (x))
#define INT_TO_POINTER(x)  ((void *) (x))

/* Evaluates to 0 if cond is true-ish; compile error otherwise */
#define ZERO_OR_COMPILE_ERROR(cond) ((int) sizeof(char[1 - 2 * !(cond)]) - 1)

/* Evaluates to 0 if array is an array; compile error if not array (e.g.
 * pointer)
 */
#define IS_ARRAY(array) \
	ZERO_OR_COMPILE_ERROR( \
		!__builtin_types_compatible_p(__typeof__(array), \
					      __typeof__(&(array)[0])))

#if defined(__cplusplus)
template < class T, size_t N >
constexpr size_t ARRAY_SIZE(T(&)[N]) { return N; }

#else
/* Evaluates to number of elements in an array; compile error if not
 * an array (e.g. pointer)
 */
#define ARRAY_SIZE(array) \
	((unsigned long) (IS_ARRAY(array) + \
		(sizeof(array) / sizeof((array)[0]))))
#endif

/* Evaluates to 1 if ptr is part of array, 0 otherwise; compile error if
 * "array" argument is not an array (e.g. "ptr" and "array" mixed up)
 */
#define PART_OF_ARRAY(array, ptr) \
	((ptr) && ((ptr) >= &array[0] && (ptr) < &array[ARRAY_SIZE(array)]))

#define CONTAINER_OF(ptr, type, field) \
	((type *)(((char *)(ptr)) - offsetof(type, field)))

/* round "x" up/down to next multiple of "align" (which must be a power of 2) */
#define ROUND_UP(x, align)                                   \
	(((unsigned long)(x) + ((unsigned long)align - 1)) & \
	 ~((unsigned long)align - 1))
#define ROUND_DOWN(x, align) ((unsigned long)(x) & ~((unsigned long)align - 1))

#define ceiling_fraction(numerator, divider) \
	(((numerator) + ((divider) - 1)) / (divider))

#ifdef INLINED
#define INLINE inline
#else
#define INLINE
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static inline int is_power_of_two(unsigned int x)
{
	return (x != 0) && !(x & (x - 1));
}

static inline s64_t arithmetic_shift_right(s64_t value, u8_t shift)
{
	s64_t sign_ext;

	if (shift == 0) {
		return value;
	}

	/* extract sign bit */
	sign_ext = (value >> 63) & 1;

	/* make all bits of sign_ext be the same as the value's sign bit */
	sign_ext = -sign_ext;

	/* shift value and fill opened bit positions with sign bit */
	return (value >> shift) | (sign_ext << (64 - shift));
}

#endif /* !_ASMLANGUAGE */

/* KB, MB, GB */
#define KB(x) ((x) << 10)
#define MB(x) (KB(x) << 10)
#define GB(x) (MB(x) << 10)

/* KHZ, MHZ */
#define KHZ(x) ((x) * 1000)
#define MHZ(x) (KHZ(x) * 1000)

#ifndef BIT
#if defined(_ASMLANGUAGE)
#define BIT(n)  (1 << (n))
#else
#define BIT(n)  (1UL << (n))
#endif
#endif

/**
 * @brief Macro sets or clears bit depending on boolean value
 *
 * @param var Variable to be altered
 * @param bit Bit number
 * @param set Value 0 clears bit, any other value sets bit
 */
#define WRITE_BIT(var, bit, set) \
	((var) = (set) ? ((var) | BIT(bit)) : ((var) & ~BIT(bit)))

#define BIT_MASK(n) (BIT(n) - 1)

/**
 * @brief Check for macro definition in compiler-visible expressions
 *
 * This trick was pioneered in Linux as the config_enabled() macro.
 * The madness has the effect of taking a macro value that may be
 * defined to "1" (e.g. CONFIG_MYFEATURE), or may not be defined at
 * all and turning it into a literal expression that can be used at
 * "runtime".  That is, it works similarly to
 * "defined(CONFIG_MYFEATURE)" does except that it is an expansion
 * that can exist in a standard expression and be seen by the compiler
 * and optimizer.  Thus much ifdef usage can be replaced with cleaner
 * expressions like:
 *
 *     if (IS_ENABLED(CONFIG_MYFEATURE))
 *             myfeature_enable();
 *
 * INTERNAL
 * First pass just to expand any existing macros, we need the macro
 * value to be e.g. a literal "1" at expansion time in the next macro,
 * not "(1)", etc...  Standard recursive expansion does not work.
 */
#define IS_ENABLED(config_macro) _IS_ENABLED1(config_macro)

/* Now stick on a "_XXXX" prefix, it will now be "_XXXX1" if config_macro
 * is "1", or just "_XXXX" if it's undefined.
 *   ENABLED:   _IS_ENABLED2(_XXXX1)
 *   DISABLED   _IS_ENABLED2(_XXXX)
 */
#define _IS_ENABLED1(config_macro) _IS_ENABLED2(_XXXX##config_macro)

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
 *   ENABLED:   _IS_ENABLED3(_YYYY,    1,    0)
 *   DISABLED   _IS_ENABLED3(_XXXX 1,  0)
 */
#define _IS_ENABLED2(one_or_two_args) _IS_ENABLED3(one_or_two_args 1, 0)

/* And our second argument is thus now cooked to be 1 in the case
 * where the value is defined to 1, and 0 if not:
 */
#define _IS_ENABLED3(ignore_this, val, ...) val

/**
 * Macros for doing code-generation with the preprocessor.
 *
 * Generally it is better to generate code with the preprocessor than
 * to copy-paste code or to generate code with the build system /
 * python script's etc.
 *
 * http://stackoverflow.com/a/12540675
 */
#define UTIL_EMPTY(...)
#define UTIL_DEFER(...) __VA_ARGS__ UTIL_EMPTY()
#define UTIL_OBSTRUCT(...) __VA_ARGS__ UTIL_DEFER(UTIL_EMPTY)()
#define UTIL_EXPAND(...) __VA_ARGS__

#define UTIL_EVAL(...)  UTIL_EVAL1(UTIL_EVAL1(UTIL_EVAL1(__VA_ARGS__)))
#define UTIL_EVAL1(...) UTIL_EVAL2(UTIL_EVAL2(UTIL_EVAL2(__VA_ARGS__)))
#define UTIL_EVAL2(...) UTIL_EVAL3(UTIL_EVAL3(UTIL_EVAL3(__VA_ARGS__)))
#define UTIL_EVAL3(...) UTIL_EVAL4(UTIL_EVAL4(UTIL_EVAL4(__VA_ARGS__)))
#define UTIL_EVAL4(...) UTIL_EVAL5(UTIL_EVAL5(UTIL_EVAL5(__VA_ARGS__)))
#define UTIL_EVAL5(...) __VA_ARGS__

#define UTIL_CAT(a, ...) UTIL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define UTIL_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define UTIL_INC(x) UTIL_PRIMITIVE_CAT(UTIL_INC_, x)
#define UTIL_INC_0 1
#define UTIL_INC_1 2
#define UTIL_INC_2 3
#define UTIL_INC_3 4
#define UTIL_INC_4 5
#define UTIL_INC_5 6
#define UTIL_INC_6 7
#define UTIL_INC_7 8
#define UTIL_INC_8 9
#define UTIL_INC_9 10
#define UTIL_INC_10 11
#define UTIL_INC_11 12
#define UTIL_INC_12 13
#define UTIL_INC_13 14
#define UTIL_INC_14 15
#define UTIL_INC_15 16
#define UTIL_INC_16 17
#define UTIL_INC_17 18
#define UTIL_INC_18 19
#define UTIL_INC_19 19

#define UTIL_DEC(x) UTIL_PRIMITIVE_CAT(UTIL_DEC_, x)
#define UTIL_DEC_0 0
#define UTIL_DEC_1 0
#define UTIL_DEC_2 1
#define UTIL_DEC_3 2
#define UTIL_DEC_4 3
#define UTIL_DEC_5 4
#define UTIL_DEC_6 5
#define UTIL_DEC_7 6
#define UTIL_DEC_8 7
#define UTIL_DEC_9 8
#define UTIL_DEC_10 9
#define UTIL_DEC_11 10
#define UTIL_DEC_12 11
#define UTIL_DEC_13 12
#define UTIL_DEC_14 13
#define UTIL_DEC_15 14
#define UTIL_DEC_16 15
#define UTIL_DEC_17 16
#define UTIL_DEC_18 17
#define UTIL_DEC_19 18
#define UTIL_DEC_20 19
#define UTIL_DEC_21 20
#define UTIL_DEC_22 21
#define UTIL_DEC_23 22
#define UTIL_DEC_24 23
#define UTIL_DEC_25 24
#define UTIL_DEC_26 25
#define UTIL_DEC_27 26
#define UTIL_DEC_28 27
#define UTIL_DEC_29 28
#define UTIL_DEC_30 29
#define UTIL_DEC_31 30
#define UTIL_DEC_32 31
#define UTIL_DEC_33 32
#define UTIL_DEC_34 33
#define UTIL_DEC_35 34
#define UTIL_DEC_36 35
#define UTIL_DEC_37 36
#define UTIL_DEC_38 37
#define UTIL_DEC_39 38
#define UTIL_DEC_40 39

#define UTIL_CHECK_N(x, n, ...) n
#define UTIL_CHECK(...) UTIL_CHECK_N(__VA_ARGS__, 0,)

#define UTIL_NOT(x) UTIL_CHECK(UTIL_PRIMITIVE_CAT(UTIL_NOT_, x))
#define UTIL_NOT_0 ~, 1,

#define UTIL_COMPL(b) UTIL_PRIMITIVE_CAT(UTIL_COMPL_, b)
#define UTIL_COMPL_0 1
#define UTIL_COMPL_1 0

#define UTIL_BOOL(x) UTIL_COMPL(UTIL_NOT(x))

#define UTIL_IIF(c) UTIL_PRIMITIVE_CAT(UTIL_IIF_, c)
#define UTIL_IIF_0(t, ...) __VA_ARGS__
#define UTIL_IIF_1(t, ...) t

#define UTIL_IF(c) UTIL_IIF(UTIL_BOOL(c))

#define UTIL_EAT(...)
#define UTIL_EXPAND(...) __VA_ARGS__
#define UTIL_WHEN(c) UTIL_IF(c)(UTIL_EXPAND, UTIL_EAT)

#define UTIL_REPEAT(count, macro, ...)			    \
	UTIL_WHEN(count)				    \
	(						    \
		UTIL_OBSTRUCT(UTIL_REPEAT_INDIRECT) ()	    \
		(					    \
			UTIL_DEC(count), macro, __VA_ARGS__ \
		)					    \
		UTIL_OBSTRUCT(macro)			    \
		(					    \
			UTIL_DEC(count), __VA_ARGS__	    \
		)					    \
	)
#define UTIL_REPEAT_INDIRECT() UTIL_REPEAT

/**
 * Generates a sequence of code.
 * Useful for generating code like;
 *
 * NRF_PWM0, NRF_PWM1, NRF_PWM2,
 *
 * @arg LEN: The length of the sequence. Must be defined and less than
 * 20.
 *
 * @arg F(i, F_ARG): A macro function that accepts two arguments.
 *  F is called repeatedly, the first argument
 *  is the index in the sequence, and the second argument is the third
 *  argument given to UTIL_LISTIFY.
 *
 * Example:
 *
 *    \#define FOO(i, _) NRF_PWM ## i ,
 *    { UTIL_LISTIFY(PWM_COUNT, FOO) }
 *    // The above two lines will generate the below:
 *    { NRF_PWM0 , NRF_PWM1 , }
 *
 * @note Calling UTIL_LISTIFY with undefined arguments has undefined
 * behavior.
 */
#define UTIL_LISTIFY(LEN, F, F_ARG) UTIL_EVAL(UTIL_REPEAT(LEN, F, F_ARG))

/**@brief Implementation details for NUM_VAR_ARGS */
#define NUM_VA_ARGS_LESS_1_IMPL(				\
	_ignored,						\
	_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,		\
	_11, _12, _13, _14, _15, _16, _17, _18, _19, _20,	\
	_21, _22, _23, _24, _25, _26, _27, _28, _29, _30,	\
	_31, _32, _33, _34, _35, _36, _37, _38, _39, _40,	\
	_41, _42, _43, _44, _45, _46, _47, _48, _49, _50,	\
	_51, _52, _53, _54, _55, _56, _57, _58, _59, _60,	\
	_61, _62, N, ...) N

/**@brief Macro to get the number of arguments in a call variadic macro call.
 * First argument is not counted.
 *
 * param[in]    ...     List of arguments
 *
 * @retval  Number of variadic arguments in the argument list
 */
#define NUM_VA_ARGS_LESS_1(...) \
	NUM_VA_ARGS_LESS_1_IMPL(__VA_ARGS__, 63, 62, 61, \
	60, 59, 58, 57, 56, 55, 54, 53, 52, 51,		 \
	50, 49, 48, 47, 46, 45, 44, 43, 42, 41,		 \
	40, 39, 38, 37, 36, 35, 34, 33, 32, 31,		 \
	30, 29, 28, 27, 26, 25, 24, 23, 22, 21,		 \
	20, 19, 18, 17, 16, 15, 14, 13, 12, 11,		 \
	10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, ~)

/**
 * @brief Mapping macro
 *
 * Macro that process all arguments using given macro
 *
 * @param ... Macro name to be used for argument processing followed by
 *            arguments to process. Macro should have following
 *            form: MACRO(argument).
 *
 * @return All arguments processed by given macro
 */
#define MACRO_MAP(...) MACRO_MAP_(__VA_ARGS__)
#define MACRO_MAP_(...)							\
	/* To make sure it works also for 2 arguments in total */	\
	MACRO_MAP_N(NUM_VA_ARGS_LESS_1(__VA_ARGS__), __VA_ARGS__)

/**
 * @brief Mapping N arguments macro
 *
 * Macro similar to @ref MACRO_MAP but maps exact number of arguments.
 * If there is more arguments given, the rest would be ignored.
 *
 * @param N   Number of arguments to map
 * @param ... Macro name to be used for argument processing followed by
 *            arguments to process. Macro should have following
 *            form: MACRO(argument).
 *
 * @return Selected number of arguments processed by given macro
 */
#define MACRO_MAP_N(N, ...) MACRO_MAP_N_(N, __VA_ARGS__)
#define MACRO_MAP_N_(N, ...) UTIL_CAT(MACRO_MAP_, N)(__VA_ARGS__,)

#define MACRO_MAP_0(...)
#define MACRO_MAP_1(macro, a, ...)  macro(a)
#define MACRO_MAP_2(macro, a, ...)  macro(a)MACRO_MAP_1(macro, __VA_ARGS__,)
#define MACRO_MAP_3(macro, a, ...)  macro(a)MACRO_MAP_2(macro, __VA_ARGS__,)
#define MACRO_MAP_4(macro, a, ...)  macro(a)MACRO_MAP_3(macro, __VA_ARGS__,)
#define MACRO_MAP_5(macro, a, ...)  macro(a)MACRO_MAP_4(macro, __VA_ARGS__,)
#define MACRO_MAP_6(macro, a, ...)  macro(a)MACRO_MAP_5(macro, __VA_ARGS__,)
#define MACRO_MAP_7(macro, a, ...)  macro(a)MACRO_MAP_6(macro, __VA_ARGS__,)
#define MACRO_MAP_8(macro, a, ...)  macro(a)MACRO_MAP_7(macro, __VA_ARGS__,)
#define MACRO_MAP_9(macro, a, ...)  macro(a)MACRO_MAP_8(macro, __VA_ARGS__,)
#define MACRO_MAP_10(macro, a, ...) macro(a)MACRO_MAP_9(macro, __VA_ARGS__,)
#define MACRO_MAP_11(macro, a, ...) macro(a)MACRO_MAP_10(macro, __VA_ARGS__,)
#define MACRO_MAP_12(macro, a, ...) macro(a)MACRO_MAP_11(macro, __VA_ARGS__,)
#define MACRO_MAP_13(macro, a, ...) macro(a)MACRO_MAP_12(macro, __VA_ARGS__,)
#define MACRO_MAP_14(macro, a, ...) macro(a)MACRO_MAP_13(macro, __VA_ARGS__,)
#define MACRO_MAP_15(macro, a, ...) macro(a)MACRO_MAP_14(macro, __VA_ARGS__,)
/*
 * The following provides variadic preprocessor macro support to
 * help eliminate multiple, repetitive function/macro calls.  This
 * allows up to 10 "arguments" in addition to _call .
 * Note - derived from work on:
 * https://codecraft.co/2014/11/25/variadic-macros-tricks/
 */

#define _GET_ARG(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N

#define _for_0(_call, ...)
#define _for_1(_call, x) _call(x)
#define _for_2(_call, x, ...) _call(x) _for_1(_call, ##__VA_ARGS__)
#define _for_3(_call, x, ...) _call(x) _for_2(_call, ##__VA_ARGS__)
#define _for_4(_call, x, ...) _call(x) _for_3(_call, ##__VA_ARGS__)
#define _for_5(_call, x, ...) _call(x) _for_4(_call, ##__VA_ARGS__)
#define _for_6(_call, x, ...) _call(x) _for_5(_call, ##__VA_ARGS__)
#define _for_7(_call, x, ...) _call(x) _for_6(_call, ##__VA_ARGS__)
#define _for_8(_call, x, ...) _call(x) _for_7(_call, ##__VA_ARGS__)
#define _for_9(_call, x, ...) _call(x) _for_8(_call, ##__VA_ARGS__)
#define _for_10(_call, x, ...) _call(x) _for_9(_call, ##__VA_ARGS__)

#define FOR_EACH(x, ...) \
	_GET_ARG(__VA_ARGS__, \
	_for_10, _for_9, _for_8, _for_7, _for_6, _for_5, \
	_for_4, _for_3, _for_2, _for_1, _for_0)(x, ##__VA_ARGS__)

#endif /* ZEPHYR_INCLUDE_MISC_UTIL_H_ */
