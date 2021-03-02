/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CBPRINTF_INTERNAL_H_
#define ZEPHYR_INCLUDE_SYS_CBPRINTF_INTERNAL_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>
#include <sys/util.h>
#include <sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

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
#elif defined(__riscv)
#define VA_STACK_MIN_ALIGN	(__riscv_xlen / 8)
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

/** @brief Return 1 if argument is a pointer to char or wchar_t
 *
 * @param x argument.
 *
 * @return 1 if char * or wchar_t *, 0 otherwise.
 */
#define Z_CBPRINTF_IS_PCHAR(x) _Generic((x), \
			char * : 1, \
			const char * : 1, \
			volatile char * : 1, \
			const volatile char * : 1, \
			wchar_t * : 1, \
			const wchar_t * : 1, \
			volatile wchar_t * : 1, \
			const volatile wchar_t * : 1, \
			default : \
				0)

/** @brief Calculate number of char * or wchar_t * arguments in the arguments.
 *
 * @param fmt string.
 *
 * @param ... string arguments.
 *
 * @return number of arguments which are char * or wchar_t *.
 */
#define Z_CBPRINTF_HAS_PCHAR_ARGS(fmt, ...) \
	(FOR_EACH(Z_CBPRINTF_IS_PCHAR, (+),  __VA_ARGS__))

/**
 * @brief Check if formatted string must be packaged in runtime.
 *
 * @param skip number of char/wchar_t pointers in the argument list which are
 * accepted for static packaging.
 *
 * @param ... String with arguments (fmt, ...).
 *
 * @retval 1 if string must be packaged at runtime.
 * @retval 0 if string can be statically packaged.
 */
#if Z_C_GENERIC
#define Z_CBPRINTF_MUST_RUNTIME_PACKAGE(skip, ...) \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
			(0), \
			(((Z_CBPRINTF_HAS_PCHAR_ARGS(__VA_ARGS__) - skip) > 0)))
#else
#define Z_CBPRINTF_MUST_RUNTIME_PACKAGE(skip, ...) 1
#endif

/** @brief Get storage size for given argument.
 *
 * Floats are promoted to double so they use size of double, others int storage
 * or it's own storage size if it is bigger than int. Strings are stored in
 * the package with 1 byte header indicating if string is stored as pointer or
 * by value.
 *
 * @param x argument.
 *
 * @return Number of bytes used for storing the argument.
 */
#define Z_CBPRINTF_ARG_SIZE(v) \
	_Generic((v), \
		float : sizeof(double), \
		default : \
			_Generic((v), \
				void * : 0, \
				default : \
					sizeof((v)+0) \
				) \
		)

/** @brief Promote and store argument in the buffer.
 *
 * @param buf Buffer.
 *
 * @param arg Argument.
 */
#define Z_CBPRINTF_STORE_ARG(buf, arg) \
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
			(const void **)buf) = arg

/** @brief Return alignment needed for given argument.
 *
 * @param _arg Argument
 *
 * @return Alignment in bytes.
 */
#define Z_CBPRINTF_ALIGNMENT(_arg) \
	MAX(_Generic((_arg) + 0, \
		float : VA_STACK_ALIGN(double), \
		double : VA_STACK_ALIGN(double), \
		long double : VA_STACK_ALIGN(long double), \
		long long : VA_STACK_ALIGN(long long), \
		unsigned long long : VA_STACK_ALIGN(long long), \
		default : \
			__alignof__((_arg) + 0)), VA_STACK_MIN_ALIGN)

/** @brief Detect long double variable.
 *
 * @param x Argument.
 *
 * @return 1 if @p x is a long double, 0 otherwise.
 */
#define Z_CBPRINTF_IS_LONGDOUBLE(x) \
	_Generic((x) + 0, long double : 1, default : 0)

/** @brief Safely package arguments to a buffer.
 *
 * Argument is put into the buffer if capable buffer is provided. Length is
 * incremented even if data is not packaged.
 *
 * @param _buf buffer.
 *
 * @param _idx index. Index is postincremented.
 *
 * @param _max maximum index (buffer capacity).
 *
 * @param _arg argument.
 */
