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

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_H_

/* needs to be outside _ASMLANGUAGE so 'true' and 'false' can turn
 * into '1' and '0' for asm or linker scripts
 */
#include <stdbool.h>

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Helper to pass a int as a pointer or vice-versa. */
#define POINTER_TO_UINT(x) ((uintptr_t) (x))
#define UINT_TO_POINTER(x) ((void *) (uintptr_t) (x))
#define POINTER_TO_INT(x)  ((intptr_t) (x))
#define INT_TO_POINTER(x)  ((void *) (intptr_t) (x))

#if !(defined (__CHAR_BIT__) && defined (__SIZEOF_LONG__))
#	error Missing required predefined macros for BITS_PER_LONG calculation
#endif

#define BITS_PER_LONG	(__CHAR_BIT__ * __SIZEOF_LONG__)
/* Create a contiguous bitmask starting at bit position @l and ending at
 * position @h.
 */
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

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
extern "C++" {
template < class T, size_t N >
#if __cplusplus >= 201103L
constexpr
#endif /* >= C++11 */
size_t ARRAY_SIZE(T(&)[N]) { return N; }
}
#else
/* Evaluates to number of elements in an array; compile error if not
 * an array (e.g. pointer)
 */
#define ARRAY_SIZE(array) \
	((long) (IS_ARRAY(array) + (sizeof(array) / sizeof((array)[0]))))
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
	(((unsigned long)(x) + ((unsigned long)(align) - 1)) & \
	 ~((unsigned long)(align) - 1))
#define ROUND_DOWN(x, align)                                 \
	((unsigned long)(x) & ~((unsigned long)(align) - 1))

/* round up/down to the next word boundary */
#define WB_UP(x) ROUND_UP(x, sizeof(void *))
#define WB_DN(x) ROUND_DOWN(x, sizeof(void *))

#define ceiling_fraction(numerator, divider) \
	(((numerator) + ((divider) - 1)) / (divider))

/** @brief Return larger value of two provided expressions.
 *
 * @note Arguments are evaluated twice. See Z_MAX for GCC only, single
 * evaluation version.
 */
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/** @brief Return smaller value of two provided expressions.
 *
 * @note Arguments are evaluated twice. See Z_MIN for GCC only, single
 * evaluation version.
 */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static inline bool is_power_of_two(unsigned int x)
{
	return (x != 0U) && ((x & (x - 1)) == 0U);
}

static inline s64_t arithmetic_shift_right(s64_t value, u8_t shift)
{
	s64_t sign_ext;

	if (shift == 0U) {
		return value;
	}

	/* extract sign bit */
	sign_ext = (value >> 63) & 1;

	/* make all bits of sign_ext be the same as the value's sign bit */
	sign_ext = -sign_ext;

	/* shift value and fill opened bit positions with sign bit */
	return (value >> shift) | (sign_ext << (64 - shift));
}

/**
 * @brief      Convert a single character into a hexadecimal nibble.
 *
 * @param[in]  c     The character to convert
 * @param      x     The address of storage for the converted number.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int char2hex(char c, u8_t *x);

/**
 * @brief      Convert a single hexadecimal nibble into a character.
 *
 * @param[in]  c     The number to convert
 * @param      x     The address of storage for the converted character.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hex2char(u8_t x, char *c);

/**
 * @brief      Convert a binary array into string representation.
 *
 * @param[in]  buf     The binary array to convert
 * @param[in]  buflen  The length of the binary array to convert
 * @param[out] hex     Address of where to store the string representation.
 * @param[in]  hexlen  Size of the storage area for string representation.
 *
 * @return     The length of the converted string, or 0 if an error occurred.
 */
size_t bin2hex(const u8_t *buf, size_t buflen, char *hex, size_t hexlen);

/*
 * Convert hex string to byte string
 * Return number of bytes written to buf, or 0 on error
 * @return     The length of the converted array, or 0 if an error occurred.
 */

/**
 * @brief      Convert a hexadecimal string into a binary array.
 *
 * @param[in]  hex     The hexadecimal string to convert
 * @param[in]  hexlen  The length of the hexadecimal string to convert.
 * @param[out] buf     Address of where to store the binary data
 * @param[in]  buflen  Size of the storage area for binary data
 *
 * @return     The length of the binary array , or 0 if an error occurred.
 */
