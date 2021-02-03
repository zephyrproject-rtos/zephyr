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

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Return 1 if argument is a pointer to char or wchar_t
 *
 * @param x argument.
 *
 * @return 1 if char * or wchar_t *, 0 otherwise.
 */
#define Z_CBPRINTF_IS_PCHAR(x) _Generic((x), \
			char *: 1, \
			const char *: 1, \
			volatile char *: 1, \
			const volatile char *: 1, \
			wchar_t *: 1, \
			const wchar_t *: 1, \
			volatile wchar_t *: 1, \
			const volatile wchar_t *: 1, \
			default: 0)


/** @brief Calculate number of char * or wchar_t * arguments in the arguments.
 *
 * @param fmt string.
 *
 * @param ... string arguments.
 *
 * @return number of arguments which are char * or wchar_t *.
 */
#define Z_CBPRINTF_HAS_PCHAR_ARGS_(fmt, ...) \
	(FAST_FOR_EACH(Z_CBPRINTF_IS_PCHAR, (+),  __VA_ARGS__))

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
#define Z_CBPRINTF_MUST_RUNTIME_PACKAGE(skip, ...) \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
			(0), \
			((Z_CBPRINTF_HAS_PCHAR_ARGS_(__VA_ARGS__) - skip) > 0 ?\
			 1 : 0))

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
	_Generic((v), void *:sizeof(void *), \
		      float: sizeof(double), \
		      char *: sizeof(char *) + 1, \
		      const char *: sizeof(char *) + 1, \
		      volatile const char *: sizeof(char *) + 1, \
		      volatile char *: sizeof(char *) + 1, \
		      default: _Generic((v), void *: 0, default: sizeof((v)+0)))

/** @brief Get storage size in words.
 *
 * @param x argument.
 *
 * @return number of words needed for storage.
 */
#define Z_CBPRINTF_ARG_WSIZE(x) (Z_CBPRINTF_ARG_SIZE(x) / sizeof(int))

/** @brief Macro for packaging single argument.
 *
 * Macro handles special cases:
 * - promotions of arguments smaller than int
 * - promotion of float
 * - char * packaging which is prefixed with null byte.
 * - special treatment of void * which would generate warning on arithmetic
 *   operation ((void *)ptr + 0).
 */
#define Z_CBPRINTF_PACK(__buf, x, _arg_wsize) do {\
	uint8_t *_buf = __buf; \
	_Generic((x)+0, \
		char *:({*_buf = 0; _buf++;}), \
		default: ({(void)_buf;})); \
	double _d = _Generic((x), \
			float: (x), \
			default: 0.0); \
	(void)_d; \
	int _i = _Generic((x), \
			short: (x), \
			default: 0); \
	(void)_i; \
	void *_v = _Generic((x), \
			void *: (x), \
			default: NULL); \
	(void)_v; \
	__auto_type _a = _Generic((x), \
				  void *: NULL, \
				  default: (x) + 0); \
	z_cbprintf_wcpy(_buf, \
			(void *)_Generic((x), \
					float: &_d, \
					short: &_i, \
					void *: &_v, \
					default: &_a), \
			_arg_wsize ); \
} while (0)

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
#define Z_CBPRINTF_PACK_ARG(_buf, _idx, _max, _arg) do { \
	uint32_t _arg_size = Z_CBPRINTF_ARG_SIZE(_arg); \
	uint32_t _arg_wsize = _arg_size / sizeof(uint32_t); \
	if (_buf && _idx < _max) { \
		Z_CBPRINTF_PACK(&_buf[_idx], _arg, _arg_wsize); \
	} \
	_idx += _arg_size; \
} while (0)

/** @brief Package single argument.
 *
 * Macro is called in a loop for each argument in the string.
 *
 * @param arg argument.
 */
#define Z_CBPRINTF_LOOP_PACK_ARG(arg) do { \
	Z_CBPRINTF_PACK_ARG(__package_buf, __package_len, __package_max, arg);\
} while (0)

/** @brief Statically package a formatted string with arguments.
 *
 * @param buf buffer. If null then only length is calculated.
 *
 * @param len buffer capacity on input, package size on output.
 *
 * @param fmt_as_ptr Flag indicating that format string is stored as void
 * pointer.
 *
 * @param ... String with variable list of arguments.
 */
#define Z_CBPRINTF_STATIC_PACKAGE(buf, len, fmt_as_ptr, ... /* fmt, ... */) \
do { \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
	uint8_t *__package_buf = buf; \
	size_t __package_max = (buf != NULL) ? len : SIZE_MAX; \
	size_t __package_len = 0; \
	FAST_FOR_EACH(Z_CBPRINTF_LOOP_PACK_ARG, (;), \
			IF_ENABLED(fmt_as_ptr, ((uint8_t *)))__VA_ARGS__); \
	len = __package_len; \
	_Pragma("GCC diagnostic pop") \
} while (0)

#define Z_CBPRINTF_FMT_SIZE(fmt_as_ptr) (sizeof(void *) + (fmt_as_ptr ? 0 : 1))

/** @brief Calculate package size. 0 is retuned if only null pointer is given.*/
#define Z_CBPRINTF_STATIC_PACKAGE_SIZE(_name, fmt_as_ptr, ...) \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
	static const size_t _name = \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
		(FAST_GET_ARG_N(1, __VA_ARGS__) == NULL ? \
			0 : Z_CBPRINTF_FMT_SIZE(fmt_as_ptr)), \
		(Z_CBPRINTF_FMT_SIZE(fmt_as_ptr) + \
		 FAST_FOR_EACH(Z_CBPRINTF_ARG_SIZE, \
				(+), FAST_GET_ARGS_LESS_N(1,__VA_ARGS__)))); \
	_Pragma("GCC diagnostic pop")

__attribute__((always_inline))
static inline void z_cbprintf_wcpy(void *dst, void *src, uint32_t wlen)
{
	uint32_t *dst32 = dst;
	uint32_t *src32 = src;

	for (uint32_t i = 0; i < wlen; i++) {
		dst32[i] = src32[i];
	}
}

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_INTERNAL_H_ */
