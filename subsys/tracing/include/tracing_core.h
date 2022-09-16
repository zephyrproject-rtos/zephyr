/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_CORE_H
#define _TRACE_CORE_H

#include <zephyr/irq.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRACING_LOCK()		{ int key; key = irq_lock()

#define TRACING_UNLOCK()	{ irq_unlock(key); } }

/**
 * @brief Check tracing enabled or not.
 *
 * @return True if tracing enabled; False if tracing disabled.
 */
bool is_tracing_enabled(void);

/**
 * @brief Give tracing buffer to backend.
 *
 * @param data Tracing buffer address.
 * @param length Tracing buffer length.
 */
void tracing_buffer_handle(uint8_t *data, uint32_t length);

/**
 * @brief Handle tracing packet drop.
 */
void tracing_packet_drop_handle(void);

/**
 * @brief Handle tracing command.
 *
 * @param buf Tracing command buffer address.
 * @param length Tracing command buffer length.
 */
void tracing_cmd_handle(uint8_t *buf, uint32_t length);

/**
 * @brief Trigger tracing thread to run after every first put.
 *
 * @param before_put_is_empty If tracing buffer is empty before this put.
 */
void tracing_trigger_output(bool before_put_is_empty);

/**
 * @brief Check if we are in tracing thread context.
 *
 * @return True if in tracing thread context; False if not.
 */
bool is_tracing_thread(void);

#ifdef __cplusplus
}
#endif

#endif