size_t hex2bin(const char *hex, size_t hexlen, u8_t *buf, size_t buflen);

/**
 * @brief      Convert a u8_t into decimal string representation.
 *
 * Convert a u8_t value into ASCII decimal string representation.
 * The string is terminated if there is enough space in buf.
 *
 * @param[out] buf     Address of where to store the string representation.
 * @param[in]  buflen  Size of the storage area for string representation.
 * @param[in]  value   The value to convert to decimal string
 *
 * @return     The length of the converted string (excluding terminator if
 *             any), or 0 if an error occurred.
 */
u8_t u8_to_dec(char *buf, u8_t buflen, u8_t value);

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

/** 64-bit unsigned integer with bit position _n set */
#define BIT64(_n) (1ULL << (_n))

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
#define IS_ENABLED(config_macro) Z_IS_ENABLED1(config_macro)

/* Now stick on a "_XXXX" prefix, it will now be "_XXXX1" if config_macro
 * is "1", or just "_XXXX" if it's undefined.
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
#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args true, false)

/* And our second argument is thus now cooked to be 1 in the case
 * where the value is defined to 1, and 0 if not:
 */
#define Z_IS_ENABLED3(ignore_this, val, ...) val

/**
 * @brief Insert code depending on result of flag evaluation.
 *
 * This is based on same idea as @ref IS_ENABLED macro but as the result of
 * flag evaluation provided code is injected. Because preprocessor interprets
 * each comma as an argument boundary, code must be provided in the brackets.
 * Brackets are stripped away during macro processing.
 *
 * Usage example:
 *
 * \#define MACRO(x) COND_CODE_1(CONFIG_FLAG, (u32_t x;), ())
 *
 * It can be considered as alternative to:
 *
 * \#if defined(CONFIG_FLAG) && (CONFIG_FLAG == 1)
 * \#define MACRO(x) u32_t x;
 * \#else
 * \#define MACRO(x)
 * \#endif
 *
 * However, the advantage of that approach is that code is resolved in place
 * where it is used while \#if method resolves given macro when header is
 * included and product is fixed in the given scope.
 *
 * @note Flag can also be a result of preprocessor output e.g.
 *	 product of NUM_VA_ARGS_LESS_1(...).
 *
 * @param _flag		Evaluated flag
 * @param _if_1_code	Code used if flag exists and equal 1. Argument must be
 *			in brackets.
 * @param _else_code	Code used if flag doesn't exists or isn't equal 1.
 *
 */
#define COND_CODE_1(_flag, _if_1_code, _else_code) \
	Z_COND_CODE_1(_flag, _if_1_code, _else_code)

