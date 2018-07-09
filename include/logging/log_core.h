/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_FRONTEND_H
#define LOG_FRONTEND_H

#include <logging/log_msg.h>
#include <logging/log_instance.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_LOG_DEFAULT_LEVEL
#define CONFIG_LOG_DEFAULT_LEVEL LOG_LEVEL_NONE
#endif

#ifndef CONFIG_LOG_MAX_LEVEL
#define CONFIG_LOG_MAX_LEVEL LOG_LEVEL_NONE
#endif

#define CONFIG_LOG_DOMAIN_ID 0

#define LOG_LEVEL_BITS          3

/** @brief Macro for returning local level value if defined or default.
 *
 * Check @ref IS_ENABLED macro for detailed explanation of the trick.
 */
#define _LOG_RESOLVED_LEVEL(_level, _default) \
	_LOG_RESOLVED_LEVEL1(_level, _default)

#define _LOG_RESOLVED_LEVEL1(_level, _default) \
	__LOG_RESOLVED_LEVEL2(_LOG_XXXX##_level, _level, _default)

#define _LOG_XXXX0 _LOG_YYYY,
#define _LOG_XXXX1 _LOG_YYYY,
#define _LOG_XXXX2 _LOG_YYYY,
#define _LOG_XXXX3 _LOG_YYYY,
#define _LOG_XXXX4 _LOG_YYYY,

#define __LOG_RESOLVED_LEVEL2(one_or_two_args, _level, _default) \
	__LOG_RESOLVED_LEVEL3(one_or_two_args _level, _default)

#define __LOG_RESOLVED_LEVEL3(ignore_this, val, ...) val

/******************************************************************************/
/****************** Internal macros for log frontend **************************/
/******************************************************************************/
#define _LOG_INTERNAL_X(N, ...)  UTIL_CAT(_LOG_INTERNAL_, N)(__VA_ARGS__)

#define __LOG_INTERNAL(_src_level, ...)			 \
	_LOG_INTERNAL_X(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
			_src_level, __VA_ARGS__)

#define _LOG_INTERNAL_0(_src_level, _str) \
	log_0(_str, _src_level)

#define _LOG_INTERNAL_1(_src_level, _str, _arg0) \
	log_1(_str, (u32_t)(_arg0), _src_level)

#define _LOG_INTERNAL_2(_src_level, _str, _arg0, _arg1)	\
	log_2(_str, (u32_t)(_arg0), (u32_t)(_arg1), _src_level)

#define _LOG_INTERNAL_3(_src_level, _str, _arg0, _arg1, _arg2) \
	log_3(_str, (u32_t)(_arg0), (u32_t)(_arg1), (u32_t)(_arg2), _src_level)

#define __LOG_ARG_CAST(_x) (u32_t)(_x),

#define __LOG_ARGUMENTS(...) MACRO_MAP(__LOG_ARG_CAST, __VA_ARGS__)

#define _LOG_INTERNAL_LONG(_src_level, _str, ...)		 \
	do {							 \
		u32_t args[] = {__LOG_ARGUMENTS(__VA_ARGS__)};	 \
		log_n(_str, args, ARRAY_SIZE(args), _src_level); \
	} while (0)

#define _LOG_INTERNAL_4(_src_level, _str, ...) \
		_LOG_INTERNAL_LONG(_src_level, _str, __VA_ARGS__)

#define _LOG_INTERNAL_5(_src_level, _str, ...) \
		_LOG_INTERNAL_LONG(_src_level, _str, __VA_ARGS__)

#define _LOG_INTERNAL_6(_src_level, _str, ...) \
		_LOG_INTERNAL_LONG(_src_level, _str, __VA_ARGS__)

#define _LOG_INTERNAL_7(_src_level, _str, ...) \
		_LOG_INTERNAL_LONG(_src_level, _str, __VA_ARGS__)

#define _LOG_INTERNAL_8(_src_level, _str, ...) \
		_LOG_INTERNAL_LONG(_src_level, _str, __VA_ARGS__)

#define _LOG_INTERNAL_9(_src_level, _str, ...) \
		_LOG_INTERNAL_LONG(_src_level, _str, __VA_ARGS__)

#define _LOG_LEVEL_CHECK(_level, _check_level, _default_level) \
	(_level <= _LOG_RESOLVED_LEVEL(_check_level, _default_level))

#define _LOG_CONST_LEVEL_CHECK(_level)					    \
	(IS_ENABLED(CONFIG_LOG) &&					    \
	(								    \
	_LOG_LEVEL_CHECK(_level, CONFIG_LOG_OVERRIDE_LEVEL, LOG_LEVEL_NONE) \
	||								    \
	(!IS_ENABLED(CONFIG_LOG_OVERRIDE_LEVEL) &&			    \
	_LOG_LEVEL_CHECK(_level, LOG_LEVEL, CONFIG_LOG_DEFAULT_LEVEL) &&    \
	(_level <= CONFIG_LOG_MAX_LEVEL)				    \
	)								    \
	))

/******************************************************************************/
/****************** Macros for standard logging *******************************/
/******************************************************************************/
#define __LOG(_level, _id, _filter, ...)			   \
	do {							   \
		if (_LOG_CONST_LEVEL_CHECK(_level) &&		   \
		    (_level <= LOG_RUNTIME_FILTER(_filter))) {	   \
			struct log_msg_ids src_level = {	   \
				.level = _level,		   \
				.source_id = _id,		   \
				.domain_id = CONFIG_LOG_DOMAIN_ID  \
			};					   \
			log_printf_arg_checker(__VA_ARGS__);	   \
			__LOG_INTERNAL(src_level, __VA_ARGS__);	   \
		}						   \
	} while (0)

#define _LOG(_level, ...)			       \
	__LOG(_level,				       \
	      LOG_CURRENT_MODULE_ID(),		       \
	      &LOG_ITEM_DYNAMIC_DATA(LOG_MODULE_NAME), \
	      __VA_ARGS__)

#define _LOG_INSTANCE(_level, _inst, ...)		 \
	__LOG(_level,					 \
	      IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ? \
	      LOG_DYNAMIC_ID_GET(_inst) :		 \
	      LOG_CONST_ID_GET(_inst),			 \
	      _inst,					 \
	      __VA_ARGS__)


/******************************************************************************/
/****************** Macros for hexdump logging ********************************/
/******************************************************************************/
#define __LOG_HEXDUMP(_level, _id, _filter, _data, _length)	   \
	do {							   \
		if (_LOG_CONST_LEVEL_CHECK(_level) &&		   \
		    (_level <= LOG_RUNTIME_FILTER(_filter))) {	   \
			struct log_msg_ids src_level = {	   \
				.level = _level,		   \
				.source_id = _id,		   \
				.domain_id = CONFIG_LOG_DOMAIN_ID  \
			};					   \
			log_hexdump(_data, _length, src_level);	   \
		}						   \
	} while (0)

#define _LOG_HEXDUMP(_level, _data, _length)		       \
	__LOG_HEXDUMP(_level,				       \
		      LOG_CURRENT_MODULE_ID(),		       \
		      &LOG_ITEM_DYNAMIC_DATA(LOG_MODULE_NAME), \
		      _data, _length)

#define _LOG_HEXDUMP_INSTANCE(_level, _inst, _data, _length)	 \
	__LOG_HEXDUMP(_level,					 \
		      IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ? \
		      LOG_DYNAMIC_ID_GET(_inst) :		 \
		      LOG_CONST_ID_GET(_inst),			 \
		      _inst,					 \
		      _data,					 \
		      _length)

/** @brief Dummy function to trigger log messages arguments type checking. */
static inline __printf_like(1, 2)
void log_printf_arg_checker(const char *fmt, ...)
{

}

/** @brief Standard log with no arguments.
 *
 * @param str           String.
 * @param src_level	Log identification.
 */
void log_0(const char *str, struct log_msg_ids src_level);

/** @brief Standard log with one argument.
 *
 * @param str           String.
 * @param arg1	        First argument.
 * @param src_level	Log identification.
 */
void log_1(const char *str,
	   u32_t arg1,
	   struct log_msg_ids src_level);

/** @brief Standard log with two arguments.
 *
 * @param str           String.
 * @param arg1	        First argument.
 * @param arg2	        Second argument.
 * @param src_level	Log identification.
 */
void log_2(const char *str,
	   u32_t arg1,
	   u32_t arg2,
	   struct log_msg_ids src_level);

/** @brief Standard log with three arguments.
 *
 * @param str           String.
 * @param arg1	        First argument.
 * @param arg2	        Second argument.
 * @param arg3	        Third argument.
 * @param src_level	Log identification.
 */
void log_3(const char *str,
	   u32_t arg1,
	   u32_t arg2,
	   u32_t arg3,
	   struct log_msg_ids src_level);

/** @brief Standard log with arguments list.
 *
 * @param str		String.
 * @param args		Array with arguments.
 * @param narg		Number of arguments in the array.
 * @param src_level	Log identification.
 */
void log_n(const char *str,
	   u32_t *args,
	   u32_t narg,
	   struct log_msg_ids src_level);

/** @brief Hexdump log.
 *
 * @param data		Data.
 * @param length	Data length.
 * @param src_level	Log identification.
 */
void log_hexdump(const u8_t *data,
		 u32_t length,
		 struct log_msg_ids src_level);

/** @brief Format and put string into log message.
 *
 * @param fmt	String to format.
 * @param ap	Variable list of arguments.
 *
 * @return Number of bytes processed.
 */
int log_printk(const char *fmt, va_list ap);

/**
 * @brief Writes a generic log message to the log.
 *
 * @note This function is intended to be used when porting other log systems.
 */
void log_generic(struct log_msg_ids src_level, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* LOG_FRONTEND_H */
