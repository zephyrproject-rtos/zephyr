/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CBPRINTF_INTERNAL_H_
#define ZEPHYR_INCLUDE_SYS_CBPRINTF_INTERNAL_H_

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/arch/cpu.h>

/*
 * Special alignment cases
 */

#if defined(__i386__)
/* there are no gaps on the stack */
#define VA_STACK_ALIGN(type)	1
#elif defined(__sparc__)
/* there are no gaps on the stack */
#define VA_STACK_ALIGN(type)	1
#elif defined(__x86_64__)
#define VA_STACK_MIN_ALIGN	8
#elif defined(__aarch64__)
#define VA_STACK_MIN_ALIGN	8
#elif defined(CONFIG_ARC)
#define VA_STACK_MIN_ALIGN	ARCH_STACK_PTR_ALIGN
#elif defined(__riscv)
#ifdef CONFIG_RISCV_ISA_RV32E
#define VA_STACK_ALIGN(type)	4
#else
#define VA_STACK_MIN_ALIGN	(__riscv_xlen / 8)
#endif /* CONFIG_RISCV_ISA_RV32E */
#endif

/*
 * Default alignment values if not specified by architecture config
 */

#ifndef VA_STACK_MIN_ALIGN
#define VA_STACK_MIN_ALIGN	1
#endif

#ifndef VA_STACK_ALIGN
#define VA_STACK_ALIGN(type)	MAX(VA_STACK_MIN_ALIGN, __alignof__(type))
#endif

