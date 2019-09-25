/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_INSTANCE_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_INSTANCE_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Constant data associated with the source of log messages. */
struct log_source_const_data {
	const char *name;
	u8_t level;
#ifdef CONFIG_NIOS2
	/* Workaround alert! Dummy data to ensure that structure is >8 bytes.
	 * Nios2 uses global pointer register for structures <=8 bytes and
	 * apparently does not handle well variables placed in custom sections.
	 */
	u32_t dummy;
#endif
};

/** @brief Dynamic data associated with the source of log messages. */
struct log_source_dynamic_data {
	u32_t filters;
#ifdef CONFIG_NIOS2
	/* Workaround alert! Dummy data to ensure that structure is >8 bytes.
	 * Nios2 uses global pointer register for structures <=8 bytes and
	 * apparently does not handle well variables placed in custom sections.
	 */
	u32_t dummy[2];
#endif
};

/** @brief Creates name of variable and section for constant log data.
 *
 *  @param _name Name.
 */
#define LOG_ITEM_CONST_DATA(_name) UTIL_CAT(log_const_, _name)

#define Z_LOG_CONST_ITEM_REGISTER(_name, _str_name, _level)		     \
	const struct log_source_const_data LOG_ITEM_CONST_DATA(_name)	     \
	__attribute__ ((section("." STRINGIFY(LOG_ITEM_CONST_DATA(_name))))) \
	__attribute__((used)) = {					     \
		.name = _str_name,					     \
		.level  = (_level),					     \
	}

/** @def LOG_INSTANCE_PTR_DECLARE
 * @brief Macro for declaring a logger instance pointer in the module structure.
 */

/** @def LOG_INSTANCE_REGISTER
 * @brief Macro for registering instance for logging with independent filtering.
 *
 * Module instance provides filtering of logs on instance level instead of
 * module level.
 */

/** @def LOG_INSTANCE_PTR_INIT
 * @brief Macro for initializing a pointer to the logger instance.
 */

/** @} */

#ifdef CONFIG_LOG

#define LOG_INSTANCE_FULL_NAME(_module_name, _inst_name) \
	UTIL_CAT(_module_name, UTIL_CAT(_, _inst_name))

#if defined(CONFIG_LOG_RUNTIME_FILTERING)
#define LOG_INSTANCE_PTR_DECLARE(_name)	\
	struct log_source_dynamic_data *_name

#define LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level)		   \
	Z_LOG_CONST_ITEM_REGISTER(					   \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name),	   \
		STRINGIFY(_module_name._inst_name),			   \
		_level);						   \
	struct log_source_dynamic_data LOG_INSTANCE_DYNAMIC_DATA(	   \
						_module_name, _inst_name)  \
		__attribute__ ((section("." STRINGIFY(			   \
				LOG_INSTANCE_DYNAMIC_DATA(_module_name,	   \
						       _inst_name)	   \
				)					   \
		))) __attribute__((used))

#define LOG_INSTANCE_PTR_INIT(_name, _module_name, _inst_name)	   \
	._name = &LOG_ITEM_DYNAMIC_DATA(			   \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name)),

#else /* CONFIG_LOG_RUNTIME_FILTERING */
#define LOG_INSTANCE_PTR_DECLARE(_name)	\
	const struct log_source_const_data *_name

#define LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level)	  \
	Z_LOG_CONST_ITEM_REGISTER(				  \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name), \
		STRINGIFY(_module_name._inst_name),		  \
		_level)


#define LOG_INSTANCE_PTR_INIT(_name, _module_name, _inst_name) \
	._name = &LOG_ITEM_CONST_DATA(			       \
		LOG_INSTANCE_FULL_NAME(_module_name, _inst_name)),

#endif /* CONFIG_LOG_RUNTIME_FILTERING */
#else /* CONFIG_LOG */
#define LOG_INSTANCE_PTR_DECLARE(_name) /* empty */
#define LOG_INSTANCE_REGISTER(_module_name, _inst_name,  _level)    /* empty */
#define LOG_INSTANCE_PTR_INIT(_name, _module_name, _inst_name)      /* empty */
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_INSTANCE_H_ */
