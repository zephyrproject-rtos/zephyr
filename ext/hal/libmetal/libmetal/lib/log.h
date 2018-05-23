/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	log.h
 * @brief	Logging support for libmetal.
 */

#ifndef __METAL_METAL_LOG__H__
#define __METAL_METAL_LOG__H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup logging Library Logging Interfaces
 *  @{ */

/** Log message priority levels for libmetal. */
enum metal_log_level {
	METAL_LOG_EMERGENCY,	/**< system is unusable.               */
	METAL_LOG_ALERT,	/**< action must be taken immediately. */
	METAL_LOG_CRITICAL,	/**< critical conditions.              */
	METAL_LOG_ERROR,	/**< error conditions.                 */
	METAL_LOG_WARNING,	/**< warning conditions.               */
	METAL_LOG_NOTICE,	/**< normal but significant condition. */
	METAL_LOG_INFO,		/**< informational messages.           */
	METAL_LOG_DEBUG,	/**< debug-level messages.             */
};

/** Log message handler type. */
typedef void (*metal_log_handler)(enum metal_log_level level,
				  const char *format, ...);

/**
 * @brief	Set libmetal log handler.
 * @param[in]	handler	log message handler.
 * @return	0 on success, or -errno on failure.
 */
extern void metal_set_log_handler(metal_log_handler handler);

/**
 * @brief	Get the current libmetal log handler.
 * @return	Current log handler.
 */
extern metal_log_handler metal_get_log_handler(void);

/**
 * @brief	Set the level for libmetal logging.
 * @param[in]	level	log message level.
 */
extern void metal_set_log_level(enum metal_log_level level);

/**
 * @brief	Get the current level for libmetal logging.
 * @return	Current log level.
 */
extern enum metal_log_level metal_get_log_level(void);

/**
 * @brief	Default libmetal log handler.  This handler prints libmetal log
 *		mesages to stderr.
 * @param[in]	level	log message level.
 * @param[in]	format	log message format string.
 * @return	0 on success, or -errno on failure.
 */
extern void metal_default_log_handler(enum metal_log_level level,
				      const char *format, ...);


/**
 * Emit a log message if the log level permits.
 *
 * @param	level	Log level.
 * @param	...	Format string and arguments.
 */
#define metal_log(level, ...)						       \
	((level <= _metal.common.log_level && _metal.common.log_handler) \
	       ? (void)_metal.common.log_handler(level, __VA_ARGS__)	       \
	       : (void)0)

/** @} */

#ifdef __cplusplus
}
#endif

#include <metal/system/@PROJECT_SYSTEM@/log.h>

#endif /* __METAL_METAL_LOG__H__ */