#define Z_COND_CODE_1(_flag, _if_1_code, _else_code) \
	__COND_CODE(_XXXX##_flag, _if_1_code, _else_code)

/**
 * @brief Insert code if flag is defined and equals 1.
 *
 * Usage example:
 *
 * IF_ENABLED(CONFIG_FLAG, (u32_t foo;))
 *
 * It can be considered as more compact alternative to:
 *
 * \#if defined(CONFIG_FLAG) && (CONFIG_FLAG == 1)
 *	u32_t foo;
 * \#endif
 *
 * @param _flag		Evaluated flag
 * @param _code		Code used if flag exists and equal 1. Argument must be
 *			in brackets.
 */
#define IF_ENABLED(_flag, _code) \
	COND_CODE_1(_flag, _code, ())
/**
 * @brief Insert code depending on result of flag evaluation.
 *
 * See @ref COND_CODE_1 for details.
 *
 * @param _flag		Evaluated flag
 * @param _if_0_code	Code used if flag exists and equal 0. Argument must be
 *			in brackets.
 * @param _else_code	Code used if flag doesn't exists or isn't equal 0.
 *
 */
#define COND_CODE_0(_flag, _if_0_code, _else_code) \
	Z_COND_CODE_0(_flag, _if_0_code, _else_code)

#define Z_COND_CODE_0(_flag, _if_0_code, _else_code) \
	__COND_CODE(_ZZZZ##_flag, _if_0_code, _else_code)

#define _ZZZZ0 _YYYY,

/* Macro used internally by @ref COND_CODE_1 and @ref COND_CODE_0. */
#define __COND_CODE(one_or_two_args, _if_code, _else_code) \
	__GET_ARG2_DEBRACKET(one_or_two_args _if_code, _else_code)

/* Macro used internally to remove brackets from argument. */
#define __DEBRACKET(...) __VA_ARGS__

/* Macro used internally for getting second argument and removing brackets
 * around that argument. It is expected that parameter is provided in brackets
 */
#define __GET_ARG2_DEBRACKET(ignore_this, val, ...) __DEBRACKET val

/**
 * @brief Get first argument from variable list of arguments
 */
#define GET_ARG1(arg1, ...) arg1

/**
 * @brief Get second argument from variable list of arguments
 */
#define GET_ARG2(arg1, arg2, ...) arg2

/**
 * @brief Get all arguments except the first one.
 */
#define GET_ARGS_LESS_1(val, ...) __VA_ARGS__

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
#define UTIL_INC_19 20
#define UTIL_INC_20 21
#define UTIL_INC_21 22
#define UTIL_INC_22 23
#define UTIL_INC_23 24
#define UTIL_INC_24 25
#define UTIL_INC_25 26
#define UTIL_INC_26 27
#define UTIL_INC_27 28
#define UTIL_INC_28 29
#define UTIL_INC_29 30
#define UTIL_INC_30 31
#define UTIL_INC_31 32
#define UTIL_INC_32 33
#define UTIL_INC_33 34
#define UTIL_INC_34 35
#define UTIL_INC_35 36
#define UTIL_INC_36 37
#define UTIL_INC_37 38
#define UTIL_INC_38 39
#define UTIL_INC_39 40
#define UTIL_INC_40 41
#define UTIL_INC_41 42
#define UTIL_INC_42 43
#define UTIL_INC_43 44
#define UTIL_INC_44 45
#define UTIL_INC_45 46
#define UTIL_INC_46 47
#define UTIL_INC_47 48
#define UTIL_INC_48 49
#define UTIL_INC_49 50
#define UTIL_INC_50 51
#define UTIL_INC_51 52
#define UTIL_INC_52 53
#define UTIL_INC_53 54
#define UTIL_INC_54 55
#define UTIL_INC_55 56
#define UTIL_INC_56 57
#define UTIL_INC_57 58
#define UTIL_INC_58 59
#define UTIL_INC_59 60
#define UTIL_INC_50 51
#define UTIL_INC_51 52
#define UTIL_INC_52 53
#define UTIL_INC_53 54
#define UTIL_INC_54 55
#define UTIL_INC_55 56
#define UTIL_INC_56 57
#define UTIL_INC_57 58
#define UTIL_INC_58 59
#define UTIL_INC_59 60
#define UTIL_INC_60 61
#define UTIL_INC_61 62
#define UTIL_INC_62 63
#define UTIL_INC_63 64
#define UTIL_INC_64 65
#define UTIL_INC_65 66
#define UTIL_INC_66 67
#define UTIL_INC_67 68
#define UTIL_INC_68 69
#define UTIL_INC_69 70
#define UTIL_INC_70 71
#define UTIL_INC_71 72
#define UTIL_INC_72 73
#define UTIL_INC_73 74
#define UTIL_INC_74 75
#define UTIL_INC_75 76
#define UTIL_INC_76 77
#define UTIL_INC_77 78
#define UTIL_INC_78 79
#define UTIL_INC_79 80
#define UTIL_INC_80 81
#define UTIL_INC_81 82
#define UTIL_INC_82 83
#define UTIL_INC_83 84
#define UTIL_INC_84 85
#define UTIL_INC_85 86
#define UTIL_INC_86 87
#define UTIL_INC_87 88
#define UTIL_INC_88 89
#define UTIL_INC_89 90
#define UTIL_INC_90 91
#define UTIL_INC_91 92
#define UTIL_INC_92 93
#define UTIL_INC_93 94
#define UTIL_INC_94 95
#define UTIL_INC_95 96
#define UTIL_INC_96 97
#define UTIL_INC_97 98
#define UTIL_INC_98 99
#define UTIL_INC_99 100

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
#define UTIL_DEC_41 40
#define UTIL_DEC_42 41
#define UTIL_DEC_43 42
#define UTIL_DEC_44 43
#define UTIL_DEC_45 44
#define UTIL_DEC_46 45
#define UTIL_DEC_47 46
#define UTIL_DEC_48 47
#define UTIL_DEC_49 48
#define UTIL_DEC_50 49
#define UTIL_DEC_51 50
#define UTIL_DEC_52 51
#define UTIL_DEC_53 52
#define UTIL_DEC_54 53
#define UTIL_DEC_55 54
#define UTIL_DEC_56 55
#define UTIL_DEC_57 56
#define UTIL_DEC_58 57
#define UTIL_DEC_59 58
#define UTIL_DEC_60 59
#define UTIL_DEC_61 60
#define UTIL_DEC_62 61
#define UTIL_DEC_63 62
#define UTIL_DEC_64 63
#define UTIL_DEC_65 64
#define UTIL_DEC_66 65
#define UTIL_DEC_67 66
#define UTIL_DEC_68 67
#define UTIL_DEC_69 68
#define UTIL_DEC_70 69
#define UTIL_DEC_71 70
#define UTIL_DEC_72 71
#define UTIL_DEC_73 72
#define UTIL_DEC_74 73
#define UTIL_DEC_75 74
#define UTIL_DEC_76 75
#define UTIL_DEC_77 76
#define UTIL_DEC_78 77
#define UTIL_DEC_79 78
#define UTIL_DEC_80 79
#define UTIL_DEC_81 80
#define UTIL_DEC_82 81
#define UTIL_DEC_83 82
#define UTIL_DEC_84 83
#define UTIL_DEC_85 84
#define UTIL_DEC_86 85
#define UTIL_DEC_87 86
#define UTIL_DEC_88 87
#define UTIL_DEC_89 88
#define UTIL_DEC_90 89
#define UTIL_DEC_91 90
#define UTIL_DEC_92 91
#define UTIL_DEC_93 92
#define UTIL_DEC_94 93
#define UTIL_DEC_95 94
#define UTIL_DEC_96 95
#define UTIL_DEC_97 96
#define UTIL_DEC_98 97
#define UTIL_DEC_99 98
#define UTIL_DEC_100 99
#define UTIL_DEC_101 100
#define UTIL_DEC_102 101
#define UTIL_DEC_103 102
#define UTIL_DEC_104 103
#define UTIL_DEC_105 104
#define UTIL_DEC_106 105
#define UTIL_DEC_107 106
#define UTIL_DEC_108 107
#define UTIL_DEC_109 108
#define UTIL_DEC_110 109
#define UTIL_DEC_111 110
#define UTIL_DEC_112 111
#define UTIL_DEC_113 112
#define UTIL_DEC_114 113
#define UTIL_DEC_115 114
#define UTIL_DEC_116 115
#define UTIL_DEC_117 116
#define UTIL_DEC_118 117
#define UTIL_DEC_119 118
#define UTIL_DEC_120 119
#define UTIL_DEC_121 120
#define UTIL_DEC_122 121
#define UTIL_DEC_123 122
#define UTIL_DEC_124 123
#define UTIL_DEC_125 124
#define UTIL_DEC_126 125
#define UTIL_DEC_127 126
#define UTIL_DEC_128 127
#define UTIL_DEC_129 128
#define UTIL_DEC_130 129
#define UTIL_DEC_131 130
#define UTIL_DEC_132 131
#define UTIL_DEC_133 132
#define UTIL_DEC_134 133
#define UTIL_DEC_135 134
#define UTIL_DEC_136 135
#define UTIL_DEC_137 136
#define UTIL_DEC_138 137
#define UTIL_DEC_139 138
#define UTIL_DEC_140 139
#define UTIL_DEC_141 140
#define UTIL_DEC_142 141
#define UTIL_DEC_143 142
#define UTIL_DEC_144 143
#define UTIL_DEC_145 144
#define UTIL_DEC_146 145
#define UTIL_DEC_147 146
#define UTIL_DEC_148 147
#define UTIL_DEC_149 148
#define UTIL_DEC_150 149
#define UTIL_DEC_151 150
#define UTIL_DEC_152 151
#define UTIL_DEC_153 152
#define UTIL_DEC_154 153
#define UTIL_DEC_155 154
#define UTIL_DEC_156 155
#define UTIL_DEC_157 156
#define UTIL_DEC_158 157
#define UTIL_DEC_159 158
#define UTIL_DEC_160 159
#define UTIL_DEC_161 160
#define UTIL_DEC_162 161
#define UTIL_DEC_163 162
#define UTIL_DEC_164 163
#define UTIL_DEC_165 164
#define UTIL_DEC_166 165
#define UTIL_DEC_167 166
#define UTIL_DEC_168 167
#define UTIL_DEC_169 168
#define UTIL_DEC_170 169
#define UTIL_DEC_171 170
#define UTIL_DEC_172 171
#define UTIL_DEC_173 172
#define UTIL_DEC_174 173
#define UTIL_DEC_175 174
#define UTIL_DEC_176 175
#define UTIL_DEC_177 176
#define UTIL_DEC_178 177
#define UTIL_DEC_179 178
#define UTIL_DEC_180 179
#define UTIL_DEC_181 180
#define UTIL_DEC_182 181
#define UTIL_DEC_183 182
#define UTIL_DEC_184 183
#define UTIL_DEC_185 184
#define UTIL_DEC_186 185
#define UTIL_DEC_187 186
#define UTIL_DEC_188 187
#define UTIL_DEC_189 188
#define UTIL_DEC_190 189
#define UTIL_DEC_191 190
#define UTIL_DEC_192 191
#define UTIL_DEC_193 192
#define UTIL_DEC_194 193
#define UTIL_DEC_195 194
#define UTIL_DEC_196 195
#define UTIL_DEC_197 196
#define UTIL_DEC_198 197
#define UTIL_DEC_199 198
#define UTIL_DEC_200 199
#define UTIL_DEC_201 200
#define UTIL_DEC_202 201
#define UTIL_DEC_203 202
#define UTIL_DEC_204 203
#define UTIL_DEC_205 204
#define UTIL_DEC_206 205
#define UTIL_DEC_207 206
#define UTIL_DEC_208 207
#define UTIL_DEC_209 208
#define UTIL_DEC_210 209
#define UTIL_DEC_211 210
#define UTIL_DEC_212 211
#define UTIL_DEC_213 212
#define UTIL_DEC_214 213
#define UTIL_DEC_215 214
#define UTIL_DEC_216 215
#define UTIL_DEC_217 216
#define UTIL_DEC_218 217
#define UTIL_DEC_219 218
#define UTIL_DEC_220 219
#define UTIL_DEC_221 220
#define UTIL_DEC_222 221
#define UTIL_DEC_223 222
#define UTIL_DEC_224 223
#define UTIL_DEC_225 224
#define UTIL_DEC_226 225
#define UTIL_DEC_227 226
#define UTIL_DEC_228 227
#define UTIL_DEC_229 228
#define UTIL_DEC_230 229
#define UTIL_DEC_231 230
#define UTIL_DEC_232 231
#define UTIL_DEC_233 232
#define UTIL_DEC_234 233
#define UTIL_DEC_235 234
#define UTIL_DEC_236 235
#define UTIL_DEC_237 236
#define UTIL_DEC_238 237
#define UTIL_DEC_239 238
#define UTIL_DEC_240 239
#define UTIL_DEC_241 240
#define UTIL_DEC_242 241
#define UTIL_DEC_243 242
#define UTIL_DEC_244 243
#define UTIL_DEC_245 244
#define UTIL_DEC_246 245
#define UTIL_DEC_247 246
#define UTIL_DEC_248 247
#define UTIL_DEC_249 248
#define UTIL_DEC_250 249
#define UTIL_DEC_251 250
#define UTIL_DEC_252 251
#define UTIL_DEC_253 252
#define UTIL_DEC_254 253
#define UTIL_DEC_255 254
#define UTIL_DEC_256 255

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

/*
 * These are like || and &&, but they do evaluation and
 * short-circuiting at preprocessor time instead of runtime.
 *
 * UTIL_OR(foo, bar) is sometimes a replacement for (foo || bar)
 * when "bar" is an expression that would cause a build
 * error when "foo" is true.
 *
 * UTIL_AND(foo, bar) is sometimes a replacement for (foo && bar)
 * when "bar" is an expression that would cause a build
 * error when "foo" is false.
 */
#define UTIL_OR(a, b) COND_CODE_1(UTIL_BOOL(a), (a), (b))
#define UTIL_AND(a, b) COND_CODE_1(UTIL_BOOL(a), (b), (0))

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
 * @brief Generates a sequence of code.
 *
 * Useful for generating code like;
 *
 * NRF_PWM0, NRF_PWM1, NRF_PWM2,
 *
 * @arg LEN: The length of the sequence. Must be defined and less than
 * 20.
 *
 * @arg F(i, ...): A macro function that accepts at least two arguments.
 *  F is called repeatedly, the first argument is the index in the sequence,
 *  the variable list of arguments passed to UTIL_LISTIFY are passed through
 *  to F.
 *
 * Example:
 *
 *    \#define FOO(i, _) NRF_PWM ## i ,
 *    { UTIL_LISTIFY(PWM_COUNT, FOO) }
 *    The above two lines will generate the below:
 *    { NRF_PWM0 , NRF_PWM1 , }
 *
 * @note Calling UTIL_LISTIFY with undefined arguments has undefined
 * behavior.
 */
#define UTIL_LISTIFY(LEN, F, ...) UTIL_EVAL(UTIL_REPEAT(LEN, F, __VA_ARGS__))

/* Set of internal macros used for FOR_EACH series of macros. */
#define Z_FOR_EACH_IDX(count, n, macro, semicolon, fixed_arg0, fixed_arg1, ...)\
	UTIL_WHEN(count)						\
	(								\
		UTIL_OBSTRUCT(macro)					\
		(							\
			fixed_arg0, fixed_arg1, n, GET_ARG1(__VA_ARGS__)\
		)semicolon						\
		UTIL_OBSTRUCT(Z_FOR_EACH_IDX_INDIRECT) ()		\
		(							\
			UTIL_DEC(count), UTIL_INC(n), macro, semicolon, \
			fixed_arg0, fixed_arg1,				\
			GET_ARGS_LESS_1(__VA_ARGS__)			\
		)							\
	)

#define Z_FOR_EACH_IDX_INDIRECT() Z_FOR_EACH_IDX

#define Z_FOR_EACH_IDX2(count, iter, macro, sc, fixed_arg0, fixed_arg1, ...) \
	UTIL_EVAL(Z_FOR_EACH_IDX(count, iter, macro, sc,\
				 fixed_arg0, fixed_arg1, __VA_ARGS__))

#define Z_FOR_EACH_SWALLOW_NOTHING(F, fixed_arg, index, arg) \
	F(index, arg, fixed_arg)

#define Z_FOR_EACH_SWALLOW_FIXED_ARG(F, fixed_arg, index, arg) F(index, arg)

#define Z_FOR_EACH_SWALLOW_INDEX_FIXED_ARG(F, fixed_arg, index, arg) F(arg)
#define Z_FOR_EACH_SWALLOW_INDEX(F, fixed_arg, index, arg) F(arg, fixed_arg)

/**
 * @brief Calls macro F for each provided argument with index as first argument
 *	  and nth parameter as the second argument.
 *
 * Example:
 *
 *     #define F(idx, x) int a##idx = x;
 *     FOR_EACH_IDX(F, 4, 5, 6)
 *
 * will result in following code:
 *
 *     int a0 = 4;
 *     int a1 = 5;
 *     int a2 = 6;
 *
 * @param F Macro takes index and first argument and nth variable argument as
 *	    the second one.
 * @param ... Variable list of argument. For each argument macro F is executed.
 */
#define FOR_EACH_IDX(F, ...) \
	Z_FOR_EACH_IDX2(NUM_VA_ARGS_LESS_1(__VA_ARGS__, _), \
			0, Z_FOR_EACH_SWALLOW_FIXED_ARG, /*no ;*/, \
			F, 0, __VA_ARGS__)

/**
 * @brief Calls macro F for each provided argument with index as first argument
 *	  and nth parameter as the second argument and fixed argument as the
 *	  third one.
 *
 * Example:
 *
 *     #define F(idx, x, fixed_arg) int fixed_arg##idx = x;
 *     FOR_EACH_IDX_FIXED_ARG(F, a, 4, 5, 6)
 *
 * will result in following code:
 *
 *     int a0 = 4;
 *     int a1 = 5;
 *     int a2 = 6;
 *
 * @param F Macro takes index and first argument and nth variable argument as
 *	    the second one and fixed argumnet as the third.
 * @param fixed_arg Fixed argument passed to F macro.
 * @param ... Variable list of argument. For each argument macro F is executed.
 */
#define FOR_EACH_IDX_FIXED_ARG(F, fixed_arg, ...) \
	Z_FOR_EACH_IDX2(NUM_VA_ARGS_LESS_1(__VA_ARGS__, _), \
			0, Z_FOR_EACH_SWALLOW_NOTHING, /*no ;*/, \
			F, fixed_arg, __VA_ARGS__)

/**
 * @brief Calls macro F for each provided argument.
 *
 * Example:
 *
 *     #define F(x) int a##x;
 *     FOR_EACH(F, 4, 5, 6)
 *
 * will result in following code:
 *
 *     int a4;
 *     int a5;
 *     int a6;
 *
 * @param F Macro takes nth variable argument as the argument.
 * @param ... Variable list of argument. For each argument macro F is executed.
 */
#define FOR_EACH(F, ...) \
	Z_FOR_EACH_IDX2(NUM_VA_ARGS_LESS_1(__VA_ARGS__, _), \
			0, Z_FOR_EACH_SWALLOW_INDEX_FIXED_ARG, /*no ;*/, \
			F, 0, __VA_ARGS__)

/**
 * @brief Calls macro F for each provided argument with additional fixed
 *	  argument.
 *
 * After each iteration semicolon is added.
 *
 * Example:
 *
 *     static void func(int val, void *dev);
 *     FOR_EACH_FIXED_ARG(func, dev, 4, 5, 6)
 *
 * will result in following code:
 *
 *     func(4, dev);
 *     func(5, dev);
 *     func(6, dev);
 *
 * @param F Macro takes nth variable argument as the first parameter and
 *	     fixed argument as the second parameter.
 * @param fixed_arg Fixed argument forward to macro execution for each argument.
 * @param ... Variable list of argument. For each argument macro F is executed.
 */
#define FOR_EACH_FIXED_ARG(F, fixed_arg, ...) \
	Z_FOR_EACH_IDX2(NUM_VA_ARGS_LESS_1(__VA_ARGS__, _), \
			0, Z_FOR_EACH_SWALLOW_INDEX, ;, \
			F, fixed_arg, __VA_ARGS__)

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

/**
 * @brief Macro to get the number of arguments in a call variadic macro call.
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
 * Macro that process all arguments using given macro
 *
 * @deprecated Use FOR_EACH instead.
 *
 * @param ... Macro name to be used for argument processing followed by
 *            arguments to process. Macro should have following
 *            form: MACRO(argument).
 *
 * @return All arguments processed by given macro
 */
#define MACRO_MAP(...) __DEPRECATED_MACRO FOR_EACH(__VA_ARGS__)

/**
 * @brief Mapping macro that pastes results together
 *
 * Like @ref MACRO_MAP(), but pastes the results together into a
 * single token by repeated application of @ref UTIL_CAT().
 *
 * For example, with this macro FOO:
 *
 *     #define FOO(x) item_##x##_
 *
 * MACRO_MAP_CAT(FOO, a, b, c) expands to the token:
 *
 *     item_a_item_b_item_c_
 *
 * @param ... Macro to expand on each argument, followed by its
 *            arguments. (The macro should take exactly one argument.)
 * @return The results of expanding the macro on each argument, all pasted
 *         together
 */
#define MACRO_MAP_CAT(...) MACRO_MAP_CAT_(__VA_ARGS__)
#define MACRO_MAP_CAT_(...)						\
	/* To make sure it works also for 2 arguments in total */	\
	MACRO_MAP_CAT_N(NUM_VA_ARGS_LESS_1(__VA_ARGS__), __VA_ARGS__)

/**
 * @brief Mapping macro that pastes a fixed number of results together
 *
 * Similar to @ref MACRO_MAP_CAT(), but expects a fixed number of
 * arguments. If more arguments are given than are expected, the rest
 * are ignored.
 *
 * @param N   Number of arguments to map
 * @param ... Macro to expand on each argument, followed by its
 *            arguments. (The macro should take exactly one argument.)
 * @return The results of expanding the macro on each argument, all pasted
 *         together
 */
#define MACRO_MAP_CAT_N(N, ...) MACRO_MAP_CAT_N_(N, __VA_ARGS__)
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_H_ */
