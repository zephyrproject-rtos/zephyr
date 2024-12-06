/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_MEMORY_H__
#define __OS_WRAPPER_MEMORY_H__

/**
 * @brief  Allocate memory from the heap. For FreeRTOS, map to pvPortMalloc
 *         The buffer value is random
 * @note   The return buffer size/address is cacheline size aligned
 * @param  size: buffer size in byte
 * @retval
 */
void *rtos_mem_malloc(uint32_t size);

/**
 * @brief  Allocate memory from the heap. For FreeRTOS, map to pvPortMalloc
 *         The buffer value is zero
 * @note   The return buffer size/address is cacheline size aligned
 * @param  size: buffer size in byte
 * @retval
 */
void *rtos_mem_zmalloc(uint32_t size);

/**
 * @brief  For FreeRTOS, map to vPortFree
 * @param  pbuf:
 * @param  size: Optional parameters, the default value is 0. This parameter currently has no
 * effect.
 */
void rtos_mem_free(void *pbuf);

#endif
