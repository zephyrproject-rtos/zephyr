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
	uint8_t level;
#ifdef CONFIG_NIOS2
	/* Workaround alert! Dummy data to ensure that structure is >8 bytes.
	 * Nios2 uses global pointer register for structures <=8 bytes and
	 * apparently does not handle well variables placed in custom sections.
	 */
	uint32_t dummy;
#endif
};

/** @brief Dynamic data associated with the source of log messages. */
struct log_source_dynamic_data {
	uint32_t filters;
#ifdef CONFIG_NIOS2
	/* Workaround alert! Dummy data to ensure that structure is >8 bytes.
	 * Nios2 uses global pointer register for structures <=8 bytes and
	 * apparently does not handle well variables placed in custom sections.
	 */
	uint32_t dummy[2];
#endif
#if defined(CONFIG_RISCV) && defined(CONFIG_64BIT)
	/* Workaround: RV64 needs to ensure that structure is just 8 bytes. */
	uint32_t dummy;
#endif
};

/** @internal
 *
 * Creates name of variable and section for constant log data.
 *
 *  @param _name Name.
 */
#define Z_LOG_ITEM_CONST_DATA(_name) UTIL_CAT(log_const_, _name)

/** @internal
 *
 * Create static logging instance in read only memory.
 *
 * @param _name name of the module. With added prefix forms name of variable and
 * memory section.
 *
 * @param _str_name Name of the module that will be used when message is formatted.
 *
 * @param _level Messages up to this level are compiled in.
 */
#define Z_LOG_CONST_ITEM_REGISTER(_name, _str_name, _level)		       \
	const struct log_source_const_data Z_LOG_ITEM_CONST_DATA(_name)	       \
	__attribute__ ((section("." STRINGIFY(Z_LOG_ITEM_CONST_DATA(_name))))) \
	__attribute__((used)) = {					       \
		.name = _str_name,					       \
		.level  = (_level),					       \
	}

/** @brief Initialize pointer to logger instance with explicitly provided object.
 *
 * Macro can be used to initialized a pointer with object that is not unique to
 * the given instance, thus not created with @ref LOG_INSTANCE_REGISTER.
 *
 * @param _name Name of the structure element for holding logging object.
 * @param _object Pointer to a logging instance object.
 */
#define LOG_OBJECT_PTR_INIT(_name, _object) \
	IF_ENABLED(CONFIG_LOG, (._name = _object,))

/** @internal
 *
 * Create a name for which contains module and instance names.
 */
#define Z_LOG_INSTANCE_FULL_NAME(_module_name, _inst_name) \
	UTIL_CAT(_module_name, UTIL_CAT(_, _inst_name))

/** @internal
 *
 * Returns a pointer associated with given logging instance. When runtime filtering
 * is enabled then dynamic instance is returned.
 *
 * @param _name Name of the instance.
 *
 * @return Pointer to the instance object (static or dynamic).
 */
#define Z_LOG_OBJECT_PTR(_name) \
		COND_CODE_1(CONFIG_LOG_RUNTIME_FILTERING, \
			(&LOG_ITEM_DYNAMIC_DATA(_name)), \
			(&Z_LOG_ITEM_CONST_DATA(_name))) \

/** @brief Get pointer to a logging instance.
 *
 * Instance is identified by @p _module_name and @p _inst_name.
 *
 * @param _module_name Module name.
 * @param _inst_name Instance name.
 *
 * @return Pointer to a logging instance.
 */
#define LOG_INSTANCE_PTR(_module_name, _inst_name) \
	Z_LOG_OBJECT_PTR(Z_LOG_INSTANCE_FULL_NAME(_module_name, _inst_name))

/** @brief Macro for initializing a pointer to the logger instance.
 *
 * @p _module_name and @p _inst_name are concatenated to form a name of the object.
 *
 * Macro is intended to be used in user structure initializer to initialize a field
 * in the structure that holds pointer to the logging instance. Structure field
 * should be declared using @p LOG_INSTANCE_PTR_DECLARE.
 *
 * @param _name Name of a structure element that have a pointer to logging instance object.
 * @param _module_name Module name.
 * @param _inst_name Instance name.
 */
#define LOG_INSTANCE_PTR_INIT(_name, _module_name, _inst_name)	   \
	LOG_OBJECT_PTR_INIT(_name, LOG_INSTANCE_PTR(_module_name, _inst_name))

#define Z_LOG_INSTANCE_STRUCT \
	COND_CODE_1(CONFIG_LOG_RUNTIME_FILTERING, \
		    (struct log_source_dynamic_data), \
		    (const struct log_source_const_data))

/**
 * @brief Declare a logger instance pointer in the module structure.
 *
 * @param _name Name of a structure element that will have a pointer to logging
 * instance object.
 */
#define LOG_INSTANCE_PTR_DECLARE(_name)	\
	IF_ENABLED(CONFIG_LOG, (Z_LOG_INSTANCE_STRUCT * _name))

#define Z_LOG_RUNTIME_INSTANCE_REGISTER(_module_name, _inst_name) \
	struct log_source_dynamic_data LOG_INSTANCE_DYNAMIC_DATA(_module_name, _inst_name) \
		__attribute__ ((section("." STRINGIFY( \
				LOG_INSTANCE_DYNAMIC_DATA(_module_name, _inst_name) \
				) \
		))) __attribute__((used))

#define Z_LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level) \
	Z_LOG_CONST_ITEM_REGISTER( \
		Z_LOG_INSTANCE_FULL_NAME(_module_name, _inst_name), \
		STRINGIFY(_module_name._inst_name), \
		_level); \
	IF_ENABLED(CONFIG_LOG_RUNTIME_FILTERING, \
		   (Z_LOG_RUNTIME_INSTANCE_REGISTER(_module_name, _inst_name)))

/**
 * @brief Macro for registering instance for logging with independent filtering.
 *
 * Module instance provides filtering of logs on instance level instead of
 * module level. Instance create using this macro can later on be used with
 * @ref LOG_INSTANCE_PTR_INIT or referenced by @ref LOG_INSTANCE_PTR.
 *
 * @param _module_name Module name.
 * @param _inst_name Instance name.
 * @param _level Initial static filtering.
 */
#define LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level) \
	IF_ENABLED(CONFIG_LOG, (Z_LOG_INSTANCE_REGISTER(_module_name, _inst_name, _level)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_INSTANCE_H_ */
