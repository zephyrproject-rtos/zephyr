/*
 * Copyright (c) 2017, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_LOG_H__
#define NRFX_LOG_H__

#include <zephyr/logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRFX_MODULE_PREFIX  _CONCAT(NRFX_, NRFX_LOG_MODULE)
/*
 * The following macros from nrfx_config control the log messages coming from
 * a given module:
 * - NRFX_<module>_CONFIG_LOG_ENABLED enables the messages (when set to 1)
 * - NRFX_<module>_CONFIG_LOG_LEVEL specifies the severity level of the messages
 *   that are to be output.
 */
#if !IS_ENABLED(_CONCAT(NRFX_MODULE_PREFIX, _CONFIG_LOG_ENABLED))
#define NRFX_MODULE_CONFIG_LOG_LEVEL 0
#else
#define NRFX_MODULE_CONFIG_LOG_LEVEL \
	_CONCAT(NRFX_MODULE_PREFIX, _CONFIG_LOG_LEVEL)
#endif

#if	NRFX_MODULE_CONFIG_LOG_LEVEL == 0
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_NONE
#elif	NRFX_MODULE_CONFIG_LOG_LEVEL == 1
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_ERR
#elif	NRFX_MODULE_CONFIG_LOG_LEVEL == 2
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_WRN
#elif	NRFX_MODULE_CONFIG_LOG_LEVEL == 3
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_INF
#elif	NRFX_MODULE_CONFIG_LOG_LEVEL == 4
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_DBG
#endif
LOG_MODULE_REGISTER(NRFX_MODULE_PREFIX, NRFX_MODULE_LOG_LEVEL);

/**
 * @defgroup nrfx_log nrfx_log.h
 * @{
 * @ingroup nrfx
 *
 * @brief This file contains macros that should be implemented according to
 *        the needs of the host environment into which @em nrfx is integrated.
 */

/**
 * @brief Macro for logging a message with the severity level ERROR.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_ERROR(...)  LOG_ERR(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level WARNING.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_WARNING(...)  LOG_WRN(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level INFO.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_INFO(...)  LOG_INF(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level DEBUG.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_DEBUG(...)  LOG_DBG(__VA_ARGS__)

/**
 * @brief Macro for logging a memory dump with the severity level ERROR.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_ERROR(p_memory, length) \
	LOG_HEXDUMP_ERR(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level WARNING.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_WARNING(p_memory, length) \
	LOG_HEXDUMP_WRN(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level INFO.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_INFO(p_memory, length) \
	LOG_HEXDUMP_INF(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level DEBUG.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_DEBUG(p_memory, length) \
	LOG_HEXDUMP_DBG(p_memory, length, "")

/**
 * @brief Macro for getting the textual representation of a given error code.
 *
 * @param[in] error_code Error code.
 *
 * @return String containing the textual representation of the error code.
 */
#define NRFX_LOG_ERROR_STRING_GET(error_code)  nrfx_error_string_get(error_code)
extern char const *nrfx_error_string_get(nrfx_err_t code);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_LOG_H__
