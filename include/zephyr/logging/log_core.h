/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_CORE_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_CORE_H_

#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_instance.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <zephyr/sys/util.h>

/* This header file keeps all macros and functions needed for creating logging
 * messages (macros like @ref LOG_ERR).
 */
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERR  1
#define LOG_LEVEL_WRN  2
#define LOG_LEVEL_INF  3
#define LOG_LEVEL_DBG  4

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_LOG
#define CONFIG_LOG_DEFAULT_LEVEL 0
#define CONFIG_LOG_MAX_LEVEL 0
#endif

/* Id of local domain. */
#define Z_LOG_LOCAL_DOMAIN_ID 0

#define LOG_FUNCTION_PREFIX_MASK \
	(((uint32_t)IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_ERR) << \
	  LOG_LEVEL_ERR) | \
	 ((uint32_t)IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_WRN) << \
	  LOG_LEVEL_WRN) | \
	 ((uint32_t)IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_INF) << \
	  LOG_LEVEL_INF) | \
	 ((uint32_t)IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_DBG) << LOG_LEVEL_DBG))

/** @brief Macro for returning local level value if defined or default.
 *
 * Check @ref IS_ENABLED macro for detailed explanation of the trick.
 */
#define Z_LOG_RESOLVED_LEVEL(_level, _default) \
	Z_LOG_RESOLVED_LEVEL1(_level, _default)

#define Z_LOG_RESOLVED_LEVEL1(_level, _default) \
	__COND_CODE(_LOG_XXXX##_level, (_level), (_default))

#define _LOG_XXXX0  _LOG_YYYY,
#define _LOG_XXXX1  _LOG_YYYY,
#define _LOG_XXXX2  _LOG_YYYY,
#define _LOG_XXXX3  _LOG_YYYY,
#define _LOG_XXXX4  _LOG_YYYY,

/**
 * @brief Macro for conditional code generation if provided log level allows.
 *
 * Macro behaves similarly to standard \#if \#else \#endif clause. The
 * difference is that it is evaluated when used and not when header file is
 * included.
 *
 * @param _eval_level Evaluated level. If level evaluates to one of existing log
 *		      log level (1-4) then macro evaluates to _iftrue.
 * @param _iftrue     Code that should be inserted when evaluated to true. Note,
 *		      that parameter must be provided in brackets.
 * @param _iffalse    Code that should be inserted when evaluated to false.
 *		      Note, that parameter must be provided in brackets.
 */
#define Z_LOG_EVAL(_eval_level, _iftrue, _iffalse) \
	Z_LOG_EVAL1(_eval_level, _iftrue, _iffalse)

#define Z_LOG_EVAL1(_eval_level, _iftrue, _iffalse) \
	__COND_CODE(_LOG_ZZZZ##_eval_level, _iftrue, _iffalse)

#define _LOG_ZZZZ1  _LOG_YYYY,
#define _LOG_ZZZZ2  _LOG_YYYY,
#define _LOG_ZZZZ3  _LOG_YYYY,
#define _LOG_ZZZZ4  _LOG_YYYY,

/**
 *
 * @brief Macro for getting ID of current module.
 */
#define LOG_CURRENT_MODULE_ID() (__log_level != 0 ? \
	log_const_source_id(__log_current_const_data) : 0U)

/* Set of defines that are set to 1 if function name prefix is enabled for given level. */
#define Z_LOG_FUNC_PREFIX_0 0
#define Z_LOG_FUNC_PREFIX_1 COND_CODE_1(CONFIG_LOG_FUNC_NAME_PREFIX_ERR, (1), (0))
#define Z_LOG_FUNC_PREFIX_2 COND_CODE_1(CONFIG_LOG_FUNC_NAME_PREFIX_WRN, (1), (0))
#define Z_LOG_FUNC_PREFIX_3 COND_CODE_1(CONFIG_LOG_FUNC_NAME_PREFIX_INF, (1), (0))
#define Z_LOG_FUNC_PREFIX_4 COND_CODE_1(CONFIG_LOG_FUNC_NAME_PREFIX_DBG, (1), (0))

/**
 * @brief Macro for optional injection of function name as first argument of
 *	  formatted string. COND_CODE_0() macro is used to handle no arguments
 *	  case.
 *
 * The purpose of this macro is to prefix string literal with format specifier
 * for function name and inject function name as first argument. In order to
 * handle string with no arguments _LOG_Z_EVAL is used.
 */
#define Z_LOG_STR_WITH_PREFIX2(...) \
	"%s: " GET_ARG_N(1, __VA_ARGS__), (const char *)__func__\
		COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__),\
			    (),\
			    (, GET_ARGS_LESS_N(1, __VA_ARGS__))\
			   )

