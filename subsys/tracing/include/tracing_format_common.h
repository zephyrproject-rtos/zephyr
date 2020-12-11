/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_FORMAT_COMMON_H
#define _TRACE_FORMAT_COMMON_H

#include <stdarg.h>
#include <sys/printk.h>
#include <tracing/tracing_format.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A structure to represent tracing format context.
 */
typedef struct {
	int status;
	uint32_t length;
} tracing_ctx_t;

/**
 * @brief Put string format tracing message to tracing buffer.
 *
 * @param str   String to format.
 * @param args  Variable parameters.
 *
 * @return true if put tracing message to tracing buffer successfully.
 */
bool tracing_format_string_put(const char *str, va_list args);

/**
 * @brief Put raw data format tracing message to tracing buffer.
 *
 * @param data   Raw data to be traced.
 * @param length Raw data length.
 *
 * @return true if put tracing message to tracing buffer successfully.
 */
bool tracing_format_raw_data_put(uint8_t *data, uint32_t size);

/**
 * @brief Put tracing_data format message to tracing buffer.
 *
 * @param tracing_data_array Tracing_data format data array to be traced.
 * @param count Tracing_data array data count.
 *
 * @return true if put tracing message to tracing buffer successfully.
 */
bool tracing_format_data_put(tracing_data_t *tracing_data_array, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif
