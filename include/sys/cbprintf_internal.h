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
#include <toolchain.h>
#include <sys/util.h>

#ifdef CONFIG_CBPRINTF_STATIC_PACKAGE_CHECK_ALIGNMENT
#include <sys/__assert.h>
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

static inline void z_cbprintf_wcpy(int *dst, int *src, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

#include <sys/cbprintf_cxx.h>

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
	z_cbprintf_cxx_is_pchar(x, flags & CBPRINTF_MUST_RUNTIME_PACKAGE_CONST_CHAR)
#else
#define Z_CBPRINTF_IS_PCHAR(x, flags) \
	_Generic((x) + 0, \
		char * : 1, \
		const char * : (flags & CBPRINTF_MUST_RUNTIME_PACKAGE_CONST_CHAR) ? 0 : 1, \
		volatile char * : 1, \
		const volatile char * : 1, \
		wchar_t * : 1, \
		const wchar_t * : (flags & CBPRINTF_MUST_RUNTIME_PACKAGE_CONST_CHAR) ? 0 : 1, \
		volatile wchar_t * : 1, \
		const volatile wchar_t * : 1, \
		default : \
			0)
#endif

/** @brief Calculate number of char * or wchar_t * arguments in the arguments.
 *
 * @param fmt string.
 *
 * @param ... string arguments.
 *
 * @return number of arguments which are char * or wchar_t *.
 */
#define Z_CBPRINTF_HAS_PCHAR_ARGS(flags, fmt, ...) \
	(FOR_EACH_FIXED_ARG(Z_CBPRINTF_IS_PCHAR, (+), flags, __VA_ARGS__))

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
#define Z_CBPRINTF_MUST_RUNTIME_PACKAGE(skip, flags, ...) ({\
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
	int _rv = COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
			(0), \
			(((Z_CBPRINTF_HAS_PCHAR_ARGS(flags, __VA_ARGS__) - skip) > 0))); \
	_Pragma("GCC diagnostic pop")\
	_rv; \
})
#else
#define Z_CBPRINTF_MUST_RUNTIME_PACKAGE(skip, flags, ...) 1
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
	__auto_type _v = (v) + 0; \
	size_t _arg_size = _Generic((v), \
		float : sizeof(double), \
		default : \
			sizeof((_v)) \
		); \
	_arg_size; \
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
} while (0)
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
#define Z_CBPRINTF_PACK_ARG2(_buf, _idx, _align_offset, _max, \
			     _cfg_flags, _s_idx, _s_buf, _arg) \
do { \
	BUILD_ASSERT(!((sizeof(double) < VA_STACK_ALIGN(long double)) && \
			Z_CBPRINTF_IS_LONGDOUBLE(_arg) && \
			!IS_ENABLED(CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE)),\
			"Packaging of long double not enabled in Kconfig."); \
	while (_align_offset % Z_CBPRINTF_ALIGNMENT(_arg)) { \
		_idx += sizeof(int); \
		_align_offset += sizeof(int); \
	} \
	uint32_t _arg_size = Z_CBPRINTF_ARG_SIZE(_arg); \
	if (Z_CBPRINTF_IS_PCHAR(_arg, 0)) { \
		_s_buf[_s_idx++] = _idx / sizeof(int); \
	} \
	if (_buf && _idx < (int)_max) { \
		Z_CBPRINTF_STORE_ARG(&_buf[_idx], _arg); \
	} \
	_idx += _arg_size; \
	_align_offset += _arg_size; \
} while (0)

/** @brief Package single argument.
 *
 * Macro is called in a loop for each argument in the string.
 *
 * @param arg argument.
 */
#define Z_CBPRINTF_PACK_ARG(arg) \
	Z_CBPRINTF_PACK_ARG2(_pbuf, _pkg_len, _pkg_offset, _pmax, _flags, \
			     _s_cnt, _s_buffer, arg)

/** @brief Package descriptor.
 *
 * @param len Package length.
 *
 * @param str_cnt Number of strings stored in the package.
 */
struct z_cbprintf_desc {
	uint8_t len;
	uint8_t str_cnt;
	uint8_t ro_str_cnt;
};

/** @brief Package header. */
union z_cbprintf_hdr {
	struct z_cbprintf_desc desc;
	void *raw;
};

/* When using clang additional warning needs to be suppressed since each
 * argument of fmt string is used for sizeof() which results in the warning
 * if argument is a stirng literal. Suppression is added here instead of
 * the macro which generates the warning to not slow down the compiler.
 */
#ifdef __clang__
#define Z_CBPRINTF_SUPPRESS_SIZEOF_ARRAY_DECAY \
	_Pragma("GCC diagnostic ignored \"-Wsizeof-array-decay\"")
#else
#define Z_CBPRINTF_SUPPRESS_SIZEOF_ARRAY_DECAY
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
 * @param _flags Option flags. See @ref CBPRINTF_PACKAGE_FLAGS.
 *
 * @param ... String with variable list of arguments.
 */
#define Z_CBPRINTF_STATIC_PACKAGE_GENERIC(buf, _inlen, _outlen, _align_offset, \
					  _flags, ... /* fmt, ... */) \
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
	bool str_idxs = _flags & CBPRINTF_PACKAGE_ADD_STRING_IDXS; \
	uint8_t *_pbuf = buf; \
	uint8_t _s_cnt = 0; \
	uint16_t _s_buffer[16]; \
	size_t _pmax = (buf != NULL) ? _inlen : INT32_MAX; \
	int _pkg_len = 0; \
	int _total_len = 0; \
	int _pkg_offset = _align_offset; \
	union z_cbprintf_hdr *_len_loc; \
	/* package starts with string address and field with length */ \
	if (_pmax < sizeof(union z_cbprintf_hdr)) { \
		_outlen = -ENOSPC; \
		break; \
	} \
	_len_loc = (union z_cbprintf_hdr *)_pbuf; \
	_pkg_len += sizeof(union z_cbprintf_hdr); \
	_pkg_offset += sizeof(union z_cbprintf_hdr); \
	/* Pack remaining arguments */\
	FOR_EACH(Z_CBPRINTF_PACK_ARG, (;), __VA_ARGS__);\
	_total_len = _pkg_len; \
	if (str_idxs) {\
		_total_len += _s_cnt; \
		if (_pbuf) { \
			for (int i = 0; i < _s_cnt; i++) { \
				_pbuf[_pkg_len + i] = _s_buffer[i]; \
			} \
		} \
	} \
	/* Store length */ \
	_outlen = (_total_len > (int)_pmax) ? -ENOSPC : _total_len; \
	/* Store length in the header, set number of dumped strings to 0 */ \
	if (_pbuf) { \
		union z_cbprintf_hdr hdr = { \
			.desc = { \
				.len = (uint8_t)(_pkg_len / sizeof(int)), \
				.str_cnt = 0, \
				.ro_str_cnt = str_idxs ? _s_cnt : (uint8_t)0, \
			} \
		}; \
		*_len_loc = hdr; \
	} \
	_Pragma("GCC diagnostic pop") \
} while (0)

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
} while (0)
#endif /* Z_C_GENERIC */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_SYS_CBPRINTF_INTERNAL_H_ */
