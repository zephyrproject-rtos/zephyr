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
		char * : 1, \
		const char * : ((flags) & CBPRINTF_PACKAGE_CONST_CHAR_RO) ? 0 : 1, \
		volatile char * : 1, \
		const volatile char * : 1, \
		wchar_t * : 1, \
		const wchar_t * : ((flags) & CBPRINTF_PACKAGE_CONST_CHAR_RO) ? 0 : 1, \
		volatile wchar_t * : 1, \
		const volatile wchar_t * : 1, \
		default : \
			0)
#endif

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
	__auto_type _v = (v) + 0; \
	/* Static code analysis may complain about unused variable. */ \
	(void)_v; \
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
#define Z_CBPRINTF_PACK_ARG2(arg_idx, _buf, _idx, _align_offset, _max, _arg) \
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
	uint32_t _loc = _idx / sizeof(int); \
	if (arg_idx < 1 + _fros_cnt) { \
		if (_ros_pos_en) { \
			_ros_pos_buf[_ros_pos_idx++] = _loc; \
		} \
	} else if (Z_CBPRINTF_IS_PCHAR(_arg, 0)) { \
		if (_cros_en) { \
			if (Z_CBPRINTF_IS_X_PCHAR(arg_idx, _arg, _flags)) { \
				if (_rws_pos_en) { \
					_rws_buffer[_rws_pos_idx++] = _loc; \
				} \
			} else { \
				if (_ros_pos_en) { \
					_ros_pos_buf[_ros_pos_idx++] = _loc; \
				} \
			} \
		} else if (_rws_pos_en) { \
			_rws_buffer[_rws_pos_idx++] = _idx / sizeof(int); \
		} \
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
	Z_CBPRINTF_ON_STACK_ALLOC(_rws_buffer, _rws_cnt); \
	size_t _pmax = (buf != NULL) ? _inlen : INT32_MAX; \
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
	_total_len += _rws_cnt; \
	if (_pbuf) { \
		/* Append string locations. */ \
		uint8_t *_pbuf_loc = &_pbuf[_pkg_len]; \
		for (size_t i = 0; i < _ros_cnt; i++) { \
			*_pbuf_loc++ = _ros_pos_buf[i]; \
		} \
		for (size_t i = 0; i < _rws_cnt; i++) { \
			*_pbuf_loc++ = _rws_buffer[i]; \
		} \
	} \
	/* Store length */ \
	_outlen = (_total_len > (int)_pmax) ? -ENOSPC : _total_len; \
	/* Store length in the header, set number of dumped strings to 0 */ \
	if (_pbuf) { \
		union cbprintf_package_hdr hdr = { \
			.desc = { \
				.len = (uint8_t)(_pkg_len / sizeof(int)), \
				.str_cnt = 0, \
				.ro_str_cnt = _ros_cnt, \
				.rw_str_cnt = _rws_cnt, \
			} \
		}; \
		IF_ENABLED(CONFIG_CBPRINTF_PACKAGE_HEADER_STORE_CREATION_FLAGS, \
			   (hdr.desc.pkg_flags = flags)); \
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
