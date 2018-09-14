/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_CORE_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_CORE_H_

#include <logging/log_msg.h>
#include <logging/log_instance.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !CONFIG_LOG
#define CONFIG_LOG_DEFAULT_LEVEL 0
#define CONFIG_LOG_DOMAIN_ID 0
#define CONFIG_LOG_MAX_LEVEL 0
#endif

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
	__LOG_ARG_2(one_or_two_args _level, _default)

#define LOG_DEBRACKET(...) __VA_ARGS__

#define __LOG_ARG_2(ignore_this, val, ...) val
#define __LOG_ARG_2_DEBRACKET(ignore_this, val, ...) LOG_DEBRACKET val

/**
 * @brief Macro for conditional code generation if provided log level allows.
 *
 * Macro behaves similarly to standard #if #else #endif clause. The difference is
 * that it is evaluated when used and not when header file is included.
 *
 * @param _eval_level Evaluated level. If level evaluates to one of existing log
 *		      log level (1-4) then macro evaluates to _iftrue.
 * @param _iftrue     Code that should be inserted when evaluated to true. Note,
 *		      that parameter must be provided in brackets.
 * @param _iffalse    Code that should be inserted when evaluated to false.
 *		      Note, that parameter must be provided in brackets.
 */
#define _LOG_EVAL(_eval_level, _iftrue, _iffalse) \
	_LOG_EVAL1(_eval_level, _iftrue, _iffalse)

