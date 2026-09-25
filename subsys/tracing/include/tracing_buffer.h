/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_BUFFER_H
#define _TRACE_BUFFER_H

#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize tracing buffer.
 */
void tracing_buffer_init(void);

/**
 * @brief Get buffer from tracing command buffer.
 *
 * @param data Pointer to tracing command buffer start address.
 *
 * @return Tracing command buffer size (in bytes).
 */
uint32_t tracing_cmd_buffer_alloc(uint8_t **data);

/**
 * @brief Get the tracing ring buffer.
 *
 * @return Pointer to the tracing ring buffer.
 */
struct ring_buf *tracing_buffer_get_ring_buf(void);

#ifdef __cplusplus
}
#endif

#endif
