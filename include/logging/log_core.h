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

/******************************************************************************/
/****************** Internal macros for log frontend **************************/
/******************************************************************************/
#define _LOG_INTERNAL_X(N, ...)  UTIL_CAT(_LOG_INTERNAL_, N) (__VA_ARGS__)

#define __LOG_INTERNAL(_src_level, ...)			 \
	_LOG_INTERNAL_X(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
			_src_level, __VA_ARGS__)

#define _LOG_INTERNAL_0(_src_level, _str) \
	log_0(_str, _src_level)

#define _LOG_INTERNAL_1(_src_level, _str, _arg0) \
	log_1(_str, (u32_t)_arg0, _src_level)

#define _LOG_INTERNAL_2(_src_level, _str, _arg0, _arg1)	\
	log_2(_str, (u32_t)_arg0, (u32_t)_arg1, _src_level)

#define _LOG_INTERNAL_3(_src_level, _str, _arg0, _arg1, _arg2) \
	log_3(_str, (u32_t)_arg0, (u32_t)_arg1, (u32_t)_arg2, _src_level)

#define _LOG_INTERNAL_4(_src_level, _str, _arg0, _arg1, _arg2, _arg3) \
	log_4(_str, (u32_t)_arg0, (u32_t)_arg1, (u32_t)_arg2,	      \
	      (u32_t)_arg3, _src_level)

#define _LOG_INTERNAL_5(_src_level, _str, _arg0, _arg1, _arg2, _arg3, _arg4) \
	log_5(_str, (u32_t)_arg0, (u32_t)_arg1, (u32_t)_arg2,		     \
	      (u32_t)_arg3, (u32_t)_arg4, _src_level)

#define _LOG_INTERNAL_6(_src_level, _str, _arg0, _arg1, _arg2, \
			_arg3, _arg4, _arg5)		       \
	log_6(_str, (u32_t)_arg0, (u32_t)_arg1, (u32_t)_arg2,  \
	      (u32_t)_arg3, (u32_t)_arg4, (u32_t)_arg5, _src_level)


#define LOG_SRC_LEVEL_MSK(_id, _level)				  \
	(((_level) << LOG_MSG_LEVEL_OFFSET)                     | \
	 ((_id) << LOG_MSG_SRC_ID_OFFSET)                       | \
	 (CONFIG_LOG_DOMAIN_ID << LOG_MSG_DOMAIN_ID_OFFSET))

/******************************************************************************/
/****************** Macros for standard logging *******************************/
/******************************************************************************/
#define _LOG_INTERNAL(_level, _id, _p_filter, ...)	  \
	if (IS_ENABLED(CONFIG_LOG)              &&	  \
	    (_level <= LOG_SRC_LEVEL)           &&	  \
	    (_level <= CONFIG_LOG_MAX_LEVEL)    &&	  \
	    (_level <= LOG_RUNTIME_FILTER(_p_filter))) {  \
		struct log_msg_ids src_level = {	  \
			.level = _level,		  \
			.source_id = _id,		  \
			.domain_id = CONFIG_LOG_DOMAIN_ID \
		};					  \
		__LOG_INTERNAL(src_level, __VA_ARGS__);	  \
	}

#define LOG_INTERNAL_MODULE(_level, ...)				   \
	_LOG_INTERNAL(_level,						   \
		      LOG_ROM_ID_GET(&LOG_ITEM_ROM_DATA(LOG_MODULE_NAME)), \
		      &LOG_ITEM_RAM_DATA(LOG_MODULE_NAME),		   \
		      __VA_ARGS__)

#define LOG_INTERNAL_INSTANCE(_level, _p_inst, ...)		 \
	_LOG_INTERNAL(_level,					 \
		      IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ? \
		      LOG_RAM_ID_GET(_p_inst) :			 \
		      LOG_ROM_ID_GET(_p_inst),			 \
		      _p_inst,					 \
		      __VA_ARGS__)


