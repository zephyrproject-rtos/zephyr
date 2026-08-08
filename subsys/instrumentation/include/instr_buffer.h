/*
 * Copyright (c) 2025 Antmicro
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
 * @brief Initialize instrumentation buffer.
 */
void instr_buffer_init(void);

/**
 * @brief Get the instrumentation ring buffer.
 *
 * @return Pointer to the instrumentation ring buffer.
 */
struct ring_buf *instr_buffer_get_ring_buf(void);

#ifdef __cplusplus
}
#endif

#endif
