/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SMTC_MODEM_HAL_DBG_TRACE_H__
#define __SMTC_MODEM_HAL_DBG_TRACE_H__

/**
 * @brief This file is provided *to* the LoRa Basics Modem source code as a
 * HAL implementation of its logging API.
 * It also provides some stuff like MODEM_HAL_FEATURE_ON.
 */

#include <stdio.h>

#include <zephyr/logging/log.h>

/* FIXME: Only required because other sources are missing this include… */
#include "smtc_modem_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SMTC_MODEM_HAL_DBG_TRACE_C
LOG_MODULE_DECLARE(lorawan, CONFIG_LORA_BASICS_MODEM_LOG_LEVEL);
#endif

#define MODEM_HAL_FEATURE_OFF (0)
#define MODEM_HAL_FEATURE_ON  (!MODEM_HAL_FEATURE_OFF)

/**
 * @brief Trims the end of a string
 *
 * This is used to trim Semtech's logging strings that contain trailing
 * spaces and endlines.
 * The trimming is done by writing a null character when the whitespaces start.
 *
 * @param[in] text The text to trim
 */
void smtc_str_trim_end(char *text);

/* FIXME: modem_utilities/circularfs.c will be patched to reduce this size */
#define SMTC_PRINT_BUFFER_SIZE 220

/**
 * @brief This macro matches the Semtech LBM logging macros with the logging
 * macros LOG_* from zephyr, while trimming the endlines present in LBM.
 */
#define SMTC_LOG(log_fn, ...)                                                                      \
	do {                                                                                       \
		char __log_buffer[SMTC_PRINT_BUFFER_SIZE];                                         \
		snprintf(__log_buffer, SMTC_PRINT_BUFFER_SIZE, __VA_ARGS__);                       \
		smtc_str_trim_end(__log_buffer);                                                   \
		log_fn("%s", __log_buffer);                                                        \
	} while (0)

#define MODEM_HAL_DBG_TRACE MODEM_HAL_FEATURE_ON

#define SMTC_MODEM_HAL_TRACE_MSG(msg)                   SMTC_LOG(LOG_INF, "%s", msg);
#define SMTC_MODEM_HAL_TRACE_PRINTF(...)                SMTC_LOG(LOG_INF, __VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_INFO(...)                  SMTC_LOG(LOG_INF, __VA_ARGS__)
#define SMTC_MODEM_HAL_TRACE_WARNING(...)               SMTC_LOG(LOG_WRN, __VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_ERROR(...)                 SMTC_LOG(LOG_ERR, __VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_ARRAY(msg, array, len)     LOG_HEXDUMP_INF(array, len, msg);
#define SMTC_MODEM_HAL_TRACE_PACKARRAY(msg, array, len) LOG_HEXDUMP_INF(array, len, msg);

#if CONFIG_LORA_BASICS_MODEM_LOG_VERBOSE

#define MODEM_HAL_DEEP_DBG_TRACE MODEM_HAL_FEATURE_ON

/* Deep debug trace default definitions*/
#define SMTC_MODEM_HAL_TRACE_PRINTF_DEBUG(...)                SMTC_LOG(LOG_DBG, __VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_MSG_DEBUG(msg)                   SMTC_LOG(LOG_DBG, "%s", msg);
#define SMTC_MODEM_HAL_TRACE_MSG_COLOR_DEBUG(msg, color)      SMTC_LOG(LOG_DBG, "%s", msg);
#define SMTC_MODEM_HAL_TRACE_INFO_DEBUG(...)                  SMTC_LOG(LOG_INF, __VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_WARNING_DEBUG(...)               SMTC_LOG(LOG_WRN, __VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_ERROR_DEBUG(...)                 SMTC_LOG(LOG_ERR, __VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_ARRAY_DEBUG(msg, array, len)     LOG_HEXDUMP_DBG(array, len, msg);
#define SMTC_MODEM_HAL_TRACE_PACKARRAY_DEBUG(msg, array, len) LOG_HEXDUMP_DBG(array, len, msg);

#else /* CONFIG_LORA_BASICS_MODEM_LOG_VERBOSE */

/* Deep debug trace default definitions*/
#define SMTC_MODEM_HAL_TRACE_PRINTF_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_MSG_DEBUG(msg)
#define SMTC_MODEM_HAL_TRACE_MSG_COLOR_DEBUG(msg, color)
#define SMTC_MODEM_HAL_TRACE_INFO_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_WARNING_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_ERROR_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_ARRAY_DEBUG(msg, array, len)
#define SMTC_MODEM_HAL_TRACE_PACKARRAY_DEBUG(...)

#endif /* CONFIG_LORA_BASICS_MODEM_LOG_VERBOSE */

#if CONFIG_LORA_BASICS_MODEM_RADIO_PLANNER_LOG_VERBOSE

#define SMTC_MODEM_HAL_RP_TRACE_MSG(msg)    SMTC_MODEM_HAL_TRACE_PRINTF(msg)
#define SMTC_MODEM_HAL_RP_TRACE_PRINTF(...) SMTC_MODEM_HAL_TRACE_PRINTF(__VA_ARGS__)

#else /* CONFIG_LORA_BASICS_MODEM_RADIO_PLANNER_LOG_VERBOSE */

#define SMTC_MODEM_HAL_RP_TRACE_MSG(msg)
#define SMTC_MODEM_HAL_RP_TRACE_PRINTF(...)

#endif /* CONFIG_LORA_BASICS_MODEM_RADIO_PLANNER_LOG_VERBOSE */

#if CONFIG_LORA_BASICS_MODEM_GEOLOCATION_LOG_VERBOSE
#define GNSS_ALMANAC_DEEP_DBG_TRACE MODEM_HAL_FEATURE_ON
#define GNSS_SCAN_DEEP_DBG_TRACE    MODEM_HAL_FEATURE_ON
#define GNSS_SEND_DEEP_DBG_TRACE    MODEM_HAL_FEATURE_ON
#define WIFI_SCAN_DEEP_DBG_TRACE    MODEM_HAL_FEATURE_ON
#define WIFI_SEND_DEEP_DBG_TRACE    MODEM_HAL_FEATURE_ON
#endif /* CONFIG_LORA_BASICS_MODEM_GEOLOCATION_LOG_VERBOSE */

#ifdef __cplusplus
}
#endif

#endif /* __SMTC_MODEM_HAL_DBG_TRACE_H__*/
