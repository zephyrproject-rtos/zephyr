/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_

#include <zephyr/logging/log_instance.h>
#include <zephyr/sys/mpsc_packet.h>
#include <zephyr/sys/cbprintf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/toolchain.h>

#ifdef __GNUC__
#ifndef alloca
#define alloca __builtin_alloca
#endif
#else
#include <alloca.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_MSG_DEBUG 0
#define LOG_MSG_DBG(...) IF_ENABLED(LOG_MSG_DEBUG, (printk(__VA_ARGS__)))

#ifdef CONFIG_LOG_TIMESTAMP_64BIT
typedef uint64_t log_timestamp_t;
#else
typedef uint32_t log_timestamp_t;
#endif

/**
 * @brief Log message API
 * @defgroup log_msg Log message API
 * @ingroup logger
 * @{
 */

#define Z_LOG_MSG_LOG 0

#define Z_LOG_MSG_PACKAGE_BITS 11

#define Z_LOG_MSG_MAX_PACKAGE BIT_MASK(Z_LOG_MSG_PACKAGE_BITS)

#define LOG_MSG_GENERIC_HDR \
	MPSC_PBUF_HDR;\
	uint32_t type:1

struct log_msg_desc {
	LOG_MSG_GENERIC_HDR;
	uint32_t domain:3;
	uint32_t level:3;
	uint32_t package_len:Z_LOG_MSG_PACKAGE_BITS;
	uint32_t data_len:12;
};

union log_msg_source {
	const struct log_source_const_data *fixed;
	struct log_source_dynamic_data *dynamic;
	void *raw;
};

struct log_msg_hdr {
	struct log_msg_desc desc;
/* Attempting to keep best alignment. When address is 64 bit and timestamp 32
 * swap the order to have 16 byte header instead of 24 byte.
 */
#if (INTPTR_MAX > INT32_MAX) && !defined(CONFIG_LOG_TIMESTAMP_64BIT)
	log_timestamp_t timestamp;
	const void *source;
#else
	const void *source;
	log_timestamp_t timestamp;
#endif
#if defined(CONFIG_LOG_THREAD_ID_PREFIX)
	void *tid;
#endif
};
/* Messages are aligned to alignment required by cbprintf package. */
#define Z_LOG_MSG_ALIGNMENT CBPRINTF_PACKAGE_ALIGNMENT

#define Z_LOG_MSG_PADDING \
	((sizeof(struct log_msg_hdr) % Z_LOG_MSG_ALIGNMENT) > 0 ? \
	(Z_LOG_MSG_ALIGNMENT - (sizeof(struct log_msg_hdr) % Z_LOG_MSG_ALIGNMENT)) : \
		0)

struct log_msg {
	struct log_msg_hdr hdr;
	/* Adding padding to ensure that cbprintf package that follows is
	 * properly aligned.
	 */
	uint8_t padding[Z_LOG_MSG_PADDING];
	uint8_t data[];
};

/**
 * @cond INTERNAL_HIDDEN
 */
BUILD_ASSERT(sizeof(struct log_msg) % Z_LOG_MSG_ALIGNMENT == 0,
	     "Log msg size must aligned");
/**
 * @endcond
 */


struct log_msg_generic_hdr {
	LOG_MSG_GENERIC_HDR;
};

union log_msg_generic {
	union mpsc_pbuf_generic buf;
	struct log_msg_generic_hdr generic;
	struct log_msg log;
};

/** @brief Method used for creating a log message.
 *
 * It is used for testing purposes to validate that expected mode was used.
 */
enum z_log_msg_mode {
	/* Runtime mode is least efficient but supports all cases thus it is
	 * treated as a fallback method when others cannot be used.
	 */
	Z_LOG_MSG_MODE_RUNTIME,
	/* Mode creates statically a string package on stack and calls a
	 * function for creating a message. It takes code size than
	 * Z_LOG_MSG_MODE_ZERO_COPY but is a bit slower.
	 */
	Z_LOG_MSG_MODE_FROM_STACK,

	/* Mode calculates size of the message and allocates it and writes
	 * directly to the message space. It is the fastest method but requires
	 * more code size.
	 */
	Z_LOG_MSG_MODE_ZERO_COPY,

	/* Mode optimized for simple messages with 0 to 2 32 bit word arguments.*/
	Z_LOG_MSG_MODE_SIMPLE,
};

