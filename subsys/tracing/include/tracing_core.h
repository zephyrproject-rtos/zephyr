/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_CORE_H
#define _TRACE_CORE_H

#include <zephyr/spinlock.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Serialize access to the single tracing ring buffer across producers. A
 * spinlock (rather than a bare irq_lock) is required so concurrent producers on
 * different CPUs are serialized on SMP; on uniprocessor it degrades to the same
 * interrupt-lock with no added cost.
 */
extern struct k_spinlock tracing_lock;

#define TRACING_LOCK()		{ k_spinlock_key_t key; key = k_spin_lock(&tracing_lock)

#define TRACING_UNLOCK()	{ k_spin_unlock(&tracing_lock, key); } }

/**
 * @brief Check tracing enabled or not.
 *
 * @return True if tracing enabled; False if tracing disabled.
 */
bool is_tracing_enabled(void);

/**
 * @brief Enable or disable emission of tracing data at runtime.
 *
 * @param enable True to enable tracing, false to disable it.
 */
void tracing_set_enabled(bool enable);

/**
 * @brief Get the number of tracing packets dropped since boot.
 *
 * Packets are dropped when the tracing buffer is full at the time of a put.
 *
 * @return Cumulative dropped-packet count.
 */
uint32_t tracing_packet_drop_count_get(void);

/**
 * @brief Flush every registered tracing backend.
 *
 * @retval 0 on success.
 * @retval -errno first backend-reported failure (remaining backends still flushed).
 */
int tracing_backends_flush(void);

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