/* Macro handles case when no format string is provided: e.g. LOG_DBG().
 * Handling of format string is deferred to the next level macro.
 */
#define Z_LOG_STR_WITH_PREFIX(...) \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(_, ##__VA_ARGS__), \
		("%s", (const char *)__func__), \
		(Z_LOG_STR_WITH_PREFIX2(__VA_ARGS__)))

/**
 * @brief Handle optional injection of function name as the first argument.
 *
 * Additionally, macro is handling the empty message case.
 */
#define Z_LOG_STR(_level, ...) \
	COND_CODE_1(UTIL_CAT(Z_LOG_FUNC_PREFIX_##_level), \
		(Z_LOG_STR_WITH_PREFIX(__VA_ARGS__)), (__VA_ARGS__))

#define Z_LOG_LEVEL_CHECK(_level, _check_level, _default_level) \
	((_level) <= Z_LOG_RESOLVED_LEVEL(_check_level, _default_level))

/** @brief Compile time level checking.
 *
 * This check is resolved at compile time and logging message is removed if check fails.
 *
 * @param _level Log level.
 *
 * @retval true Message shall be compiled in.
 * @retval false Message shall removed during the compilation.
 */
#define Z_LOG_CONST_LEVEL_CHECK(_level)					    \
	(IS_ENABLED(CONFIG_LOG) &&					    \
	(Z_LOG_LEVEL_CHECK(_level, CONFIG_LOG_OVERRIDE_LEVEL, LOG_LEVEL_NONE) \
	||								    \
	((IS_ENABLED(CONFIG_LOG_OVERRIDE_LEVEL) == false) &&		    \
	((_level) <= __log_level) &&					    \
	((_level) <= CONFIG_LOG_MAX_LEVEL)				    \
	)								    \
	))

/** @brief Static level checking for instance logging.
 *
 * This check applies only to instance logging and only if runtime filtering
 * is disabled. It is performed in runtime but because level comes from the
 * structure which is constant it is not exact runtime filtering because it
 * cannot be changed in runtime.
 *
 * @param _level Log level.
 * @param _inst 1 is source is the instance of a module.
 * @param _source Data associated with the instance.
 *
 * @retval true Continue with log message creation.
 * @retval false Drop that message.
 */
#define Z_LOG_STATIC_INST_LEVEL_CHECK(_level, _inst, _source)                                      \
	(IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) || !_inst ||                                     \
	 (_level <= ((const struct log_source_const_data *)_source)->level))

/** @brief Dynamic level checking.
 *
 * It uses the level from the dynamic structure.
 *
 * @param _level Log level.
 * @param _source Data associated with the source.
 *
 * @retval true Continue with log message creation.
 * @retval false Drop that message.
 */
#define Z_LOG_DYNAMIC_LEVEL_CHECK(_level, _source)                                                 \
	(!IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) || k_is_user_context() ||                       \
	 ((_level) <= Z_LOG_RUNTIME_FILTER(((struct log_source_dynamic_data *)_source)->filters)))