#define Z_LOG_MSG_DESC_INITIALIZER(_domain_id, _level, _plen, _dlen) \
{ \
	.valid = 0, \
	.busy = 0, \
	.type = Z_LOG_MSG_LOG, \
	.domain = (_domain_id), \
	.level = (_level), \
	.package_len = (_plen), \
	.data_len = (_dlen), \
}

#define Z_LOG_MSG_CBPRINTF_FLAGS(_cstr_cnt) \
	(CBPRINTF_PACKAGE_FIRST_RO_STR_CNT(_cstr_cnt) | \
	(IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? \
	 CBPRINTF_PACKAGE_ADD_STRING_IDXS : 0))

#ifdef CONFIG_LOG_USE_VLA
#define Z_LOG_MSG_ON_STACK_ALLOC(ptr, len) \
	long long _ll_buf[DIV_ROUND_UP(len, sizeof(long long))]; \
	long double _ld_buf[DIV_ROUND_UP(len, sizeof(long double))]; \
	(ptr) = (sizeof(long double) == Z_LOG_MSG_ALIGNMENT) ? \
			(struct log_msg *)_ld_buf : (struct log_msg *)_ll_buf; \
	if (IS_ENABLED(CONFIG_LOG_TEST_CLEAR_MESSAGE_SPACE)) { \
		/* During test fill with 0's to simplify message comparison */ \
		memset((ptr), 0, (len)); \
	}
#else /* Z_LOG_MSG_USE_VLA */
/* When VLA cannot be used we need to trick compiler a bit and create multiple
 * fixed size arrays and take the smallest one that will fit the message.
 * Compiler will remove unused arrays and stack usage will be kept similar
 * to vla case, rounded to the size of the used buffer.
 */
#define Z_LOG_MSG_ON_STACK_ALLOC(ptr, len) \
	long long _ll_buf32[32 / sizeof(long long)]; \
	long long _ll_buf48[48 / sizeof(long long)]; \
	long long _ll_buf64[64 / sizeof(long long)]; \
	long long _ll_buf128[128 / sizeof(long long)]; \
	long long _ll_buf256[256 / sizeof(long long)]; \
	long double _ld_buf32[32 / sizeof(long double)]; \
	long double _ld_buf48[48 / sizeof(long double)]; \
	long double _ld_buf64[64 / sizeof(long double)]; \
	long double _ld_buf128[128 / sizeof(long double)]; \
	long double _ld_buf256[256 / sizeof(long double)]; \
	if (sizeof(long double) == Z_LOG_MSG_ALIGNMENT) { \
		ptr = (len > 128) ? (struct log_msg *)_ld_buf256 : \
			((len > 64) ? (struct log_msg *)_ld_buf128 : \
			((len > 48) ? (struct log_msg *)_ld_buf64 : \
			((len > 32) ? (struct log_msg *)_ld_buf48 : \
				      (struct log_msg *)_ld_buf32)));\
	} else { \
		ptr = (len > 128) ? (struct log_msg *)_ll_buf256 : \
			((len > 64) ? (struct log_msg *)_ll_buf128 : \
			((len > 48) ? (struct log_msg *)_ll_buf64 : \
			((len > 32) ? (struct log_msg *)_ll_buf48 : \
				      (struct log_msg *)_ll_buf32)));\
	} \
	if (IS_ENABLED(CONFIG_LOG_TEST_CLEAR_MESSAGE_SPACE)) { \
		/* During test fill with 0's to simplify message comparison */ \
		memset((ptr), 0, (len)); \
	}
#endif /* Z_LOG_MSG_USE_VLA */

#define Z_LOG_MSG_ALIGN_OFFSET \
	offsetof(struct log_msg, data)

#define Z_LOG_MSG_LEN(pkg_len, data_len) \
	(offsetof(struct log_msg, data) + (pkg_len) + (data_len))

#define Z_LOG_MSG_ALIGNED_WLEN(pkg_len, data_len) \
	DIV_ROUND_UP(ROUND_UP(Z_LOG_MSG_LEN(pkg_len, data_len), \
				  Z_LOG_MSG_ALIGNMENT), \
			 sizeof(uint32_t))

/*
 * With Zephyr SDK 0.14.2, aarch64-zephyr-elf-gcc (10.3.0) fails to ensure $sp
 * is below the active memory during message construction. As a result,
 * interrupts happening in the middle of that process can end up smashing active
 * data and causing a logging fault. Work around this by inserting a compiler
 * barrier after the allocation and before any use to make sure GCC moves the
 * stack pointer soon enough
 */