/******************************************************************************/
/****************** Macros for hexdump logging ********************************/
/******************************************************************************/
#define _LOG_INTERNAL_HEXDUMP(_level, _id, _p_filter, _p_data, _length)	\
	if (IS_ENABLED(CONFIG_LOG)              &&			\
	    (_level <= LOG_SRC_LEVEL)           &&			\
	    (_level <= CONFIG_LOG_MAX_LEVEL)    &&			\
	    (_level <= LOG_RUNTIME_FILTER(_p_filter))) {		\
		struct log_msg_ids src_level = {			\
			.level = _level,				\
			.source_id = _id,				\
			.domain_id = CONFIG_LOG_DOMAIN_ID		\
		};							\
		log_hexdump(_p_data, _length, src_level);		\
	}

#define LOG_INTERNAL_HEXDUMP_MODULE(_level, _p_data, _length)			   \
	_LOG_INTERNAL_HEXDUMP(_level,						   \
			      LOG_ROM_ID_GET(&LOG_ITEM_ROM_DATA(LOG_MODULE_NAME)), \
			      &LOG_ITEM_RAM_DATA(LOG_MODULE_NAME),		   \
			      _p_data, _length)

#define LOG_INTERNAL_HEXDUMP_INSTANCE(_level, _p_inst, _p_data, _length) \
	_LOG_INTERNAL_HEXDUMP(_level,					 \
			      IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ? \
			      LOG_RAM_ID_GET(_p_inst) :			 \
			      LOG_ROM_ID_GET(_p_inst),			 \
			      _p_inst,					 \
			      _p_data,					 \
			      _length)

/** @brief Standard log with no arguments.
 *
 * @param p_str         String.
 * @param src_level	Log identification.
 */
void log_0(const char *p_str, struct log_msg_ids src_level);

/** @brief Standard log with one argument.
 *
 * @param p_str         String.
 * @param arg1	        First argument.
 * @param src_level	Log identification.
 */
void log_1(const char *p_str,
	   u32_t arg1,
	   struct log_msg_ids src_level);

/** @brief Standard log with two arguments.
 *
 * @param p_str         String.
 * @param arg1	        First argument.
 * @param arg2	        Second argument.
 * @param src_level	Log identification.
 */
void log_2(const char *p_str,
	   u32_t arg1,
	   u32_t arg2,
	   struct log_msg_ids src_level);

/** @brief Standard log with three arguments.
 *
 * @param p_str         String.
 * @param arg1	        First argument.
 * @param arg2	        Second argument.
 * @param arg3	        Third argument.
 * @param src_level	Log identification.
 */
void log_3(const char *p_str,
	   u32_t arg1,
	   u32_t arg2,
	   u32_t arg3,
	   struct log_msg_ids src_level);

/** @brief Standard log with four arguments.
 *
 * @param p_str         String.
 * @param arg1	        First argument.
 * @param arg2	        Second argument.
 * @param arg3	        Third argument.
 * @param arg4	        Fourth argument.
 * @param src_level	Log identification.
 */
void log_4(const char *p_str,
	   u32_t arg1,
	   u32_t arg2,
	   u32_t arg3,
	   u32_t arg4,
	   struct log_msg_ids src_level);

/** @brief Standard log with five arguments.
 *
 * @param p_str         String.
 * @param arg1	        First argument.
 * @param arg2	        Second argument.
 * @param arg3	        Third argument.
 * @param arg4	        Fourth argument.
 * @param arg5	        Fifth argument.
 * @param src_level	Log identification.
 */
void log_5(const char *p_str,
	   u32_t arg1,
	   u32_t arg2,
	   u32_t arg3,
	   u32_t arg4,
	   u32_t arg5,
	   struct log_msg_ids src_level);

/** @brief Standard log with six arguments.
 *
 * @param p_str         String.
 * @param arg1	        First argument.
 * @param arg2	        Second argument.
 * @param arg3	        Third argument.
 * @param arg4	        Fourth argument.
 * @param arg5	        Fifth argument.
 * @param arg6	        Sixth argument.
 * @param src_level	Log identification.
 */
void log_6(const char *p_str,
	   u32_t arg1,
	   u32_t arg2,
	   u32_t arg3,
	   u32_t arg4,
	   u32_t arg5,
	   u32_t arg6,
	   struct log_msg_ids src_level);

void log_hexdump(const u8_t *p_data,
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

#ifdef __cplusplus
}
#endif

#endif /* LOG_FRONTEND_H */