/** @brief Check if message shall be created.
 *
 * Aggregate all checks into a single one.
 *
 * @param _level Log level.
 * @param _inst 1 is source is the instance of a module.
 * @param _source Data associated with the source.
 *
 * @retval true Continue with log message creation.
 * @retval false Drop that message.
 */
#define Z_LOG_LEVEL_ALL_CHECK(_level, _inst, _source)                                              \
	(Z_LOG_CONST_LEVEL_CHECK(_level) &&                                                        \
	 Z_LOG_STATIC_INST_LEVEL_CHECK(_level, _inst, _source) &&                                  \
	 Z_LOG_DYNAMIC_LEVEL_CHECK(_level, _source))

/** @brief Get current module data that is used for source id retrieving.
 *
 * If runtime filtering is used then pointer to dynamic data is returned and else constant
 * data is used.
 */
#define Z_LOG_CURRENT_DATA()                                                                       \
	COND_CODE_1(CONFIG_LOG_RUNTIME_FILTERING, \
			(__log_current_dynamic_data), (__log_current_const_data))

/*****************************************************************************/
/****************** Definitions used by minimal logging *********************/
/*****************************************************************************/
void z_log_minimal_hexdump_print(int level, const void *data, size_t size);
void z_log_minimal_vprintk(const char *fmt, va_list ap);
void z_log_minimal_printk(const char *fmt, ...);

#define Z_LOG_TO_PRINTK(_level, fmt, ...) do { \
	z_log_minimal_printk("%c: " fmt "\n", \
			     z_log_minimal_level_to_char(_level), \
			     ##__VA_ARGS__); \
} while (false)

#define Z_LOG_TO_VPRINTK(_level, fmt, valist) do { \
	z_log_minimal_printk("%c: ", z_log_minimal_level_to_char(_level)); \
	z_log_minimal_vprintk(fmt, valist); \
	z_log_minimal_printk("\n"); \
} while (false)

static inline char z_log_minimal_level_to_char(int level)
{
	switch (level) {
	case LOG_LEVEL_ERR:
		return 'E';
	case LOG_LEVEL_WRN:
		return 'W';
	case LOG_LEVEL_INF:
		return 'I';
	case LOG_LEVEL_DBG:
		return 'D';
	default:
		return '?';
	}
}

#define Z_LOG_INST(_inst) COND_CODE_1(CONFIG_LOG, (_inst), NULL)

/* If strings are removed from the binary then there is a risk of creating invalid
 * cbprintf package if %p is used with character pointer which is interpreted as
 * string. A compile time check is performed (since format string is known at
 * compile time) and check fails logging message is not created but error is
 * emitted instead. String check may increase compilation time so it is not
 * always performed (could significantly increase CI time).
 */
#ifdef CONFIG_LOG_FMT_STRING_VALIDATE
#define LOG_STRING_WARNING(_mode, _src, ...) \
	    Z_LOG_MSG_CREATE(UTIL_NOT(IS_ENABLED(CONFIG_USERSPACE)), _mode, \
			     Z_LOG_LOCAL_DOMAIN_ID, _src, LOG_LEVEL_ERR, NULL, 0, \
			     "char pointer used for %%p, cast to void *:\"%s\"", \
			     GET_ARG_N(1, __VA_ARGS__))

#define LOG_POINTERS_VALIDATE(string_ok, ...) \
	TOOLCHAIN_DISABLE_GCC_WARNING(TOOLCHAIN_WARNING_POINTER_ARITH); \
	string_ok = Z_CBPRINTF_POINTERS_VALIDATE(__VA_ARGS__); \
	TOOLCHAIN_ENABLE_GCC_WARNING(TOOLCHAIN_WARNING_POINTER_ARITH);
#else
#define LOG_POINTERS_VALIDATE(string_ok, ...) string_ok = true
#define LOG_STRING_WARNING(_mode, _src, ...)
#endif