#define Z_LOG_ARM64_VLA_PROTECT() compiler_barrier()

#define _LOG_MSG_SIMPLE_XXXX0 1
#define _LOG_MSG_SIMPLE_XXXX1 1
#define _LOG_MSG_SIMPLE_XXXX2 1

/* Determine if amount of arguments (less than 3) qualifies to  simple message. */
#define LOG_MSG_SIMPLE_ARG_CNT_CHECK(...) \
	COND_CODE_1(UTIL_CAT(_LOG_MSG_SIMPLE_XXXX, NUM_VA_ARGS_LESS_1(__VA_ARGS__)), (1), (0))

/* Set of marcos used to determine if arguments type allows simplified message creation mode. */
#define LOG_MSG_SIMPLE_ARG_TYPE_CHECK_0(fmt) 1
#define LOG_MSG_SIMPLE_ARG_TYPE_CHECK_1(fmt, arg) Z_CBPRINTF_IS_WORD_NUM(arg)
#define LOG_MSG_SIMPLE_ARG_TYPE_CHECK_2(fmt, arg0, arg1) \
	Z_CBPRINTF_IS_WORD_NUM(arg0) && Z_CBPRINTF_IS_WORD_NUM(arg1)

/** brief Determine if string arguments types allow to use simplified message creation mode.
 *
 * @param ... String with arguments.
 */
#define LOG_MSG_SIMPLE_ARG_TYPE_CHECK(...) \
	UTIL_CAT(LOG_MSG_SIMPLE_ARG_TYPE_CHECK_, NUM_VA_ARGS_LESS_1(__VA_ARGS__))(__VA_ARGS__)

/** @brief Check if message can be handled using simplified method.
 *
 * Following conditions must be met:
 * - 32 bit platform
 * - Number of arguments from 0 to 2
 * - Type of an argument must be a numeric value that fits in 32 bit word.
 *
 * @param ... String with arguments.
 *
 * @retval 1 if message qualifies.
 * @retval 0 if message does not qualify.
 */
#define LOG_MSG_SIMPLE_CHECK(...) \
	COND_CODE_1(CONFIG_64BIT, (0), (\
		COND_CODE_1(LOG_MSG_SIMPLE_ARG_CNT_CHECK(__VA_ARGS__), ( \
				LOG_MSG_SIMPLE_ARG_TYPE_CHECK(__VA_ARGS__)), (0))))

/* Helper macro for handing log with one argument. Macro casts the first argument to uint32_t. */
#define Z_LOG_MSG_SIMPLE_CREATE_1(_source, _level, ...) \
	z_log_msg_simple_create_1(_source, _level, GET_ARG_N(1, __VA_ARGS__), \
			(uint32_t)(uintptr_t)GET_ARG_N(2, __VA_ARGS__))

/* Helper macro for handing log with two arguments. Macro casts arguments to uint32_t.
 */
#define Z_LOG_MSG_SIMPLE_CREATE_2(_source, _level, ...) \
	z_log_msg_simple_create_2(_source, _level, GET_ARG_N(1, __VA_ARGS__), \
			(uint32_t)(uintptr_t)GET_ARG_N(2, __VA_ARGS__), \
			(uint32_t)(uintptr_t)GET_ARG_N(3, __VA_ARGS__))

/* Call specific function based on the number of arguments.
 * Since up 2 to arguments are supported COND_CODE_0 and COND_CODE_1 can be used to
 * handle all cases (0, 1 and 2 arguments). When tracing is enable then for each
 * function a macro is create. The difference between function and macro is that
 * macro is applied to any input arguments so we need to make sure that it is
 * always called with proper number of arguments. For that it is wrapped around
 * into another macro and dummy arguments to cover for cases when there is less
 * arguments in a log call.
 */
#define Z_LOG_MSG_SIMPLE_FUNC2(arg_cnt, _source, _level, ...) \
	COND_CODE_0(arg_cnt, \
			(z_log_msg_simple_create_0(_source, _level, GET_ARG_N(1, __VA_ARGS__))), \
			(COND_CODE_1(arg_cnt, ( \
			    Z_LOG_MSG_SIMPLE_CREATE_1(_source, _level, __VA_ARGS__, dummy) \
			    ), ( \
			    Z_LOG_MSG_SIMPLE_CREATE_2(_source, _level, __VA_ARGS__, dummy, dummy) \
			    ) \
			)))