static inline void z_cbprintf_wcpy(int *dst, int *src, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

#include <zephyr/sys/cbprintf_cxx.h>

#ifdef __cplusplus
extern "C" {
#endif


#if defined(__sparc__)
/* The SPARC V8 ABI guarantees that the arguments of a variable argument
 * list function are stored on the stack at addresses which are 32-bit
 * aligned. It means that variables of type unit64_t and double may not
 * be properly aligned on the stack.
 *
 * The compiler is aware of the ABI and takes care of this. However,
 * as we are directly accessing the variable argument list here, we need
 * to take the alignment into consideration and copy 64-bit arguments
 * as 32-bit words.
 */
#define Z_CBPRINTF_VA_STACK_LL_DBL_MEMCPY	1
#else
#define Z_CBPRINTF_VA_STACK_LL_DBL_MEMCPY	0
#endif

/** @brief Return 1 if argument is a pointer to char or wchar_t
 *
 * @param x argument.
 *
 * @return 1 if char * or wchar_t *, 0 otherwise.
 */
#ifdef __cplusplus
#define Z_CBPRINTF_IS_PCHAR(x, flags) \
	z_cbprintf_cxx_is_pchar(x, (flags) & CBPRINTF_PACKAGE_CONST_CHAR_RO)
#else
#define Z_CBPRINTF_IS_PCHAR(x, flags) \
	_Generic((x) + 0, \
		/* char * */ \
		char * : 1, \
		const char * : ((flags) & CBPRINTF_PACKAGE_CONST_CHAR_RO) ? 0 : 1, \
		volatile char * : 1, \
		const volatile char * : 1, \
		/* unsigned char * */ \
		unsigned char * : 1, \
		const unsigned char * : ((flags) & CBPRINTF_PACKAGE_CONST_CHAR_RO) ? 0 : 1, \
		volatile unsigned char * : 1, \
		const volatile unsigned char * : 1,\
		/* wchar_t * */ \
		wchar_t * : 1, \
		const wchar_t * : ((flags) & CBPRINTF_PACKAGE_CONST_CHAR_RO) ? 0 : 1, \
		volatile wchar_t * : 1, \
		const volatile wchar_t * : 1, \
		default : \
			0)
#endif

/** @brief Check if argument fits in 32 bit word.
 *
 * @param x Input argument.
 *
 * @retval 1 if variable is of type that fits in 32 bit word.
 * @retval 0 if variable is of different type.
 */
#ifdef __cplusplus
#define Z_CBPRINTF_IS_WORD_NUM(x) \
	z_cbprintf_cxx_is_word_num(x)
#else
#define Z_CBPRINTF_IS_WORD_NUM(x) \
	_Generic(x, \
		char : 1, \
		unsigned char : 1, \
		short : 1, \
		unsigned short : 1, \
		int : 1, \
		unsigned int : 1, \
		long : sizeof(long) <= 4, \
		unsigned long : sizeof(long) <= 4, \
		default : \
			0)
#endif

/** @brief Check if argument is a none character pointer.
 *
 * @note Macro triggers a pointer arithmetic warning and usage shall be wrapped in
 * the pragma that suppresses this warning.
 *
 * @param x Input argument.
 *
 * @retval 1 if variable is a none character pointer.
 * @retval 0 if variable is of different type.
 */
#ifdef __cplusplus
#define Z_CBPRINTF_IS_NONE_CHAR_PTR(x) z_cbprintf_cxx_is_none_char_ptr(x)
#else
#define Z_CBPRINTF_IS_NONE_CHAR_PTR(x) \
	_Generic((x) + 0, \
		char * : 0, \
		volatile char * : 0, \
		const char * : 0, \
		const volatile char * : 0, \
		unsigned char * : 0, \
		volatile unsigned char * : 0, \
		const unsigned char * : 0, \
		const volatile unsigned char * : 0, \
		char: 0, \
		unsigned char: 0, \
		short: 0, \
		unsigned short: 0, \
		int: 0, \
		unsigned int: 0,\
		long: 0, \
		unsigned long: 0,\
		long long: 0, \
		unsigned long long: 0, \
		float: 0, \
		double: 0, \
		default : \
			1)
#endif

/** @brief Get number of none character pointers in the string with at least 1 argument.
 *
 * @param ... String with at least 1 argument.
 *
 * @return Number of none character pointer arguments.
 */
#define Z_CBPRINTF_NONE_CHAR_PTR_ARGS(...) \
	(FOR_EACH(Z_CBPRINTF_IS_NONE_CHAR_PTR, (+),  __VA_ARGS__)) \

/** @brief Get number of none character pointers in the string argument list.
 *
 * @param ... Format string with arguments.
 *
 * @return Number of none character pointer arguments.
 */
#define Z_CBPRINTF_NONE_CHAR_PTR_COUNT(...) \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
		    (0), \
		    (Z_CBPRINTF_NONE_CHAR_PTR_ARGS(GET_ARGS_LESS_N(1, __VA_ARGS__))))

/** @brief Calculate number of pointer format specifiers in the string.
 *
 * If constant string is provided then result is calculated at compile time
 * however for it is not consider constant by the compiler, e.g. can not be
 * used in the static assert.
 *
 * String length is limited to 256.
 *
 * @param fmt Format string.
 * @param ... String arguments.
 *
 * @return Number of %p format specifiers in the string.
 */
#define Z_CBPRINTF_P_COUNT(fmt, ...) \
	((sizeof(fmt) >= 2 && fmt[0] == '%' && fmt[1] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 3 && fmt[0] != '%' && fmt[1] == '%' && fmt[2] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 4 && fmt[1] != '%' && fmt[2] == '%' && fmt[3] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 5 && fmt[2] != '%' && fmt[3] == '%' && fmt[4] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 6 && fmt[3] != '%' && fmt[4] == '%' && fmt[5] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 7 && fmt[4] != '%' && fmt[5] == '%' && fmt[6] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 8 && fmt[5] != '%' && fmt[6] == '%' && fmt[7] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 9 && fmt[6] != '%' && fmt[7] == '%' && fmt[8] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 10 && fmt[7] != '%' && fmt[8] == '%' && fmt[9] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 11 && fmt[8] != '%' && fmt[9] == '%' && fmt[10] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 12 && fmt[9] != '%' && fmt[10] == '%' && fmt[11] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 13 && fmt[10] != '%' && fmt[11] == '%' && fmt[12] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 14 && fmt[11] != '%' && fmt[12] == '%' && fmt[13] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 15 && fmt[12] != '%' && fmt[13] == '%' && fmt[14] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 16 && fmt[13] != '%' && fmt[14] == '%' && fmt[15] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 17 && fmt[14] != '%' && fmt[15] == '%' && fmt[16] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 18 && fmt[15] != '%' && fmt[16] == '%' && fmt[17] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 19 && fmt[16] != '%' && fmt[17] == '%' && fmt[18] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 20 && fmt[17] != '%' && fmt[18] == '%' && fmt[19] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 21 && fmt[18] != '%' && fmt[19] == '%' && fmt[20] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 22 && fmt[19] != '%' && fmt[20] == '%' && fmt[21] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 23 && fmt[20] != '%' && fmt[21] == '%' && fmt[22] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 24 && fmt[21] != '%' && fmt[22] == '%' && fmt[23] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 25 && fmt[22] != '%' && fmt[23] == '%' && fmt[24] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 26 && fmt[23] != '%' && fmt[24] == '%' && fmt[25] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 27 && fmt[24] != '%' && fmt[25] == '%' && fmt[26] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 28 && fmt[25] != '%' && fmt[26] == '%' && fmt[27] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 29 && fmt[26] != '%' && fmt[27] == '%' && fmt[28] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 30 && fmt[27] != '%' && fmt[28] == '%' && fmt[29] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 31 && fmt[28] != '%' && fmt[29] == '%' && fmt[30] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 32 && fmt[29] != '%' && fmt[30] == '%' && fmt[31] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 33 && fmt[30] != '%' && fmt[31] == '%' && fmt[32] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 34 && fmt[31] != '%' && fmt[32] == '%' && fmt[33] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 35 && fmt[32] != '%' && fmt[33] == '%' && fmt[34] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 36 && fmt[33] != '%' && fmt[34] == '%' && fmt[35] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 37 && fmt[34] != '%' && fmt[35] == '%' && fmt[36] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 38 && fmt[35] != '%' && fmt[36] == '%' && fmt[37] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 39 && fmt[36] != '%' && fmt[37] == '%' && fmt[38] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 40 && fmt[37] != '%' && fmt[38] == '%' && fmt[39] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 41 && fmt[38] != '%' && fmt[39] == '%' && fmt[40] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 42 && fmt[39] != '%' && fmt[40] == '%' && fmt[41] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 43 && fmt[40] != '%' && fmt[41] == '%' && fmt[42] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 44 && fmt[41] != '%' && fmt[42] == '%' && fmt[43] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 45 && fmt[42] != '%' && fmt[43] == '%' && fmt[44] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 46 && fmt[43] != '%' && fmt[44] == '%' && fmt[45] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 47 && fmt[44] != '%' && fmt[45] == '%' && fmt[46] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 48 && fmt[45] != '%' && fmt[46] == '%' && fmt[47] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 49 && fmt[46] != '%' && fmt[47] == '%' && fmt[48] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 50 && fmt[47] != '%' && fmt[48] == '%' && fmt[49] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 51 && fmt[48] != '%' && fmt[49] == '%' && fmt[50] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 52 && fmt[49] != '%' && fmt[50] == '%' && fmt[51] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 53 && fmt[50] != '%' && fmt[51] == '%' && fmt[52] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 54 && fmt[51] != '%' && fmt[52] == '%' && fmt[53] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 55 && fmt[52] != '%' && fmt[53] == '%' && fmt[54] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 56 && fmt[53] != '%' && fmt[54] == '%' && fmt[55] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 57 && fmt[54] != '%' && fmt[55] == '%' && fmt[56] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 58 && fmt[55] != '%' && fmt[56] == '%' && fmt[57] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 59 && fmt[56] != '%' && fmt[57] == '%' && fmt[58] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 60 && fmt[57] != '%' && fmt[58] == '%' && fmt[59] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 61 && fmt[58] != '%' && fmt[59] == '%' && fmt[60] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 62 && fmt[59] != '%' && fmt[60] == '%' && fmt[61] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 63 && fmt[60] != '%' && fmt[61] == '%' && fmt[62] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 64 && fmt[61] != '%' && fmt[62] == '%' && fmt[63] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 65 && fmt[62] != '%' && fmt[63] == '%' && fmt[64] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 66 && fmt[63] != '%' && fmt[64] == '%' && fmt[65] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 67 && fmt[64] != '%' && fmt[65] == '%' && fmt[66] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 68 && fmt[65] != '%' && fmt[66] == '%' && fmt[67] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 69 && fmt[66] != '%' && fmt[67] == '%' && fmt[68] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 70 && fmt[67] != '%' && fmt[68] == '%' && fmt[69] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 71 && fmt[68] != '%' && fmt[69] == '%' && fmt[70] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 72 && fmt[69] != '%' && fmt[70] == '%' && fmt[71] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 73 && fmt[70] != '%' && fmt[71] == '%' && fmt[72] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 74 && fmt[71] != '%' && fmt[72] == '%' && fmt[73] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 75 && fmt[72] != '%' && fmt[73] == '%' && fmt[74] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 76 && fmt[73] != '%' && fmt[74] == '%' && fmt[75] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 77 && fmt[74] != '%' && fmt[75] == '%' && fmt[76] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 78 && fmt[75] != '%' && fmt[76] == '%' && fmt[77] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 79 && fmt[76] != '%' && fmt[77] == '%' && fmt[78] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 80 && fmt[77] != '%' && fmt[78] == '%' && fmt[79] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 81 && fmt[78] != '%' && fmt[79] == '%' && fmt[80] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 82 && fmt[79] != '%' && fmt[80] == '%' && fmt[81] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 83 && fmt[80] != '%' && fmt[81] == '%' && fmt[82] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 84 && fmt[81] != '%' && fmt[82] == '%' && fmt[83] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 85 && fmt[82] != '%' && fmt[83] == '%' && fmt[84] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 86 && fmt[83] != '%' && fmt[84] == '%' && fmt[85] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 87 && fmt[84] != '%' && fmt[85] == '%' && fmt[86] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 88 && fmt[85] != '%' && fmt[86] == '%' && fmt[87] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 89 && fmt[86] != '%' && fmt[87] == '%' && fmt[88] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 90 && fmt[87] != '%' && fmt[88] == '%' && fmt[89] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 91 && fmt[88] != '%' && fmt[89] == '%' && fmt[90] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 92 && fmt[89] != '%' && fmt[90] == '%' && fmt[91] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 93 && fmt[90] != '%' && fmt[91] == '%' && fmt[92] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 94 && fmt[91] != '%' && fmt[92] == '%' && fmt[93] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 95 && fmt[92] != '%' && fmt[93] == '%' && fmt[94] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 96 && fmt[93] != '%' && fmt[94] == '%' && fmt[95] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 97 && fmt[94] != '%' && fmt[95] == '%' && fmt[96] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 98 && fmt[95] != '%' && fmt[96] == '%' && fmt[97] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 99 && fmt[96] != '%' && fmt[97] == '%' && fmt[98] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 100 && fmt[97] != '%' && fmt[98] == '%' && fmt[99] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 101 && fmt[98] != '%' && fmt[99] == '%' && fmt[100] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 102 && fmt[99] != '%' && fmt[100] == '%' && fmt[101] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 103 && fmt[100] != '%' && fmt[101] == '%' && fmt[102] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 104 && fmt[101] != '%' && fmt[102] == '%' && fmt[103] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 105 && fmt[102] != '%' && fmt[103] == '%' && fmt[104] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 106 && fmt[103] != '%' && fmt[104] == '%' && fmt[105] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 107 && fmt[104] != '%' && fmt[105] == '%' && fmt[106] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 108 && fmt[105] != '%' && fmt[106] == '%' && fmt[107] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 109 && fmt[106] != '%' && fmt[107] == '%' && fmt[108] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 110 && fmt[107] != '%' && fmt[108] == '%' && fmt[109] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 111 && fmt[108] != '%' && fmt[109] == '%' && fmt[110] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 112 && fmt[109] != '%' && fmt[110] == '%' && fmt[111] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 113 && fmt[110] != '%' && fmt[111] == '%' && fmt[112] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 114 && fmt[111] != '%' && fmt[112] == '%' && fmt[113] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 115 && fmt[112] != '%' && fmt[113] == '%' && fmt[114] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 116 && fmt[113] != '%' && fmt[114] == '%' && fmt[115] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 117 && fmt[114] != '%' && fmt[115] == '%' && fmt[116] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 118 && fmt[115] != '%' && fmt[116] == '%' && fmt[117] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 119 && fmt[116] != '%' && fmt[117] == '%' && fmt[118] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 120 && fmt[117] != '%' && fmt[118] == '%' && fmt[119] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 121 && fmt[118] != '%' && fmt[119] == '%' && fmt[120] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 122 && fmt[119] != '%' && fmt[120] == '%' && fmt[121] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 123 && fmt[120] != '%' && fmt[121] == '%' && fmt[122] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 124 && fmt[121] != '%' && fmt[122] == '%' && fmt[123] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 125 && fmt[122] != '%' && fmt[123] == '%' && fmt[124] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 126 && fmt[123] != '%' && fmt[124] == '%' && fmt[125] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 127 && fmt[124] != '%' && fmt[125] == '%' && fmt[126] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 128 && fmt[125] != '%' && fmt[126] == '%' && fmt[127] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 129 && fmt[126] != '%' && fmt[127] == '%' && fmt[128] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 130 && fmt[127] != '%' && fmt[128] == '%' && fmt[129] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 131 && fmt[128] != '%' && fmt[129] == '%' && fmt[130] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 132 && fmt[129] != '%' && fmt[130] == '%' && fmt[131] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 133 && fmt[130] != '%' && fmt[131] == '%' && fmt[132] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 134 && fmt[131] != '%' && fmt[132] == '%' && fmt[133] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 135 && fmt[132] != '%' && fmt[133] == '%' && fmt[134] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 136 && fmt[133] != '%' && fmt[134] == '%' && fmt[135] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 137 && fmt[134] != '%' && fmt[135] == '%' && fmt[136] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 138 && fmt[135] != '%' && fmt[136] == '%' && fmt[137] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 139 && fmt[136] != '%' && fmt[137] == '%' && fmt[138] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 140 && fmt[137] != '%' && fmt[138] == '%' && fmt[139] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 141 && fmt[138] != '%' && fmt[139] == '%' && fmt[140] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 142 && fmt[139] != '%' && fmt[140] == '%' && fmt[141] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 143 && fmt[140] != '%' && fmt[141] == '%' && fmt[142] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 144 && fmt[141] != '%' && fmt[142] == '%' && fmt[143] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 145 && fmt[142] != '%' && fmt[143] == '%' && fmt[144] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 146 && fmt[143] != '%' && fmt[144] == '%' && fmt[145] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 147 && fmt[144] != '%' && fmt[145] == '%' && fmt[146] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 148 && fmt[145] != '%' && fmt[146] == '%' && fmt[147] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 149 && fmt[146] != '%' && fmt[147] == '%' && fmt[148] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 150 && fmt[147] != '%' && fmt[148] == '%' && fmt[149] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 151 && fmt[148] != '%' && fmt[149] == '%' && fmt[150] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 152 && fmt[149] != '%' && fmt[150] == '%' && fmt[151] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 153 && fmt[150] != '%' && fmt[151] == '%' && fmt[152] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 154 && fmt[151] != '%' && fmt[152] == '%' && fmt[153] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 155 && fmt[152] != '%' && fmt[153] == '%' && fmt[154] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 156 && fmt[153] != '%' && fmt[154] == '%' && fmt[155] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 157 && fmt[154] != '%' && fmt[155] == '%' && fmt[156] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 158 && fmt[155] != '%' && fmt[156] == '%' && fmt[157] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 159 && fmt[156] != '%' && fmt[157] == '%' && fmt[158] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 160 && fmt[157] != '%' && fmt[158] == '%' && fmt[159] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 161 && fmt[158] != '%' && fmt[159] == '%' && fmt[160] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 162 && fmt[159] != '%' && fmt[160] == '%' && fmt[161] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 163 && fmt[160] != '%' && fmt[161] == '%' && fmt[162] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 164 && fmt[161] != '%' && fmt[162] == '%' && fmt[163] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 165 && fmt[162] != '%' && fmt[163] == '%' && fmt[164] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 166 && fmt[163] != '%' && fmt[164] == '%' && fmt[165] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 167 && fmt[164] != '%' && fmt[165] == '%' && fmt[166] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 168 && fmt[165] != '%' && fmt[166] == '%' && fmt[167] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 169 && fmt[166] != '%' && fmt[167] == '%' && fmt[168] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 170 && fmt[167] != '%' && fmt[168] == '%' && fmt[169] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 171 && fmt[168] != '%' && fmt[169] == '%' && fmt[170] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 172 && fmt[169] != '%' && fmt[170] == '%' && fmt[171] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 173 && fmt[170] != '%' && fmt[171] == '%' && fmt[172] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 174 && fmt[171] != '%' && fmt[172] == '%' && fmt[173] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 175 && fmt[172] != '%' && fmt[173] == '%' && fmt[174] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 176 && fmt[173] != '%' && fmt[174] == '%' && fmt[175] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 177 && fmt[174] != '%' && fmt[175] == '%' && fmt[176] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 178 && fmt[175] != '%' && fmt[176] == '%' && fmt[177] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 179 && fmt[176] != '%' && fmt[177] == '%' && fmt[178] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 180 && fmt[177] != '%' && fmt[178] == '%' && fmt[179] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 181 && fmt[178] != '%' && fmt[179] == '%' && fmt[180] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 182 && fmt[179] != '%' && fmt[180] == '%' && fmt[181] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 183 && fmt[180] != '%' && fmt[181] == '%' && fmt[182] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 184 && fmt[181] != '%' && fmt[182] == '%' && fmt[183] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 185 && fmt[182] != '%' && fmt[183] == '%' && fmt[184] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 186 && fmt[183] != '%' && fmt[184] == '%' && fmt[185] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 187 && fmt[184] != '%' && fmt[185] == '%' && fmt[186] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 188 && fmt[185] != '%' && fmt[186] == '%' && fmt[187] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 189 && fmt[186] != '%' && fmt[187] == '%' && fmt[188] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 190 && fmt[187] != '%' && fmt[188] == '%' && fmt[189] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 191 && fmt[188] != '%' && fmt[189] == '%' && fmt[190] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 192 && fmt[189] != '%' && fmt[190] == '%' && fmt[191] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 193 && fmt[190] != '%' && fmt[191] == '%' && fmt[192] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 194 && fmt[191] != '%' && fmt[192] == '%' && fmt[193] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 195 && fmt[192] != '%' && fmt[193] == '%' && fmt[194] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 196 && fmt[193] != '%' && fmt[194] == '%' && fmt[195] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 197 && fmt[194] != '%' && fmt[195] == '%' && fmt[196] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 198 && fmt[195] != '%' && fmt[196] == '%' && fmt[197] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 199 && fmt[196] != '%' && fmt[197] == '%' && fmt[198] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 200 && fmt[197] != '%' && fmt[198] == '%' && fmt[199] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 201 && fmt[198] != '%' && fmt[199] == '%' && fmt[200] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 202 && fmt[199] != '%' && fmt[200] == '%' && fmt[201] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 203 && fmt[200] != '%' && fmt[201] == '%' && fmt[202] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 204 && fmt[201] != '%' && fmt[202] == '%' && fmt[203] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 205 && fmt[202] != '%' && fmt[203] == '%' && fmt[204] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 206 && fmt[203] != '%' && fmt[204] == '%' && fmt[205] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 207 && fmt[204] != '%' && fmt[205] == '%' && fmt[206] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 208 && fmt[205] != '%' && fmt[206] == '%' && fmt[207] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 209 && fmt[206] != '%' && fmt[207] == '%' && fmt[208] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 210 && fmt[207] != '%' && fmt[208] == '%' && fmt[209] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 211 && fmt[208] != '%' && fmt[209] == '%' && fmt[210] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 212 && fmt[209] != '%' && fmt[210] == '%' && fmt[211] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 213 && fmt[210] != '%' && fmt[211] == '%' && fmt[212] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 214 && fmt[211] != '%' && fmt[212] == '%' && fmt[213] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 215 && fmt[212] != '%' && fmt[213] == '%' && fmt[214] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 216 && fmt[213] != '%' && fmt[214] == '%' && fmt[215] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 217 && fmt[214] != '%' && fmt[215] == '%' && fmt[216] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 218 && fmt[215] != '%' && fmt[216] == '%' && fmt[217] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 219 && fmt[216] != '%' && fmt[217] == '%' && fmt[218] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 220 && fmt[217] != '%' && fmt[218] == '%' && fmt[219] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 221 && fmt[218] != '%' && fmt[219] == '%' && fmt[220] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 222 && fmt[219] != '%' && fmt[220] == '%' && fmt[221] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 223 && fmt[220] != '%' && fmt[221] == '%' && fmt[222] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 224 && fmt[221] != '%' && fmt[222] == '%' && fmt[223] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 225 && fmt[222] != '%' && fmt[223] == '%' && fmt[224] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 226 && fmt[223] != '%' && fmt[224] == '%' && fmt[225] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 227 && fmt[224] != '%' && fmt[225] == '%' && fmt[226] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 228 && fmt[225] != '%' && fmt[226] == '%' && fmt[227] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 229 && fmt[226] != '%' && fmt[227] == '%' && fmt[228] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 230 && fmt[227] != '%' && fmt[228] == '%' && fmt[229] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 231 && fmt[228] != '%' && fmt[229] == '%' && fmt[230] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 232 && fmt[229] != '%' && fmt[230] == '%' && fmt[231] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 233 && fmt[230] != '%' && fmt[231] == '%' && fmt[232] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 234 && fmt[231] != '%' && fmt[232] == '%' && fmt[233] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 235 && fmt[232] != '%' && fmt[233] == '%' && fmt[234] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 236 && fmt[233] != '%' && fmt[234] == '%' && fmt[235] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 237 && fmt[234] != '%' && fmt[235] == '%' && fmt[236] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 238 && fmt[235] != '%' && fmt[236] == '%' && fmt[237] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 239 && fmt[236] != '%' && fmt[237] == '%' && fmt[238] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 240 && fmt[237] != '%' && fmt[238] == '%' && fmt[239] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 241 && fmt[238] != '%' && fmt[239] == '%' && fmt[240] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 242 && fmt[239] != '%' && fmt[240] == '%' && fmt[241] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 243 && fmt[240] != '%' && fmt[241] == '%' && fmt[242] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 244 && fmt[241] != '%' && fmt[242] == '%' && fmt[243] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 245 && fmt[242] != '%' && fmt[243] == '%' && fmt[244] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 246 && fmt[243] != '%' && fmt[244] == '%' && fmt[245] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 247 && fmt[244] != '%' && fmt[245] == '%' && fmt[246] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 248 && fmt[245] != '%' && fmt[246] == '%' && fmt[247] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 249 && fmt[246] != '%' && fmt[247] == '%' && fmt[248] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 250 && fmt[247] != '%' && fmt[248] == '%' && fmt[249] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 251 && fmt[248] != '%' && fmt[249] == '%' && fmt[250] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 252 && fmt[249] != '%' && fmt[250] == '%' && fmt[251] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 253 && fmt[250] != '%' && fmt[251] == '%' && fmt[252] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 254 && fmt[251] != '%' && fmt[252] == '%' && fmt[253] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 255 && fmt[252] != '%' && fmt[253] == '%' && fmt[254] == 'p') ? 1 : 0) + \
	((sizeof(fmt) >= 256 && fmt[253] != '%' && fmt[254] == '%' && fmt[255] == 'p') ? 1 : 0)

/** @brief Determine if all %p arguments are none character pointer arguments.
 *
 * Static package creation relies on the assumption that character pointers are
 * only using %s arguments. To not confuse it with %p, any character pointer
 * that is used with %p should be casted to a pointer of a different type, e.g.
 * void *. This macro can be used to determine, at compile time, if such casting
 * is missing. It is determined at compile time but cannot be used for static
 * assertion so only runtime error reporting can be added.
 *
 * @note Macro triggers a pointer arithmetic warning and usage shall be wrapped in
 * the pragma that suppresses this warning.
 *
 * @param ... Format string with arguments.
 *
 * @retval True if string is okay.
 * @retval False if casting is missing.
 */
#define Z_CBPRINTF_POINTERS_VALIDATE(...) \
	(Z_CBPRINTF_NONE_CHAR_PTR_COUNT(__VA_ARGS__) == \
	 Z_CBPRINTF_P_COUNT(GET_ARG_N(1, __VA_ARGS__)))

/* @brief Check if argument is a certain type of char pointer. What exectly is checked
 * depends on @p flags. If flags is 0 then 1 is returned if @p x is a char pointer.
 *
 * @param idx Argument index.
 * @param x Argument.
 * @param flags Flags. See @p CBPRINTF_PACKAGE_FLAGS.
 *
 * @retval 1 if @p x is char pointer meeting criteria identified by @p flags.
 * @retval 0 otherwise.
 */
#define Z_CBPRINTF_IS_X_PCHAR(idx, x, flags) \
	  (idx < Z_CBPRINTF_PACKAGE_FIRST_RO_STR_CNT_GET(flags) ? \
		0 : Z_CBPRINTF_IS_PCHAR(x, flags))

/** @brief Calculate number of char * or wchar_t * arguments in the arguments.
 *
 * @param fmt string.
 *
 * @param ... string arguments.
 *
 * @return number of arguments which are char * or wchar_t *.
 */
#define Z_CBPRINTF_HAS_PCHAR_ARGS(flags, fmt, ...) \
	(FOR_EACH_IDX_FIXED_ARG(Z_CBPRINTF_IS_X_PCHAR, (+), flags, __VA_ARGS__))

#define Z_CBPRINTF_PCHAR_COUNT(flags, ...) \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
		    (0), \
		    (Z_CBPRINTF_HAS_PCHAR_ARGS(flags, __VA_ARGS__)))