#define _LOG_EVAL1(_eval_level, _iftrue, _iffalse) \
	_LOG_EVAL2(_LOG_ZZZZ##_eval_level, _iftrue, _iffalse)

#define _LOG_ZZZZ1 _LOG_YYYY,
#define _LOG_ZZZZ2 _LOG_YYYY,
#define _LOG_ZZZZ3 _LOG_YYYY,
#define _LOG_ZZZZ4 _LOG_YYYY,

#define _LOG_EVAL2(one_or_two_args, _iftrue, _iffalse) \
	__LOG_ARG_2_DEBRACKET(one_or_two_args _iftrue, _iffalse)

/** @brief Macro for getting log level for given module.
 *
 * It is evaluated to LOG_LEVEL if defined. Otherwise CONFIG_LOG_DEFAULT_LEVEL
 * is used.
 */
#define _LOG_LEVEL() _LOG_RESOLVED_LEVEL(LOG_LEVEL, CONFIG_LOG_DEFAULT_LEVEL)

/**
 *  @def LOG_CONST_ID_GET
 *  @brief Macro for getting ID of the element of the section.
 *
 *  @param _addr Address of the element.
 */
#define LOG_CONST_ID_GET(_addr)						       \
	_LOG_EVAL(							       \
	  _LOG_LEVEL(),							       \
	  (log_const_source_id((const struct log_source_const_data *)_addr)),  \
	  (0)								       \
	)

/**
 * @def LOG_CURRENT_MODULE_ID
 * @brief Macro for getting ID of current module.
 */
#define LOG_CURRENT_MODULE_ID()						\
	_LOG_EVAL(							\
	  _LOG_LEVEL(),							\
	  (log_const_source_id(__log_current_const_data_get())),	\
	  (0)								\
	)

/**
 * @def LOG_CURRENT_DYNAMIC_DATA_ADDR
 * @brief Macro for getting address of dynamic structure of current module.
 */
#define LOG_CURRENT_DYNAMIC_DATA_ADDR()			\
	_LOG_EVAL(					\
	  _LOG_LEVEL(),					\
	  (__log_current_dynamic_data_get()),		\
	  ((struct log_source_dynamic_data *)0)		\
	)

/** @brief Macro for getting ID of the element of the section.
 *
 *  @param _addr Address of the element.
 */
#define LOG_DYNAMIC_ID_GET(_addr)					     \
	_LOG_EVAL(							     \
	  _LOG_LEVEL(),							     \
	  (log_dynamic_source_id((struct log_source_dynamic_data *)_addr)),  \
	  (0)								     \
	)

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
#define __LOG(_level, _id, _filter, ...)				    \
	do {								    \
		if (_LOG_CONST_LEVEL_CHECK(_level) &&			    \
		    (_level <= LOG_RUNTIME_FILTER(_filter))) {		    \
			struct log_msg_ids src_level = {		    \
				.level = _level,			    \
				.source_id = _id,			    \
				.domain_id = CONFIG_LOG_DOMAIN_ID	    \
			};						    \
			__LOG_INTERNAL(src_level, __VA_ARGS__);		    \
		} else if (0) {						    \
			/* Arguments checker present but never evaluated.*/ \
			/* Placed here to ensure that __VA_ARGS__ are*/     \
			/* evaluated once when log is enabled.*/	    \
			log_printf_arg_checker(__VA_ARGS__);		    \
		}							    \
	} while (0)

#define _LOG(_level, ...)			       \
	__LOG(_level,				       \
	      LOG_CURRENT_MODULE_ID(),		       \
	      LOG_CURRENT_DYNAMIC_DATA_ADDR(),	       \
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
#define __LOG_HEXDUMP(_level, _id, _filter, _data, _length, _str)     \
	do {							      \
		if (_LOG_CONST_LEVEL_CHECK(_level) &&		      \
		    (_level <= LOG_RUNTIME_FILTER(_filter))) {	      \
			struct log_msg_ids src_level = {	      \
				.level = _level,		      \
				.source_id = _id,		      \
				.domain_id = CONFIG_LOG_DOMAIN_ID     \
			};					      \
			log_hexdump(_str, _data, _length, src_level); \
		}						      \
	} while (0)

#define _LOG_HEXDUMP(_level, _data, _length, _str)	       \
	__LOG_HEXDUMP(_level,				       \
		      LOG_CURRENT_MODULE_ID(),		       \
		      LOG_CURRENT_DYNAMIC_DATA_ADDR(),	       \
		      _data, _length, _str)

#define _LOG_HEXDUMP_INSTANCE(_level, _inst, _data, _length, _str) \
	__LOG_HEXDUMP(_level,					   \
		      IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?   \
		      LOG_DYNAMIC_ID_GET(_inst) :		   \
		      LOG_CONST_ID_GET(_inst),			   \
		      _inst,					   \
		      _data,					   \
		      _length,					   \
		      _str)

/******************************************************************************/
/****************** Filtering macros ******************************************/
/******************************************************************************/

/** @brief Number of bits used to encode log level. */
#define LOG_LEVEL_BITS 3

/** @brief Filter slot size. */
#define LOG_FILTER_SLOT_SIZE LOG_LEVEL_BITS

/** @brief Number of slots in one word. */
#define LOG_FILTERS_NUM_OF_SLOTS (32 / LOG_FILTER_SLOT_SIZE)

/** @brief Slot mask. */
#define LOG_FILTER_SLOT_MASK ((1 << LOG_FILTER_SLOT_SIZE) - 1)

/** @brief Bit offset of a slot.
 *
 *  @param _id Slot ID.
 */
#define LOG_FILTER_SLOT_SHIFT(_id) (LOG_FILTER_SLOT_SIZE * (_id))

#define LOG_FILTER_SLOT_GET(_filters, _id) \
	((*(_filters) >> LOG_FILTER_SLOT_SHIFT(_id)) & LOG_FILTER_SLOT_MASK)

#define LOG_FILTER_SLOT_SET(_filters, _id, _filter)		     \
	do {							     \
		*(_filters) &= ~(LOG_FILTER_SLOT_MASK <<	     \
				 LOG_FILTER_SLOT_SHIFT(_id));	     \
		*(_filters) |= ((_filter) & LOG_FILTER_SLOT_MASK) << \
			       LOG_FILTER_SLOT_SHIFT(_id);	     \
	} while (0)

#define LOG_FILTER_AGGR_SLOT_IDX 0

#define LOG_FILTER_AGGR_SLOT_GET(_filters) \
	LOG_FILTER_SLOT_GET(_filters, LOG_FILTER_AGGR_SLOT_IDX)

#define LOG_FILTER_FIRST_BACKEND_SLOT_IDX 1

#if CONFIG_LOG_RUNTIME_FILTERING
#define LOG_RUNTIME_FILTER(_filter) \
	LOG_FILTER_SLOT_GET(&(_filter)->filters, LOG_FILTER_AGGR_SLOT_IDX)
#else
#define LOG_RUNTIME_FILTER(_filter) LOG_LEVEL_DBG
#endif

extern struct log_source_const_data __log_const_start[0];
extern struct log_source_const_data __log_const_end[0];

/** @brief Get name of the log source.
 *
 * @param source_id Source ID.
 * @return Name.
 */
static inline const char *log_name_get(u32_t source_id)
{
	return __log_const_start[source_id].name;
}

/** @brief Get compiled level of the log source.
 *
 * @param source_id Source ID.
 * @return Level.
 */
static inline u8_t log_compiled_level_get(u32_t source_id)
{
	return __log_const_start[source_id].level;
}

/** @brief Get index of the log source based on the address of the constant data
 *         associated with the source.
 *
 * @param data Address of the constant data.
 *
 * @return Source ID.
 */
static inline u32_t log_const_source_id(
				const struct log_source_const_data *data)
{
	return ((void *)data - (void *)__log_const_start)/
			sizeof(struct log_source_const_data);
}

/** @brief Get number of registered sources. */
static inline u32_t log_sources_count(void)
{
	return log_const_source_id(__log_const_end);
}

extern struct log_source_dynamic_data __log_dynamic_start[0];
extern struct log_source_dynamic_data __log_dynamic_end[0];

/** @brief Creates name of variable and section for runtime log data.
 *
 *  @param _name Name.
 */
#define LOG_ITEM_DYNAMIC_DATA(_name) UTIL_CAT(log_dynamic_, _name)

#define LOG_INSTANCE_DYNAMIC_DATA(_module_name, _inst) \
	LOG_ITEM_DYNAMIC_DATA(LOG_INSTANCE_FULL_NAME(_module_name, _inst))

/** @brief Get pointer to the filter set of the log source.
 *
 * @param source_id Source ID.
 *
 * @return Pointer to the filter set.
 */
static inline u32_t *log_dynamic_filters_get(u32_t source_id)
{
	return &__log_dynamic_start[source_id].filters;
}

/** @brief Get index of the log source based on the address of the dynamic data
 *         associated with the source.
 *
 * @param data Address of the dynamic data.
 *
 * @return Source ID.
 */
static inline u32_t log_dynamic_source_id(struct log_source_dynamic_data *data)
{
	return ((void *)data - (void *)__log_dynamic_start)/
			sizeof(struct log_source_dynamic_data);
}

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
 * @param str		String.
 * @param data		Data.
 * @param length	Data length.
 * @param src_level	Log identification.
 */
void log_hexdump(const char *str,
		 const u8_t *data,
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

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_CORE_H_ */