/** @brief Call specific function to create a log message.
 *
 * Macro picks matching function (based on number of arguments) and calls it.
 * String arguments are casted to uint32_t.
 *
 * @param _source	Source.
 * @param _level	Severity level.
 * @param ...		String with arguments.
 */
#define LOG_MSG_SIMPLE_FUNC(_source, _level, ...) \
	Z_LOG_MSG_SIMPLE_FUNC2(NUM_VA_ARGS_LESS_1(__VA_ARGS__), _source, _level, __VA_ARGS__)

/** @brief Create log message using simplified method.
 *
 * Macro is gated by the argument count check to run @ref LOG_MSG_SIMPLE_FUNC only
 * on entries with 2 or less arguments.
 *
 * @param _domain_id	Domain ID.
 * @param _source	Pointer to the source structure.
 * @param _level	Severity level.
 * @param ...		String with arguments.
 */
#define Z_LOG_MSG_SIMPLE_ARGS_CREATE(_domain_id, _source, _level, ...) \
	IF_ENABLED(LOG_MSG_SIMPLE_ARG_CNT_CHECK(__VA_ARGS__), (\
		LOG_MSG_SIMPLE_FUNC(_source, _level, __VA_ARGS__); \
	))

#define Z_LOG_MSG_STACK_CREATE(_cstr_cnt, _domain_id, _source, _level, _data, _dlen, ...) \
do { \
	int _plen; \
	uint32_t _options = Z_LOG_MSG_CBPRINTF_FLAGS(_cstr_cnt) | \
			  CBPRINTF_PACKAGE_ADD_RW_STR_POS; \
	if (GET_ARG_N(1, __VA_ARGS__) == NULL) { \
		_plen = 0; \
	} else { \
		CBPRINTF_STATIC_PACKAGE(NULL, 0, _plen, Z_LOG_MSG_ALIGN_OFFSET, _options, \
					__VA_ARGS__); \
	} \
	TOOLCHAIN_IGNORE_WSHADOW_BEGIN \
	struct log_msg *_msg; \
	TOOLCHAIN_IGNORE_WSHADOW_END \
	Z_LOG_MSG_ON_STACK_ALLOC(_msg, Z_LOG_MSG_LEN(_plen, 0)); \
	Z_LOG_ARM64_VLA_PROTECT(); \
	if (_plen != 0) { \
		CBPRINTF_STATIC_PACKAGE(_msg->data, _plen, \
					_plen, Z_LOG_MSG_ALIGN_OFFSET, _options, \
					__VA_ARGS__);\
	} \
	struct log_msg_desc _desc = \
		Z_LOG_MSG_DESC_INITIALIZER(_domain_id, _level, \
					   (uint32_t)_plen, _dlen); \
	LOG_MSG_DBG("creating message on stack: package len: %d, data len: %d\n", \
			_plen, (int)(_dlen)); \
	z_log_msg_static_create((void *)(_source), _desc, _msg->data, (_data)); \
} while (false)

#ifdef CONFIG_LOG_SPEED
#define Z_LOG_MSG_SIMPLE_CREATE(_cstr_cnt, _domain_id, _source, _level, ...) do { \
	int _plen; \
	CBPRINTF_STATIC_PACKAGE(NULL, 0, _plen, Z_LOG_MSG_ALIGN_OFFSET, \
				Z_LOG_MSG_CBPRINTF_FLAGS(_cstr_cnt), \
				__VA_ARGS__); \
	size_t _msg_wlen = Z_LOG_MSG_ALIGNED_WLEN(_plen, 0); \
	struct log_msg *_msg = z_log_msg_alloc(_msg_wlen); \
	struct log_msg_desc _desc = \
		Z_LOG_MSG_DESC_INITIALIZER(_domain_id, _level, (uint32_t)_plen, 0); \
	LOG_MSG_DBG("creating message zero copy: package len: %d, msg: %p\n", \
			_plen, _msg); \
	if (_msg) { \
		CBPRINTF_STATIC_PACKAGE(_msg->data, _plen, _plen, \
					Z_LOG_MSG_ALIGN_OFFSET, \
					Z_LOG_MSG_CBPRINTF_FLAGS(_cstr_cnt), \
					__VA_ARGS__); \
	} \
	z_log_msg_finalize(_msg, (void *)_source, _desc, NULL); \
} while (false)
#else
/* Alternative empty macro created to speed up compilation when LOG_SPEED is
 * disabled (default).
 */
