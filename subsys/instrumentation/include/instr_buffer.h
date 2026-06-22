/*
 * Copyright (c) 2025 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_BUFFER_H
#define _TRACE_BUFFER_H

#include <stdbool.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize instrumentation buffer.
 */
void instr_buffer_init(void);

/**
 * @brief Instrumentation buffer is empty or not.
 *
 * @return true if the ring buffer is empty, or false if not.
 */
bool instr_buffer_is_empty(void);

/**
 * @brief Get free space in the instrumentation buffer.
 *
 * @return Instrumentation buffer free space (in bytes).
 */
uint32_t instr_buffer_space_get(void);

/**
 * @brief Get instrumentation buffer capacity (max size).
 *
 * @return Instrumentation buffer capacity (in bytes).
 */
uint32_t instr_buffer_capacity_get(void);

/**
 * @brief Try to allocate buffer in the instrumentation buffer.
 *
 * @param data Pointer to the address. It's set to a location
 *             within the instrumentation buffer.
 * @param size Requested buffer size (in bytes).
 *
 * @return Size of allocated buffer which can be smaller than
 *         requested if there isn't enough free space or buffer wraps.
 */
uint32_t instr_buffer_put_claim(uint8_t **data, uint32_t size);

/**
 * @brief Indicate number of bytes written to the allocated buffer.
 *
 * @param size Number of bytes written to the allocated buffer.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Given @a size exceeds free space of instrumentation buffer.
 */
int instr_buffer_put_finish(uint32_t size);

/**
 * @brief Write data to instrumentation buffer.
 *
 * @param data Address of data.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written to instrumentation buffer.
 */
uint32_t instr_buffer_put(uint8_t *data, uint32_t size);

/**
 * @brief Get address of the first valid data in instrumentation buffer.
 *
 * @param data Pointer to the address. It's set to a location pointing to
 *             the first valid data within the instrumentation buffer.
 * @param size Requested buffer size (in bytes).
 *
 * @return Size of valid buffer which can be smaller than requested
 *         if there isn't enough valid data or buffer wraps.
 */
uint32_t instr_buffer_get_claim(uint8_t **data, uint32_t size);

/**
 * @brief Indicate number of bytes read from claimed buffer.
 *
 * @param size Number of bytes read from claimed buffer.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Given @a size exceeds available data of instrumentation buffer.
 */
int instr_buffer_get_finish(uint32_t size);

/**
 * @brief Read data from instrumentation buffer to output buffer.
 *
 * @param data Address of the output buffer.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written to the output buffer.
 */
uint32_t instr_buffer_get(uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