#define Z_CBPRINTF_PACK_ARG2(_buf, _idx, _max, _arg) do { \
	BUILD_ASSERT(!((sizeof(double) < VA_STACK_ALIGN(long double)) && \
			Z_CBPRINTF_IS_LONGDOUBLE(_arg) && \
			!IS_ENABLED(CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE)),\
			"Packaging of long double not enabled in Kconfig."); \
	while (_idx % Z_CBPRINTF_ALIGNMENT(_arg)) { \
		_idx += sizeof(int); \
	}; \
	uint32_t _arg_size = Z_CBPRINTF_ARG_SIZE(_arg); \
	if (_buf && _idx < _max) { \
		Z_CBPRINTF_STORE_ARG(&_buf[_idx], _arg); \
	} \
	_idx += _arg_size; \
} while (0)

/** @brief Package single argument.
 *
 * Macro is called in a loop for each argument in the string.
 *
 * @param arg argument.
 */
#define Z_CBPRINTF_PACK_ARG(arg) \
	Z_CBPRINTF_PACK_ARG2(_pbuf, _pkg_len, _pmax, arg)

/** @brief Package descriptor.
 *
 * @param len Package length.
 *
 * @param str_cnt Number of strings stored in the package.
 */
struct z_cbprintf_desc {
	uint8_t len;
	uint8_t str_cnt;
};

/** @brief Package header. */
union z_cbprintf_hdr {
	struct z_cbprintf_desc desc;
	void *raw;
};

/** @brief Statically package a formatted string with arguments.
 *
 * @param buf buffer. If null then only length is calculated.
 *
 * @param _inlen buffer capacity on input. Ignored when @p buf is null.
 *
 * @param _outlen number of bytes required to store the package.
 *
 * @param ... String with variable list of arguments.
 */
#define Z_CBPRINTF_STATIC_PACKAGE_GENERIC(buf, _inlen, _outlen, \
					  ... /* fmt, ... */) \
do { \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
	if (IS_ENABLED(CONFIG_CBPRINTF_STATIC_PACKAGE_CHECK_ALIGNMENT)) { \
		__ASSERT(!((uintptr_t)buf & (CBPRINTF_PACKAGE_ALIGNMENT - 1)), \
			"Buffer must be aligned."); \
	} \
	uint8_t *_pbuf = buf; \
	size_t _pmax = (buf != NULL) ? *_inlen : SIZE_MAX; \
	size_t _pkg_len = 0; \
	union z_cbprintf_hdr *_len_loc; \
	/* package starts with string address and field with length */ \
	if (_pmax < sizeof(char *) + 2 * sizeof(uint16_t)) { \
		break; \
	} \
	_len_loc = (union z_cbprintf_hdr *)_pbuf; \
	_pkg_len += sizeof(union z_cbprintf_hdr); \
	if (_pbuf) { \
		*(char **)&_pbuf[_pkg_len] = GET_ARG_N(1, __VA_ARGS__); \
	} \
	_pkg_len += sizeof(char *); \
	/* Pack remaining arguments */\
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), (), ( \
	    FOR_EACH(Z_CBPRINTF_PACK_ARG, (;), GET_ARGS_LESS_N(1, __VA_ARGS__));\
	)) \
	/* Store length */ \
	_outlen = (_pkg_len > _pmax) ? -ENOSPC : _pkg_len; \
	/* Store length in the header, set number of dumped strings to 0 */ \
	if (_pbuf) { \
		union z_cbprintf_hdr hdr = { .desc = {.len = _pkg_len }}; \
		*_len_loc = hdr; \
	} \
	_Pragma("GCC diagnostic pop") \
} while (0)

#if Z_C_GENERIC
#define Z_CBPRINTF_STATIC_PACKAGE(packaged, inlen, outlen, ... /* fmt, ... */) \
	Z_CBPRINTF_STATIC_PACKAGE_GENERIC(packaged, inlen, outlen, __VA_ARGS__)
#else
#define Z_CBPRINTF_STATIC_PACKAGE(packaged, inlen, outlen, ... /* fmt, ... */) \
do { \
	/* Small trick needed to avoid warning on always true */ \
	if (((uintptr_t)packaged + 1) != 1) { \
		outlen = cbprintf_package(packaged, inlen, __VA_ARGS__); \
	} else { \
		outlen = cbprintf_package(NULL, 0, __VA_ARGS__); \
	} \
} while (0)
#endif /* Z_C_GENERIC */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_INTERNAL_H_ */