/**
 * @brief Check if formatted string must be packaged in runtime.
 *
 * @param ... String with arguments (fmt, ...).
 *
 * @retval 1 if string must be packaged at runtime.
 * @retval 0 if string can be statically packaged.
 */
#if Z_C_GENERIC
#define Z_CBPRINTF_MUST_RUNTIME_PACKAGE(flags, ...) ({\
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
	int _rv; \
	if ((flags) & CBPRINTF_PACKAGE_ADD_RW_STR_POS) { \
		_rv = 0; \
	} else { \
		_rv = Z_CBPRINTF_PCHAR_COUNT(flags, __VA_ARGS__) > 0 ? 1 : 0; \
	} \
	_Pragma("GCC diagnostic pop")\
	_rv; \
})
#else
#define Z_CBPRINTF_MUST_RUNTIME_PACKAGE(flags, ...) 1
#endif

/** @brief Get storage size for given argument.
 *
 * Floats are promoted to double so they use size of double, others int storage
 * or it's own storage size if it is bigger than int.
 *
 * @param x argument.
 *
 * @return Number of bytes used for storing the argument.
 */
#ifdef __cplusplus
#define Z_CBPRINTF_ARG_SIZE(v) z_cbprintf_cxx_arg_size(v)
#else
#define Z_CBPRINTF_ARG_SIZE(v) ({\
	__auto_type __v = (v) + 0; \
	/* Static code analysis may complain about unused variable. */ \
	(void)__v; \
	size_t __arg_size = _Generic((v), \
		float : sizeof(double), \
		default : \
			sizeof((__v)) \
		); \
	__arg_size; \
})
#endif