/*****************************************************************************/
/****************** Macros for standard logging ******************************/
/*****************************************************************************/
/** @internal
 * @brief Generic logging macro.
 *
 * It checks against static levels (resolved at compile timer), runtime levels
 * and modes and dispatch to relevant processing path.
 *
 * @param _level Log message severity level.
 *
 * @param _inst Set to 1 for instance specific log message. 0 otherwise.
 * @param _source Pointer to a structure associated with the module or instance.
 *                If it is a module then it is used only when runtime filtering is
 *                enabled. If it is instance then it is used in both cases.
 *
 * @param ... String with arguments.
 */
#define Z_LOG2(_level, _inst, _source, ...)                                                        \
	do {                                                                                       \
		if (!Z_LOG_LEVEL_ALL_CHECK(_level, _inst, _source)) {                              \
			break;                                                                     \
		}                                                                                  \
		if (IS_ENABLED(CONFIG_LOG_MODE_MINIMAL)) {                                         \
			Z_LOG_TO_PRINTK(_level, __VA_ARGS__);                                      \
			break;                                                                     \
		}                                                                                  \
		int _mode;                                                                         \
		bool string_ok;                                                                    \
		LOG_POINTERS_VALIDATE(string_ok, __VA_ARGS__);                                     \
		if (!string_ok) {                                                                  \
			LOG_STRING_WARNING(_mode, _source, __VA_ARGS__);                           \
			break;                                                                     \
		}                                                                                  \
		Z_LOG_MSG_CREATE(UTIL_NOT(IS_ENABLED(CONFIG_USERSPACE)), _mode,                    \
				 Z_LOG_LOCAL_DOMAIN_ID, _source, _level, NULL, 0, __VA_ARGS__);    \
		(void)_mode;                                                                       \
		if (false) {                                                                       \
			/* Arguments checker present but never evaluated.*/                        \
			/* Placed here to ensure that __VA_ARGS__ are*/                            \
			/* evaluated once when log is enabled.*/                                   \
			z_log_printf_arg_checker(__VA_ARGS__);                                     \
		}                                                                                  \
	} while (false)

#define Z_LOG(_level, ...)                 Z_LOG2(_level, 0, Z_LOG_CURRENT_DATA(), __VA_ARGS__)
#define Z_LOG_INSTANCE(_level, _inst, ...) Z_LOG2(_level, 1, Z_LOG_INST(_inst), __VA_ARGS__)

/*****************************************************************************/
/****************** Macros for hexdump logging *******************************/
/*****************************************************************************/
/** @internal
 * @brief Generic logging macro.
 *
 * It checks against static levels (resolved at compile timer), runtime levels
 * and modes and dispatch to relevant processing path.
 *
 * @param _level Log message severity level.
 *
 * @param _inst Set to 1 for instance specific log message. 0 otherwise.
 *
 * @param _source Pointer to a structure associated with the module or instance.
 *                If it is a module then it is used only when runtime filtering is
 *                enabled. If it is instance then it is used in both cases.
 *
 * @param _data Hexdump data;
 *
 * @param _len Hexdump data length.
 *
 * @param ... String.
 */
#define Z_LOG_HEXDUMP2(_level, _inst, _source, _data, _len, ...)                                   \
	do {                                                                                       \
		if (!Z_LOG_LEVEL_ALL_CHECK(_level, _inst, _source)) {                              \
			break;                                                                     \
		}                                                                                  \
		const char *_str = GET_ARG_N(1, __VA_ARGS__);                                      \
		if (IS_ENABLED(CONFIG_LOG_MODE_MINIMAL)) {                                         \
			Z_LOG_TO_PRINTK(_level, "%s", _str);                                       \
			z_log_minimal_hexdump_print((_level), (const char *)(_data), (_len));      \
			break;                                                                     \
		}                                                                                  \
		int _mode;                                                                         \
		Z_LOG_MSG_CREATE(UTIL_NOT(IS_ENABLED(CONFIG_USERSPACE)), _mode,                    \
				 Z_LOG_LOCAL_DOMAIN_ID, _source, _level, _data, _len,              \
				 COND_CODE_0(NUM_VA_ARGS_LESS_1(_, ##__VA_ARGS__), \
				(), \
			  (COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
				  ("%s", __VA_ARGS__), (__VA_ARGS__)))));   \
		(void)_mode;                                                                       \
	} while (false)

