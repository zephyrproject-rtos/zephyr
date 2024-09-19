/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_MEMORY_H__
#define __OS_WRAPPER_MEMORY_H__

/**
 * @brief  Initialize dynamic memory pool
 */
void rtos_mem_init(void);

/**
 * @brief  Allocate memory from the heap. The buffer value is random
 * @note   The return buffer size/address is cacheline size aligned
 * @param  size: buffer size in byte
 * @retval Pointer to memory the caller can now use
 */
void *rtos_mem_malloc(uint32_t size);

/**
 * @brief  Allocate memory from the heap. The buffer value is zero
 * @note   The return buffer size/address is cacheline size aligned
 * @param  size: buffer size in byte
 * @retval Pointer to memory the caller can now use
 */
void *rtos_mem_zmalloc(uint32_t size);

/**
 * @brief  Allocate memory from the heap. The buffer value is zero
 * @note   The return buffer size/address is cacheline size aligned
 * @param  elementNum:  Number of elements, memory size is elementNum*elementSize
 * @param  elementSize: Size of each array element (in bytes).
 * @retval Pointer to memory the caller can now use
 */
void *rtos_mem_calloc(uint32_t elementNum, uint32_t elementSize);

/**
 * @brief  Reuse or extend memory previously allocated by the malloc(), calloc(), and realloc()
 * functions
 * @note   The return buffer size/address is cacheline size aligned
 * @param  pbuf: Pointer containing the address
 * @param  size: The number of bytes of memory to be newly allocated.
 * @retval Pointer to memory the caller can now use
 */
void *rtos_mem_realloc(void *pbuf, uint32_t size);

/**
 * @brief  Deallocate memory from the heap.
 * @param  pbuf: a pointer to memory previously allocated
 */
void rtos_mem_free(void *pbuf);

/**
 * @brief  Get free heap size.
 * @retval Free heap size in byte
 */
uint32_t rtos_mem_get_free_heap_size(void);

/**
 * @brief  Get minimum ever free heap size.
 * @retval Minimum ever free heap size in byte
 */
uint32_t rtos_mem_get_minimum_ever_free_heap_size(void);

#endif