/** @brief Promote and store argument in the buffer.
 *
 * @param buf Buffer.
 *
 * @param arg Argument.
 */
#ifdef __cplusplus
#define Z_CBPRINTF_STORE_ARG(buf, arg) z_cbprintf_cxx_store_arg(buf, arg)
#else
#define Z_CBPRINTF_STORE_ARG(buf, arg) do { \
	if (Z_CBPRINTF_VA_STACK_LL_DBL_MEMCPY) { \
		/* If required, copy arguments by word to avoid unaligned access.*/ \
		__auto_type _v = (arg) + 0; \
		double _d = _Generic((arg) + 0, \
				float : (arg) + 0, \
				default : \
					0.0); \
		/* Static code analysis may complain about unused variable. */ \
		(void)_v; \
		(void)_d; \
		size_t arg_size = Z_CBPRINTF_ARG_SIZE(arg); \
		size_t _wsize = arg_size / sizeof(int); \
		z_cbprintf_wcpy((int *)buf, \
			      (int *) _Generic((arg) + 0, float : &_d, default : &_v), \
			      _wsize); \
	} else { \
		*_Generic((arg) + 0, \
			char : (int *)buf, \
			unsigned char: (int *)buf, \
			short : (int *)buf, \
			unsigned short : (int *)buf, \
			int : (int *)buf, \
			unsigned int : (unsigned int *)buf, \
			long : (long *)buf, \
			unsigned long : (unsigned long *)buf, \
			long long : (long long *)buf, \
			unsigned long long : (unsigned long long *)buf, \
			float : (double *)buf, \
			double : (double *)buf, \
			long double : (long double *)buf, \
			default : \
				(const void **)buf) = arg; \
	} \
} while (false)
#endif