#define Z_LOG_HEXDUMP(_level, _data, _length, ...)                                                 \
	Z_LOG_HEXDUMP2(_level, 0, Z_LOG_CURRENT_DATA(), _data, _length, __VA_ARGS__)

#define Z_LOG_HEXDUMP_INSTANCE(_level, _inst, _data, _length, ...)                                 \
	Z_LOG_HEXDUMP2(_level, 1, Z_LOG_INST(_inst), _data, _length, __VA_ARGS__)

/*****************************************************************************/
/****************** Filtering macros *****************************************/
/*****************************************************************************/

/** @brief Number of bits used to encode log level. */
#define LOG_LEVEL_BITS 3U

/** @brief Filter slot size. */
#define LOG_FILTER_SLOT_SIZE LOG_LEVEL_BITS

/** @brief Number of slots in one word. */
#define LOG_FILTERS_NUM_OF_SLOTS (32 / LOG_FILTER_SLOT_SIZE)

/** @brief Maximum number of backends supported when runtime filtering is enabled. */
#define LOG_FILTERS_MAX_BACKENDS \
	(LOG_FILTERS_NUM_OF_SLOTS - (1 + IS_ENABLED(CONFIG_LOG_FRONTEND)))

/** @brief Slot reserved for the frontend. Last slot is used. */
#define LOG_FRONTEND_SLOT_ID (LOG_FILTERS_NUM_OF_SLOTS - 1)

/** @brief Slot mask. */
#define LOG_FILTER_SLOT_MASK (BIT(LOG_FILTER_SLOT_SIZE) - 1U)

/** @brief Bit offset of a slot.
 *
 *  @param _id Slot ID.
 */
#define LOG_FILTER_SLOT_SHIFT(_id) (LOG_FILTER_SLOT_SIZE * (_id))

#define LOG_FILTER_SLOT_GET(_filters, _id) \
	((*(_filters) >> LOG_FILTER_SLOT_SHIFT(_id)) & LOG_FILTER_SLOT_MASK)

#define LOG_FILTER_SLOT_SET(_filters, _id, _filter)			      \
	do {								      \
		uint32_t others = *(_filters) & ~(LOG_FILTER_SLOT_MASK <<     \
				 LOG_FILTER_SLOT_SHIFT(_id));		      \
		*(_filters) = others | (((_filter) & LOG_FILTER_SLOT_MASK) << \
			       LOG_FILTER_SLOT_SHIFT(_id));		      \
	} while (false)

#define LOG_FILTER_AGGR_SLOT_IDX 0

#define LOG_FILTER_AGGR_SLOT_GET(_filters) \
	LOG_FILTER_SLOT_GET(_filters, LOG_FILTER_AGGR_SLOT_IDX)

#define LOG_FILTER_FIRST_BACKEND_SLOT_IDX 1

/* Return aggregated (highest) level for all enabled backends, e.g. if there
 * are 3 active backends, one backend is set to get INF logs from a module and
 * two other backends are set for ERR, returned level is INF.
 */
#define Z_LOG_RUNTIME_FILTER(_filter) \
	LOG_FILTER_SLOT_GET(&(_filter), LOG_FILTER_AGGR_SLOT_IDX)

/** @brief Log level value used to indicate log entry that should not be
 *	   formatted (raw string).
 */
#define LOG_LEVEL_INTERNAL_RAW_STRING LOG_LEVEL_NONE

