/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SMTC_MODEM_HAL_DBG_TRACE_H__
#define __SMTC_MODEM_HAL_DBG_TRACE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>

/* FIXME: Only required because other sources are missing this include… */
#include "smtc_modem_hal.h"

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

/* clang-format off */
#define MODEM_HAL_FEATURE_OFF                             0
#define MODEM_HAL_FEATURE_ON                              !MODEM_HAL_FEATURE_OFF

/* Sensible default values. Change in Makefile if needed */
#ifndef MODEM_HAL_DBG_TRACE
#define MODEM_HAL_DBG_TRACE                               MODEM_HAL_FEATURE_ON
#endif

#ifndef MODEM_HAL_DBG_TRACE_COLOR
#define MODEM_HAL_DBG_TRACE_COLOR                         MODEM_HAL_FEATURE_ON
#endif

#ifndef MODEM_HAL_DBG_TRACE_RP
#define MODEM_HAL_DBG_TRACE_RP                            MODEM_HAL_FEATURE_OFF
#endif

#ifndef MODEM_HAL_DEEP_DBG_TRACE
#define MODEM_HAL_DEEP_DBG_TRACE                          MODEM_HAL_FEATURE_OFF
#endif
/* clang-format on */

#if (MODEM_HAL_DBG_TRACE)

#define SMTC_MODEM_HAL_TRACE_PRINTF(...)  smtc_modem_hal_print_trace(__VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_MSG(msg)     smtc_modem_hal_print_trace("%s", msg);
#define SMTC_MODEM_HAL_TRACE_INFO(...)    smtc_modem_hal_print_trace_inf(__VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_WARNING(...) smtc_modem_hal_print_trace_wrn(__VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_ERROR(...)   smtc_modem_hal_print_trace_err(__VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_ARRAY(msg, array, len)                                                \
	smtc_modem_hal_print_trace_array_inf(msg, array, len);
#define SMTC_MODEM_HAL_TRACE_PACKARRAY(msg, array, len) SMTC_MODEM_HAL_TRACE_ARRAY(msg, array, len);

#if (MODEM_HAL_DEEP_DBG_TRACE)
/* Deep debug trace default definitions*/
#define SMTC_MODEM_HAL_TRACE_PRINTF_DEBUG(...)      smtc_modem_hal_print_trace_dbg(__VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_MSG_DEBUG(msg)         smtc_modem_hal_print_trace_dbg("%s", msg);
#define SMTC_MODEM_HAL_TRACE_MSG_COLOR_DEBUG(msg, color) smtc_modem_hal_print_trace_dbg("%s", msg);
#define SMTC_MODEM_HAL_TRACE_INFO_DEBUG(...)        smtc_modem_hal_print_trace_dbg(__VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_WARNING_DEBUG(...)     smtc_modem_hal_print_trace_dbg(__VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_ERROR_DEBUG(...)       smtc_modem_hal_print_trace_dbg(__VA_ARGS__);
#define SMTC_MODEM_HAL_TRACE_ARRAY_DEBUG(msg, array, len)                                          \
	smtc_modem_hal_print_trace_array_dbg(msg, array, len);
#define SMTC_MODEM_HAL_TRACE_PACKARRAY_DEBUG(...) SMTC_MODEM_HAL_TRACE_ARRAY_DEBUG(__VA_ARGS__)

#else
/* Deep debug trace default definitions*/
#define SMTC_MODEM_HAL_TRACE_PRINTF_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_MSG_DEBUG(msg)
#define SMTC_MODEM_HAL_TRACE_MSG_COLOR_DEBUG(msg, color)
#define SMTC_MODEM_HAL_TRACE_INFO_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_WARNING_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_ERROR_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_ARRAY_DEBUG(msg, array, len)
#define SMTC_MODEM_HAL_TRACE_PACKARRAY_DEBUG(...)

#endif /* MODEM_HAL_DEEP_DBG_TRACE */

#else /*  MODEM_HAL_DBG_TRACE */

/* Trace default definitions*/
#define SMTC_MODEM_HAL_TRACE_PRINTF(...)
#define SMTC_MODEM_HAL_TRACE_MSG(msg)
#define SMTC_MODEM_HAL_TRACE_MSG_COLOR(msg, color)
#define SMTC_MODEM_HAL_TRACE_INFO(...)
#define SMTC_MODEM_HAL_TRACE_WARNING(...)
#define SMTC_MODEM_HAL_TRACE_ERROR(...)
#define SMTC_MODEM_HAL_TRACE_ARRAY(msg, array, len)
#define SMTC_MODEM_HAL_TRACE_PACKARRAY(...)

/* Deep debug trace default definitions*/
#define SMTC_MODEM_HAL_TRACE_PRINTF_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_MSG_DEBUG(msg)
#define SMTC_MODEM_HAL_TRACE_MSG_COLOR_DEBUG(msg, color)
#define SMTC_MODEM_HAL_TRACE_INFO_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_WARNING_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_ERROR_DEBUG(...)
#define SMTC_MODEM_HAL_TRACE_ARRAY_DEBUG(msg, array, len)
#define SMTC_MODEM_HAL_TRACE_PACKARRAY_DEBUG(...)

#endif /* MODEM_HAL_DBG_TRACE */

#if (MODEM_HAL_DBG_TRACE_RP == MODEM_HAL_FEATURE_ON)

#define SMTC_MODEM_HAL_RP_TRACE_MSG(msg)    SMTC_MODEM_HAL_TRACE_PRINTF(msg)
#define SMTC_MODEM_HAL_RP_TRACE_PRINTF(...) SMTC_MODEM_HAL_TRACE_PRINTF(__VA_ARGS__)

#else /* MODEM_HAL_DBG_TRACE */

#define SMTC_MODEM_HAL_RP_TRACE_MSG(msg)
#define SMTC_MODEM_HAL_RP_TRACE_PRINTF(...)

#endif /* MODEM_HAL_DBG_TRACE */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

void smtc_modem_hal_print_trace(const char *fmt, ...);
void smtc_modem_hal_print_trace_inf(const char *fmt, ...);
void smtc_modem_hal_print_trace_dbg(const char *fmt, ...);
void smtc_modem_hal_print_trace_wrn(const char *fmt, ...);
void smtc_modem_hal_print_trace_err(const char *fmt, ...);
void smtc_modem_hal_print_trace_array_inf(char *msg, uint8_t *array, uint32_t len);
void smtc_modem_hal_print_trace_array_dbg(char *msg, uint8_t *array, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __SMTC_MODEM_HAL_DBG_TRACE_H__*/