/** @brief Return alignment needed for given argument.
 *
 * @param _arg Argument
 *
 * @return Alignment in bytes.
 */
#ifdef __cplusplus
#define Z_CBPRINTF_ALIGNMENT(_arg) z_cbprintf_cxx_alignment(_arg)
#else
#define Z_CBPRINTF_ALIGNMENT(_arg) \
	MAX(_Generic((_arg) + 0, \
		float : VA_STACK_ALIGN(double), \
		double : VA_STACK_ALIGN(double), \
		long double : VA_STACK_ALIGN(long double), \
		long long : VA_STACK_ALIGN(long long), \
		unsigned long long : VA_STACK_ALIGN(long long), \
		default : \
			__alignof__((_arg) + 0)), VA_STACK_MIN_ALIGN)
#endif

/** @brief Detect long double variable as a constant expression.
 *
 * Macro is used in static assertion. On some platforms C++ static inline
 * template function is not a constant expression and cannot be used. In that
 * case long double usage will not be detected.
 *
 * @param x Argument.
 *
 * @return 1 if @p x is a long double, 0 otherwise.
 */
#ifdef __cplusplus
#if defined(__x86_64__) || defined(__riscv) || defined(__aarch64__)
#define Z_CBPRINTF_IS_LONGDOUBLE(x) 0
#else
#define Z_CBPRINTF_IS_LONGDOUBLE(x) z_cbprintf_cxx_is_longdouble(x)
#endif
#else
#define Z_CBPRINTF_IS_LONGDOUBLE(x) \
	_Generic((x) + 0, long double : 1, default : 0)
#endif

/** @brief Safely package arguments to a buffer.
 *
 * Argument is put into the buffer if capable buffer is provided. Length is
 * incremented even if data is not packaged.
 *
 * @param _buf buffer.
 *
 * @param _idx index. Index is postincremented.
 *
 * @param _align_offset Current index with alignment offset.
 *
 * @param _max maximum index (buffer capacity).
 *
 * @param _arg argument.
 */
#define Z_CBPRINTF_PACK_ARG2(arg_idx, _buf, _idx, _align_offset, _max, _arg) \
do { \
	BUILD_ASSERT(!((sizeof(double) < VA_STACK_ALIGN(long double)) && \
			Z_CBPRINTF_IS_LONGDOUBLE(_arg) && \
			!IS_ENABLED(CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE)),\
			"Packaging of long double not enabled in Kconfig."); \
	while (_align_offset % Z_CBPRINTF_ALIGNMENT(_arg) != 0UL) { \
		_idx += sizeof(int); \
		_align_offset += sizeof(int); \
	} \
	uint32_t _arg_size = Z_CBPRINTF_ARG_SIZE(_arg); \
	uint32_t _loc = _idx / sizeof(int); \
	if (arg_idx < 1 + _fros_cnt) { \
		if (_ros_pos_en) { \
			_ros_pos_buf[_ros_pos_idx++] = _loc; \
		} \
	} else if (Z_CBPRINTF_IS_PCHAR(_arg, 0)) { \
		if (_cros_en) { \
			if (Z_CBPRINTF_IS_X_PCHAR(arg_idx, _arg, _flags)) { \
				if (_rws_pos_en) { \
					_rws_buffer[_rws_pos_idx++] = arg_idx - 1; \
					_rws_buffer[_rws_pos_idx++] = _loc; \
				} \
			} else { \
				if (_ros_pos_en) { \
					_ros_pos_buf[_ros_pos_idx++] = _loc; \
				} \
			} \
		} else if (_rws_pos_en) { \
			_rws_buffer[_rws_pos_idx++] = arg_idx - 1; \
			_rws_buffer[_rws_pos_idx++] = _idx / sizeof(int); \
		} \
	} \
	if (_buf && _idx < (int)_max) { \
		Z_CBPRINTF_STORE_ARG(&_buf[_idx], _arg); \
	} \
	_idx += _arg_size; \
	_align_offset += _arg_size; \
} while (false)

/** @brief Package single argument.
 *
 * Macro is called in a loop for each argument in the string.
 *
 * @param arg argument.
 */
#define Z_CBPRINTF_PACK_ARG(arg_idx, arg) \
	Z_CBPRINTF_PACK_ARG2(arg_idx, _pbuf, _pkg_len, _pkg_offset, _pmax, arg)

/* When using clang additional warning needs to be suppressed since each
 * argument of fmt string is used for sizeof() which results in the warning
 * if argument is a string literal. Suppression is added here instead of
 * the macro which generates the warning to not slow down the compiler.
 */
#ifdef __clang__
#define Z_CBPRINTF_SUPPRESS_SIZEOF_ARRAY_DECAY \
	_Pragma("GCC diagnostic ignored \"-Wsizeof-array-decay\"")
#else
#define Z_CBPRINTF_SUPPRESS_SIZEOF_ARRAY_DECAY
#endif

/* Allocation to avoid using VLA and alloca. Alloc frees space when leaving
 * a function which can lead to increased stack usage if logging is used
 * multiple times. VLA is not always available.
 *
 * Use large array when optimization is off to avoid increased stack usage.
 */
#ifdef CONFIG_NO_OPTIMIZATIONS
#define Z_CBPRINTF_ON_STACK_ALLOC(_name, _len) \
	__ASSERT(_len <= 32, "Too many string arguments."); \
	uint8_t _name##_buf32[32]; \
	_name = _name##_buf32