TYPE_SECTION_START_EXTERN(struct log_source_const_data, log_const);
TYPE_SECTION_END_EXTERN(struct log_source_const_data, log_const);

/** @brief Create message for logging printk-like string or a raw string.
 *
 * Part of printk string processing is appending of carriage return after any
 * new line character found in the string. If it is not desirable then @p _is_raw
 * can be set to 1 to indicate raw string. This information is stored in the source
 * field which is not used for its typical purpose in this case.
 *
 * @param _is_raw	Set to 1 to indicate raw string, set to 0 to indicate printk.
 * @param ...		Format string with arguments.
 */
#define Z_LOG_PRINTK(_is_raw, ...) do { \
	if (!IS_ENABLED(CONFIG_LOG)) { \
		break; \
	} \
	if (IS_ENABLED(CONFIG_LOG_MODE_MINIMAL)) { \
		z_log_minimal_printk(__VA_ARGS__); \
		break; \
	} \
	int _mode; \
	if (0) {\
		z_log_printf_arg_checker(__VA_ARGS__); \
	} \
	Z_LOG_MSG_CREATE(!IS_ENABLED(CONFIG_USERSPACE), _mode, \
			  Z_LOG_LOCAL_DOMAIN_ID, (const void *)(uintptr_t)_is_raw, \
			  LOG_LEVEL_INTERNAL_RAW_STRING, NULL, 0, __VA_ARGS__);\
} while (0)

/** @brief Get index of the log source based on the address of the constant data
 *         associated with the source.
 *
 * @param data Address of the constant data.
 *
 * @return Source ID.
 */
static inline uint32_t log_const_source_id(
				const struct log_source_const_data *data)
{
	return ((const uint8_t *)data - (uint8_t *)TYPE_SECTION_START(log_const))/
			sizeof(struct log_source_const_data);
}

TYPE_SECTION_START_EXTERN(struct log_source_dynamic_data, log_dynamic);
TYPE_SECTION_END_EXTERN(struct log_source_dynamic_data, log_dynamic);

/** @brief Creates name of variable and section for runtime log data.
 *
 *  @param _name Name.
 */
#define LOG_ITEM_DYNAMIC_DATA(_name) UTIL_CAT(log_dynamic_, _name)

#define LOG_INSTANCE_DYNAMIC_DATA(_module_name, _inst) \
	LOG_ITEM_DYNAMIC_DATA(Z_LOG_INSTANCE_FULL_NAME(_module_name, _inst))

/** @brief Get index of the log source based on the address of the dynamic data
 *         associated with the source.
 *
 * @param data Address of the dynamic data.
 *
 * @return Source ID.
 */
static inline uint32_t log_dynamic_source_id(struct log_source_dynamic_data *data)
{
	return ((uint8_t *)data - (uint8_t *)TYPE_SECTION_START(log_dynamic))/
			sizeof(struct log_source_dynamic_data);
}

/** @brief Get index of the log source based on the address of the associated data.
 *
 * @param source Address of the data structure (dynamic if runtime filtering is
 * enabled and static otherwise).
 *
 * @return Source ID.
 */
static inline uint32_t log_source_id(const void *source)
{
	return IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
		log_dynamic_source_id((struct log_source_dynamic_data *)source) :
		log_const_source_id((const struct log_source_const_data *)source);
}

/** @brief Dummy function to trigger log messages arguments type checking. */
static inline __printf_like(1, 2)
void z_log_printf_arg_checker(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
}

/**
 * @brief Write a generic log message.
 *
 * @note This function is intended to be used when porting other log systems.
 *
 * @param level          Log level..
 * @param fmt            String to format.
 * @param ap             Pointer to arguments list.
 */
static inline void log_generic(uint8_t level, const char *fmt, va_list ap)
{
	z_log_msg_runtime_vcreate(Z_LOG_LOCAL_DOMAIN_ID, NULL, level,
				   NULL, 0, 0, fmt, ap);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_CORE_H_ */
