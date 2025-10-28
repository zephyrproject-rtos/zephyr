/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup subsys_tracing_format_apis
 * @brief Header file for tracing format API.
 */

#ifndef ZEPHYR_INCLUDE_TRACING_TRACING_FORMAT_H
#define ZEPHYR_INCLUDE_TRACING_TRACING_FORMAT_H

#include <zephyr/toolchain/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Helpers to format trace messages as strings or raw data
 * @defgroup subsys_tracing_format_apis Tracing format
 * @ingroup subsys_tracing
 * @{
 */

/**
 * @brief A structure to represent tracing data format.
 *
 * This structure represents a piece of data to be emitted through the tracing subsystem.
 * It is typically used with @ref TRACING_FORMAT_DATA and @ref TRACING_DATA to wrap raw values or
 * memory regions in a common format that backends can consume
 */
typedef struct tracing_data {
	/** Pointer to the data buffer to be traced. */
	uint8_t *data;
	/** Size of the data buffer to be traced. */
	uint32_t length;
} __packed tracing_data_t;

/**
 * @brief Macro to trace a message in string format.
 *
 * @param fmt The format string.
 * @param ... The format arguments.
 */
#define TRACING_STRING(fmt, ...)					       \
	do {								       \
		tracing_format_string(fmt, ##__VA_ARGS__);		       \
	} while (false)

/**
 * @brief Macro to format data to tracing data format.
 *
 * @param x Data field.
 */
#define TRACING_FORMAT_DATA(x)						       \
	((struct tracing_data){.data = (uint8_t *)&(x), .length = sizeof((x))})

/**
 * @brief Macro to trace a message in tracing data format.
 *
 * All the parameters should be struct tracing_data.
 */
#define TRACING_DATA(...)						       \
	do {								       \
		struct tracing_data arg[] = {__VA_ARGS__};		       \
									       \
		tracing_format_data(arg, sizeof(arg) /			       \
				    sizeof(struct tracing_data));	       \
	} while (false)

/**
 * @brief Tracing a message in string format.
 *
 * @param str String to format.
 * @param ... Variable length arguments.
 */
void tracing_format_string(const char *str, ...);

/**
 * @brief Tracing a message in raw data format.
 *
 * @param data   Raw data to be traced.
 * @param length Raw data length.
 */
void tracing_format_raw_data(uint8_t *data, uint32_t length);

/**
 * @brief Tracing a message in tracing data format.
 *
 * @param tracing_data_array Tracing_data format data array to be traced.
 * @param count Tracing_data array data count.
 */
void tracing_format_data(tracing_data_t *tracing_data_array, uint32_t count);

/** @} */ /* end of subsys_tracing_format_apis */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_TRACING_TRACING_FORMAT_H */