#else
#define Z_CBPRINTF_ON_STACK_ALLOC(_name, _len) \
	__ASSERT(_len <= 32, "Too many string arguments."); \
	uint8_t _name##_buf4[4]; \
	uint8_t _name##_buf8[8]; \
	uint8_t _name##_buf12[12]; \
	uint8_t _name##_buf16[16]; \
	uint8_t _name##_buf32[32]; \
	_name = (_len) <= 4 ? _name##_buf4 : \
		((_len) <= 8 ? _name##_buf8 : \
		((_len) <= 12 ? _name##_buf12 : \
		((_len) <= 16 ? _name##_buf16 : \
		 _name##_buf32)))
#endif

/** @brief Statically package a formatted string with arguments.
 *
 * @param buf buffer. If null then only length is calculated.
 *
 * @param _inlen buffer capacity on input. Ignored when @p buf is null.
 *
 * @param _outlen number of bytes required to store the package.
 *
 * @param _align_offset Input buffer alignment offset in words. Where offset 0
 * means that buffer is aligned to CBPRINTF_PACKAGE_ALIGNMENT.
 *
 * @param flags Option flags. See @ref CBPRINTF_PACKAGE_FLAGS.
 *
 * @param ... String with variable list of arguments.
 */
#define Z_CBPRINTF_STATIC_PACKAGE_GENERIC(buf, _inlen, _outlen, _align_offset, \
					  flags, ... /* fmt, ... */) \
do { \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
	Z_CBPRINTF_SUPPRESS_SIZEOF_ARRAY_DECAY \
	BUILD_ASSERT(!IS_ENABLED(CONFIG_XTENSA) || \
		     (IS_ENABLED(CONFIG_XTENSA) && \
		      !(_align_offset % CBPRINTF_PACKAGE_ALIGNMENT)), \
			"Xtensa requires aligned package."); \
	BUILD_ASSERT((_align_offset % sizeof(int)) == 0, \
			"Alignment offset must be multiply of a word."); \
	IF_ENABLED(CONFIG_CBPRINTF_STATIC_PACKAGE_CHECK_ALIGNMENT, \
		(__ASSERT(!((uintptr_t)buf & (CBPRINTF_PACKAGE_ALIGNMENT - 1)), \
			  "Buffer must be aligned.");)) \
	uint32_t _flags = flags; \
	bool _ros_pos_en = (_flags) & CBPRINTF_PACKAGE_ADD_RO_STR_POS; \
	bool _rws_pos_en = (_flags) & CBPRINTF_PACKAGE_ADD_RW_STR_POS; \
	bool _cros_en = (_flags) & CBPRINTF_PACKAGE_CONST_CHAR_RO; \
	uint8_t *_pbuf = buf; \
	uint8_t _rws_pos_idx = 0; \
	uint8_t _ros_pos_idx = 0; \
	/* Variable holds count of all string pointer arguments. */ \
	uint8_t _alls_cnt = Z_CBPRINTF_PCHAR_COUNT(0, __VA_ARGS__); \
	uint8_t _fros_cnt = Z_CBPRINTF_PACKAGE_FIRST_RO_STR_CNT_GET(_flags); \
	/* Variable holds count of non const string pointers. */ \
	uint8_t _rws_cnt = _cros_en ? \
		Z_CBPRINTF_PCHAR_COUNT(_flags, __VA_ARGS__) : _alls_cnt - _fros_cnt; \
	uint8_t _ros_cnt = _ros_pos_en ? (1 + _alls_cnt - _rws_cnt) : 0; \
	uint8_t *_ros_pos_buf; \
	Z_CBPRINTF_ON_STACK_ALLOC(_ros_pos_buf, _ros_cnt); \
	uint8_t *_rws_buffer; \
	Z_CBPRINTF_ON_STACK_ALLOC(_rws_buffer, 2 * _rws_cnt); \
	size_t _pmax = !is_null_no_warn(buf) ? _inlen : INT32_MAX; \
	int _pkg_len = 0; \
	int _total_len = 0; \
	int _pkg_offset = _align_offset; \
	union cbprintf_package_hdr *_len_loc; \
	/* If string has rw string arguments CBPRINTF_PACKAGE_ADD_RW_STR_POS is a must. */ \
	if (_rws_cnt && !((_flags) & CBPRINTF_PACKAGE_ADD_RW_STR_POS)) { \
		_outlen = -EINVAL; \
		break; \
	} \
	/* package starts with string address and field with length */ \
	if (_pmax < sizeof(*_len_loc)) { \
		_outlen = -ENOSPC; \
		break; \
	} \
	_len_loc = (union cbprintf_package_hdr *)_pbuf; \
	_pkg_len += sizeof(*_len_loc); \
	_pkg_offset += sizeof(*_len_loc); \
	/* Pack remaining arguments */\
	FOR_EACH_IDX(Z_CBPRINTF_PACK_ARG, (;), __VA_ARGS__);\
	_total_len = _pkg_len; \
	/* Append string indexes to the package. */ \
	_total_len += _ros_cnt; \
	_total_len += 2 * _rws_cnt; \
	if (_pbuf != NULL) { \
		/* Append string locations. */ \
		uint8_t *_pbuf_loc = &_pbuf[_pkg_len]; \
		for (size_t _ros_idx = 0; _ros_idx < _ros_cnt; _ros_idx++) { \
			*_pbuf_loc++ = _ros_pos_buf[_ros_idx]; \
		} \
		for (size_t _rws_idx = 0; _rws_idx < (2 * _rws_cnt); _rws_idx++) { \
			*_pbuf_loc++ = _rws_buffer[_rws_idx]; \
		} \
	} \
	/* Store length */ \
	_outlen = (_total_len > (int)_pmax) ? -ENOSPC : _total_len; \
	/* Store length in the header, set number of dumped strings to 0 */ \
	if (_pbuf != NULL) { \
		union cbprintf_package_hdr pkg_hdr = { \
			.desc = { \
				.len = (uint8_t)(_pkg_len / sizeof(int)), \
				.str_cnt = 0, \
				.ro_str_cnt = _ros_cnt, \
				.rw_str_cnt = _rws_cnt, \
			} \
		}; \
		IF_ENABLED(CONFIG_CBPRINTF_PACKAGE_HEADER_STORE_CREATION_FLAGS, \
			   (pkg_hdr.desc.pkg_flags = flags)); \
		*_len_loc = pkg_hdr; \
	} \
	_Pragma("GCC diagnostic pop") \
} while (false)

#if Z_C_GENERIC
#define Z_CBPRINTF_STATIC_PACKAGE(packaged, inlen, outlen, align_offset, flags, \
				  ... /* fmt, ... */) \
	Z_CBPRINTF_STATIC_PACKAGE_GENERIC(packaged, inlen, outlen, \
					  align_offset, flags, __VA_ARGS__)
#else
#define Z_CBPRINTF_STATIC_PACKAGE(packaged, inlen, outlen, align_offset, flags, \
				  ... /* fmt, ... */) \
do { \
	/* Small trick needed to avoid warning on always true */ \
	if (((uintptr_t)packaged + 1) != 1) { \
		outlen = cbprintf_package(packaged, inlen, flags, __VA_ARGS__); \
	} else { \
		outlen = cbprintf_package(NULL, align_offset, flags, __VA_ARGS__); \
	} \
} while (false)
#endif /* Z_C_GENERIC */