#define Z_LOG_MSG_SIMPLE_CREATE(...)
#endif

/* Macro handles case when local variable with log message string is created. It
 * replaces original string literal with that variable.
 */
#define Z_LOG_FMT_ARGS_2(_name, ...) \
	COND_CODE_1(CONFIG_LOG_FMT_SECTION, \
		(COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
		   (_name), (_name, GET_ARGS_LESS_N(1, __VA_ARGS__)))), \
		(__VA_ARGS__))

/** @brief Wrapper for log message string with arguments.
 *
 * Wrapper is replacing first argument with a variable from a dedicated memory
 * section if option is enabled. Macro handles the case when there is no
 * log message provided.
 *
 * @param _name Name of the variable with log message string. It is optionally used.
 * @param ... Optional log message with arguments (may be empty).
 */
#define Z_LOG_FMT_ARGS(_name, ...) \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(_, ##__VA_ARGS__), \
		(NULL), \
		(Z_LOG_FMT_ARGS_2(_name, ##__VA_ARGS__)))

#if defined(CONFIG_LOG_USE_TAGGED_ARGUMENTS)

#define Z_LOG_FMT_TAGGED_ARGS_2(_name, ...) \
	COND_CODE_1(CONFIG_LOG_FMT_SECTION, \
		    (_name, Z_CBPRINTF_TAGGED_ARGS(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
						   GET_ARGS_LESS_N(1, __VA_ARGS__))), \
		    (GET_ARG_N(1, __VA_ARGS__), \
		     Z_CBPRINTF_TAGGED_ARGS(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
					    GET_ARGS_LESS_N(1, __VA_ARGS__))))

/** @brief Wrapper for log message string with tagged arguments.
 *
 * Wrapper is replacing first argument with a variable from a dedicated memory
 * section if option is enabled. Macro handles the case when there is no
 * log message provided. Each subsequent arguments are tagged by preceding
 * each argument with its type value.
 *
 * @param _name Name of the variable with log message string. It is optionally used.
 * @param ... Optional log message with arguments (may be empty).
 */
#define Z_LOG_FMT_TAGGED_ARGS(_name, ...) \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(_, ##__VA_ARGS__), \
		(Z_CBPRINTF_TAGGED_ARGS(0)), \
		(Z_LOG_FMT_TAGGED_ARGS_2(_name, ##__VA_ARGS__)))

#define Z_LOG_FMT_RUNTIME_ARGS(...) \
	Z_LOG_FMT_TAGGED_ARGS(__VA_ARGS__)

#else

#define Z_LOG_FMT_RUNTIME_ARGS(...) \
	Z_LOG_FMT_ARGS(__VA_ARGS__)

#endif /* CONFIG_LOG_USE_TAGGED_ARGUMENTS */

/* Macro handles case when there is no string provided, in that case variable
 * is not created.
 */
#define Z_LOG_MSG_STR_VAR_IN_SECTION(_name, ...) \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(_, ##__VA_ARGS__), \
		    (/* No args provided, no variable */), \
		    (static const char _name[] \
		     __in_section(_log_strings, static, _CONCAT(_name, _)) __used __noasan = \
			GET_ARG_N(1, __VA_ARGS__);))

/** @brief Create variable in the dedicated memory section (if enabled).
 *
 * Variable is initialized with a format string from the log message.
 *
 * @param _name Variable name.
 * @param ... Optional log message with arguments (may be empty).
 */
#define Z_LOG_MSG_STR_VAR(_name, ...) \
	IF_ENABLED(CONFIG_LOG_FMT_SECTION, \
		   (Z_LOG_MSG_STR_VAR_IN_SECTION(_name, ##__VA_ARGS__)))

/** @brief Create log message and write it into the logger buffer.
 *
 * Macro handles creation of log message which includes storing log message
 * description, timestamp, arguments, copying string arguments into message and
 * copying user data into the message space. There are 3 modes of message
 * creation:
 * - at compile time message size is determined, message is allocated and
 *   content is written directly to the message. It is the fastest but cannot
 *   be used in user mode. Message size cannot be determined at compile time if
 *   it contains data or string arguments which are string pointers.
 * - at compile time message size is determined, string package is created on
 *   stack, message is created in function call. String package can only be
 *   created on stack if it does not contain unexpected pointers to strings.
 * - string package is created at runtime. This mode has no limitations but
 *   it is significantly slower.
 *
 * @param _try_0cpy If positive then, if possible, message content is written
 * directly to message. If 0 then, if possible, string package is created on
 * the stack and message is created in the function call.
 *
 * @param _mode Used for testing. It is set according to message creation mode
 *		used.
 *
 * @param _cstr_cnt Number of constant strings present in the string. It is
 * used to help detect messages which must be runtime processed, compared to
 * message which can be prebuilt at compile time.
 *
 * @param _domain_id Domain ID.
 *
 * @param _source Pointer to the constant descriptor of the log message source.
 *
 * @param _level Log message level.
 *
 * @param _data Pointer to the data. Can be null.
 *
 * @param _dlen Number of data bytes. 0 if data is not provided.
 *
 * @param ...  Optional string with arguments (fmt, ...). It may be empty.
 */
#if defined(CONFIG_LOG_ALWAYS_RUNTIME) || !defined(CONFIG_LOG)
#define Z_LOG_MSG_CREATE2(_try_0cpy, _mode,  _cstr_cnt, _domain_id, _source,\
			  _level, _data, _dlen, ...) \
do {\
	Z_LOG_MSG_STR_VAR(_fmt, ##__VA_ARGS__) \
	z_log_msg_runtime_create((_domain_id), (void *)(_source), \
				  (_level), (uint8_t *)(_data), (_dlen),\
				  Z_LOG_MSG_CBPRINTF_FLAGS(_cstr_cnt) | \
				  (IS_ENABLED(CONFIG_LOG_USE_TAGGED_ARGUMENTS) ? \
				   CBPRINTF_PACKAGE_ARGS_ARE_TAGGED : 0), \
				  Z_LOG_FMT_RUNTIME_ARGS(_fmt, ##__VA_ARGS__));\
	(_mode) = Z_LOG_MSG_MODE_RUNTIME; \
} while (false)
#else /* CONFIG_LOG_ALWAYS_RUNTIME || !CONFIG_LOG */
#define Z_LOG_MSG_CREATE3(_try_0cpy, _mode,  _cstr_cnt, _domain_id, _source,\
			  _level, _data, _dlen, ...) \
do { \
	Z_LOG_MSG_STR_VAR(_fmt, ##__VA_ARGS__); \
	bool has_rw_str = CBPRINTF_MUST_RUNTIME_PACKAGE( \
					Z_LOG_MSG_CBPRINTF_FLAGS(_cstr_cnt), \
					__VA_ARGS__); \
	if (IS_ENABLED(CONFIG_LOG_SPEED) && (_try_0cpy) && ((_dlen) == 0) && !has_rw_str) {\
		LOG_MSG_DBG("create zero-copy message\n");\
		Z_LOG_MSG_SIMPLE_CREATE(_cstr_cnt, _domain_id, _source, \
					_level, Z_LOG_FMT_ARGS(_fmt, ##__VA_ARGS__)); \
		(_mode) = Z_LOG_MSG_MODE_ZERO_COPY; \
	} else { \
		IF_ENABLED(UTIL_AND(IS_ENABLED(CONFIG_LOG_SIMPLE_MSG_OPTIMIZE), \
				    UTIL_AND(UTIL_NOT(_domain_id), UTIL_NOT(_cstr_cnt))), \
			( \
			bool can_simple = LOG_MSG_SIMPLE_CHECK(__VA_ARGS__); \
			if (can_simple && ((_dlen) == 0) && !k_is_user_context()) { \
				LOG_MSG_DBG("create fast message\n");\
				Z_LOG_MSG_SIMPLE_ARGS_CREATE(_domain_id, _source, _level, \
						     Z_LOG_FMT_ARGS(_fmt, ##__VA_ARGS__)); \
				_mode = Z_LOG_MSG_MODE_SIMPLE; \
				break; \
			} \
			) \
		) \
		LOG_MSG_DBG("create on stack message\n");\
		Z_LOG_MSG_STACK_CREATE(_cstr_cnt, _domain_id, _source, _level, _data, \
					_dlen, Z_LOG_FMT_ARGS(_fmt, ##__VA_ARGS__)); \
		(_mode) = Z_LOG_MSG_MODE_FROM_STACK; \
	} \
	(void)(_mode); \
} while (false)

#if defined(__cplusplus)
#define Z_AUTO_TYPE auto
#else
#define Z_AUTO_TYPE __auto_type
#endif

/* Macro for getting name of a local variable with the exception of the first argument
 * which is a formatted string in log message.
 */
#define Z_LOG_LOCAL_ARG_NAME(idx, arg) COND_CODE_0(idx, (arg), (_v##idx))

/* Create local variable from input variable (expect for the first (fmt) argument). */
#define Z_LOG_LOCAL_ARG_CREATE(idx, arg) \
	COND_CODE_0(idx, (), (Z_AUTO_TYPE Z_LOG_LOCAL_ARG_NAME(idx, arg) = (arg) + 0))

/* First level of processing creates stack variables to be passed for further processing.
 * This is done to prevent multiple evaluations of input arguments (in case argument
 * evaluation has side effects, e.g. it is a non-pure function call).
 */
#define Z_LOG_MSG_CREATE2(_try_0cpy, _mode, _cstr_cnt,  _domain_id, _source, \
			   _level, _data, _dlen, ...) \
do { \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
	FOR_EACH_IDX(Z_LOG_LOCAL_ARG_CREATE, (;), __VA_ARGS__); \
	_Pragma("GCC diagnostic pop") \
	Z_LOG_MSG_CREATE3(_try_0cpy, _mode,  _cstr_cnt, _domain_id, _source,\
			   _level, _data, _dlen, \
			   FOR_EACH_IDX(Z_LOG_LOCAL_ARG_NAME, (,), __VA_ARGS__)); \
} while (false)
#endif /* CONFIG_LOG_ALWAYS_RUNTIME || !CONFIG_LOG */


#define Z_LOG_MSG_CREATE(_try_0cpy, _mode,  _domain_id, _source,\
			  _level, _data, _dlen, ...) \
	Z_LOG_MSG_CREATE2(_try_0cpy, _mode, UTIL_CAT(Z_LOG_FUNC_PREFIX_, _level), \
			   _domain_id, _source, _level, _data, _dlen, \
			   Z_LOG_STR(_level, __VA_ARGS__))

/** @brief Allocate log message.
 *
 * @param wlen Length in 32 bit words.
 *
 * @return allocated space or null if cannot be allocated.
 */
struct log_msg *z_log_msg_alloc(uint32_t wlen);

/** @brief Finalize message.
 *
 * Finalization includes setting source, copying data and timestamp in the
 * message followed by committing the message.
 *
 * @param msg Message.
 *
 * @param source Address of the source descriptor.
 *
 * @param desc Message descriptor.
 *
 * @param data Data.
 */
void z_log_msg_finalize(struct log_msg *msg, const void *source,
			 const struct log_msg_desc desc, const void *data);

/** @brief Create log message using simplified method for string with no arguments.
 *
 * @param source Pointer to the source structure.
 * @param level  Severity level.
 * @param fmt    String pointer.
 */
__syscall void z_log_msg_simple_create_0(const void *source, uint32_t level,
					 const char *fmt);

/** @brief Create log message using simplified method for string with a one argument.
 *
 * @param source Pointer to the source structure.
 * @param level  Severity level.
 * @param fmt    String pointer.
 * @param arg    String argument.
 */
__syscall void z_log_msg_simple_create_1(const void *source, uint32_t level,
					 const char *fmt, uint32_t arg);

/** @brief Create log message using simplified method for string with two arguments.
 *
 * @param source Pointer to the source structure.
 * @param level  Severity level.
 * @param fmt    String pointer.
 * @param arg0   String argument.
 * @param arg1   String argument.
 */
__syscall void z_log_msg_simple_create_2(const void *source, uint32_t level,
					 const char *fmt, uint32_t arg0, uint32_t arg1);

/** @brief Create a logging message from message details and string package.
 *
 * @param source Source.
 *
 * @param desc Message descriptor.
 *
 * @param package Package.
 *
 * @param data Data.
 */
__syscall void z_log_msg_static_create(const void *source,
					const struct log_msg_desc desc,
					uint8_t *package, const void *data);

/** @brief Create message at runtime.
 *
 * Function allows to build any log message based on input data. Processing
 * time is significantly higher than statically message creating.
 *
 * @param domain_id Domain ID.
 *
 * @param source Source.
 *
 * @param level Log level.
 *
 * @param data Data.
 *
 * @param dlen Data length.
 *
 * @param package_flags Package flags.
 *
 * @param fmt String.
 *
 * @param ap Variable list of string arguments.
 */
void z_log_msg_runtime_vcreate(uint8_t domain_id, const void *source,
				uint8_t level, const void *data,
				size_t dlen, uint32_t package_flags,
				const char *fmt,
				va_list ap);

/** @brief Create message at runtime.
 *
 * Function allows to build any log message based on input data. Processing
 * time is significantly higher than statically message creating.
 *
 * @param domain_id Domain ID.
 *
 * @param source Source.
 *
 * @param level Log level.
 *
 * @param data Data.
 *
 * @param dlen Data length.
 *
 * @param package_flags Package flags.
 *
 * @param fmt String.
 *
 * @param ... String arguments.
 */
static inline void z_log_msg_runtime_create(uint8_t domain_id,
					     const void *source,
					     uint8_t level, const void *data,
					     size_t dlen, uint32_t package_flags,
					     const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	z_log_msg_runtime_vcreate(domain_id, source, level,
				   data, dlen, package_flags, fmt, ap);
	va_end(ap);
}

static inline bool z_log_item_is_msg(const union log_msg_generic *msg)
{
	return msg->generic.type == Z_LOG_MSG_LOG;
}

/** @brief Get total length (in 32 bit words) of a log message.
 *
 * @param desc Log message descriptor.
 *
 * @return Length.
 */
static inline uint32_t log_msg_get_total_wlen(const struct log_msg_desc desc)
{
	return Z_LOG_MSG_ALIGNED_WLEN(desc.package_len, desc.data_len);
}

/** @brief Get length of the log item.
 *
 * @param item Item.
 *
 * @return Length in 32 bit words.
 */
static inline uint32_t log_msg_generic_get_wlen(const union mpsc_pbuf_generic *item)
{
	const union log_msg_generic *generic_msg = (const union log_msg_generic *)item;

	if (z_log_item_is_msg(generic_msg)) {
		const struct log_msg *msg = (const struct log_msg *)generic_msg;

		return log_msg_get_total_wlen(msg->hdr.desc);
	}

	return 0;
}

/** @brief Get log message domain ID.
 *
 * @param msg Log message.
 *
 * @return Domain ID
 */
static inline uint8_t log_msg_get_domain(struct log_msg *msg)
{
	return msg->hdr.desc.domain;
}

/** @brief Get log message level.
 *
 * @param msg Log message.
 *
 * @return Log level.
 */
static inline uint8_t log_msg_get_level(struct log_msg *msg)
{
	return msg->hdr.desc.level;
}

/** @brief Get message source data.
 *
 * @param msg Log message.
 *
 * @return Pointer to the source data.
 */
static inline const void *log_msg_get_source(struct log_msg *msg)
{
	return msg->hdr.source;
}

/** @brief Get log message source ID.
 *
 * @param msg Log message.
 *
 * @return Source ID, or -1 if not available.
 */
int16_t log_msg_get_source_id(struct log_msg *msg);

/** @brief Get timestamp.
 *
 * @param msg Log message.
 *
 * @return Timestamp.
 */
static inline log_timestamp_t log_msg_get_timestamp(struct log_msg *msg)
{
	return msg->hdr.timestamp;
}

/** @brief Get Thread ID.
 *
 * @param msg Log message.
 *
 * @return Thread ID.
 */
static inline void *log_msg_get_tid(struct log_msg *msg)
{
#if defined(CONFIG_LOG_THREAD_ID_PREFIX)
	return msg->hdr.tid;
#else
	ARG_UNUSED(msg);
	return NULL;
#endif
}

/** @brief Get data buffer.
 *
 * @param msg log message.
 *
 * @param len location where data length is written.
 *
 * @return pointer to the data buffer.
 */
static inline uint8_t *log_msg_get_data(struct log_msg *msg, size_t *len)
{
	*len = msg->hdr.desc.data_len;

	return msg->data + msg->hdr.desc.package_len;
}

/** @brief Get string package.
 *
 * @param msg log message.
 *
 * @param len location where string package length is written.
 *
 * @return pointer to the package.
 */
static inline uint8_t *log_msg_get_package(struct log_msg *msg, size_t *len)
{
	*len = msg->hdr.desc.package_len;

	return msg->data;
}

/**
 * @}
 */

#include <zephyr/syscalls/log_msg.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_MSG_H_ */