#ifdef __cplusplus
}
#endif

#ifdef CONFIG_CBPRINTF_PACKAGE_SUPPORT_TAGGED_ARGUMENTS
#ifdef __cplusplus
/*
 * Remove qualifiers like const, volatile. And also transform
 * C++ argument reference back to its basic type.
 */
#define Z_CBPRINTF_ARG_REMOVE_QUAL(arg) \
	z_cbprintf_cxx_remove_cv < \
		z_cbprintf_cxx_remove_reference < decltype(arg) > ::type \
	> ::type

/*
 * Get the type of elements in an array.
 */
#define Z_CBPRINTF_CXX_ARG_ARRAY_TYPE(arg) \
	z_cbprintf_cxx_remove_cv < \
		z_cbprintf_cxx_remove_extent < decltype(arg) > ::type \
	> ::type

/*
 * Determine if incoming type is char.
 */
#define Z_CBPRINTF_CXX_ARG_IS_TYPE_CHAR(type) \
	(z_cbprintf_cxx_is_same_type < type, \
	 char > :: value ? \
	 true : \
	 (z_cbprintf_cxx_is_same_type < type, \
	  const char > :: value ? \
	  true : \
	  (z_cbprintf_cxx_is_same_type < type, \
	   volatile char > :: value ? \
	   true : \
	   (z_cbprintf_cxx_is_same_type < type, \
	    const volatile char > :: value ? \
	    true : \
	    false))))

/*
 * Figure out if this is a char array since (char *) and (char[])
 * are of different types in C++.
 */
#define Z_CBPRINTF_CXX_ARG_IS_CHAR_ARRAY(arg) \
	(z_cbprintf_cxx_is_array < decltype(arg) > :: value ? \
	 (Z_CBPRINTF_CXX_ARG_IS_TYPE_CHAR(Z_CBPRINTF_CXX_ARG_ARRAY_TYPE(arg)) ? \
	  true : \
	  false) : \
	 false)

/*
 * Note that qualifiers of char * must be explicitly matched
 * due to type matching in C++, where remove_cv() does not work.
 */
#define Z_CBPRINTF_ARG_TYPE(arg) \
	(z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
	 char > ::value ? \
	 CBPRINTF_PACKAGE_ARG_TYPE_CHAR : \
	 (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
	  unsigned char > ::value ? \
	  CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_CHAR : \
	  (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
	   short > ::value ? \
	   CBPRINTF_PACKAGE_ARG_TYPE_SHORT : \
	   (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
	    unsigned short > ::value ? \
	    CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_SHORT : \
	    (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
	     int > ::value ? \
	     CBPRINTF_PACKAGE_ARG_TYPE_INT : \
	     (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
	      unsigned int > ::value ? \
	      CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_INT : \
	      (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
	       long > ::value ? \
	       CBPRINTF_PACKAGE_ARG_TYPE_LONG : \
	       (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
		unsigned long > ::value ? \
		CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_LONG : \
		(z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
		 long long > ::value ? \
		 CBPRINTF_PACKAGE_ARG_TYPE_LONG_LONG : \
		 (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
		  unsigned long long > ::value ? \
		  CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_LONG_LONG : \
		  (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
		   float > ::value ? \
		   CBPRINTF_PACKAGE_ARG_TYPE_FLOAT : \
		   (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
		    double > ::value ? \
		    CBPRINTF_PACKAGE_ARG_TYPE_DOUBLE : \
		    (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
		     long double > ::value ? \
		     CBPRINTF_PACKAGE_ARG_TYPE_LONG_DOUBLE : \
		      (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
		       char * > :: value ? \
		       CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR : \
		       (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
			const char * > :: value ? \
			CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR : \
			(z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
			 volatile char * > :: value ? \
			 CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR : \
			 (z_cbprintf_cxx_is_same_type < Z_CBPRINTF_ARG_REMOVE_QUAL(arg), \
			  const volatile char * > :: value ? \
			  CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR : \
			  (Z_CBPRINTF_CXX_ARG_IS_CHAR_ARRAY(arg) ? \
			   CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR : \
			   CBPRINTF_PACKAGE_ARG_TYPE_PTR_VOID))))))))))))))))))
#else
#define Z_CBPRINTF_ARG_TYPE(arg) \
	_Generic(arg, \
		char : CBPRINTF_PACKAGE_ARG_TYPE_CHAR, \
		unsigned char : CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_CHAR, \
		short : CBPRINTF_PACKAGE_ARG_TYPE_SHORT, \
		unsigned short : CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_SHORT, \
		int : CBPRINTF_PACKAGE_ARG_TYPE_INT, \
		unsigned int : CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_INT, \
		long : CBPRINTF_PACKAGE_ARG_TYPE_LONG, \
		unsigned long : CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_LONG, \
		long long : CBPRINTF_PACKAGE_ARG_TYPE_LONG_LONG, \
		unsigned long long : CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_LONG_LONG, \
		float : CBPRINTF_PACKAGE_ARG_TYPE_FLOAT, \
		double : CBPRINTF_PACKAGE_ARG_TYPE_DOUBLE, \
		long double : CBPRINTF_PACKAGE_ARG_TYPE_LONG_DOUBLE, \
		char * : CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR, \
		const char * : CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR, \
		void * : CBPRINTF_PACKAGE_ARG_TYPE_PTR_VOID, \
		default : \
			CBPRINTF_PACKAGE_ARG_TYPE_PTR_VOID \
	)
#endif /* _cplusplus */

#define Z_CBPRINTF_TAGGED_EMPTY_ARGS(...) \
	CBPRINTF_PACKAGE_ARG_TYPE_END

#define Z_CBPRINTF_TAGGED_ARGS_3(arg) \
	Z_CBPRINTF_ARG_TYPE(arg), arg

#define Z_CBPRINTF_TAGGED_ARGS_2(...) \
	FOR_EACH(Z_CBPRINTF_TAGGED_ARGS_3, (,), __VA_ARGS__), \
	CBPRINTF_PACKAGE_ARG_TYPE_END

#define Z_CBPRINTF_TAGGED_ARGS(_num_args, ...) \
	COND_CODE_0(_num_args, \
		    (CBPRINTF_PACKAGE_ARG_TYPE_END), \
		    (Z_CBPRINTF_TAGGED_ARGS_2(__VA_ARGS__)))

#endif /* CONFIG_CBPRINTF_PACKAGE_SUPPORT_TAGGED_ARGUMENTS */

#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_INTERNAL_H_ */
