/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief MCTP memory management functions
 */

#ifndef ZEPHYR_MCTP_MEMORY_H_
#define ZEPHYR_MCTP_MEMORY_H_

/**
 * Allocate memory from the MCTP heap.
 *
 * This function allocates memory from the MCTP heap. The memory returned is suitable
 * to be sent to libmctp function that take ownership of the memory. The allocated memory
 * should be freed using mctp_heap_free() when it is no longer needed.
 *
 * @param bytes Number of bytes to allocate.
 *
 * @return Pointer to the allocated memory, or NULL if allocation failed.
 */
void *mctp_heap_alloc(size_t bytes);

/**
 * Free memory allocated from the MCTP heap.
 *
 * This function frees memory that was previously allocated from the MCTP heap using
 * mctp_heap_alloc(). The pointer passed to this function must have been returned by
 * mctp_heap_alloc() and must not have been freed already.
 *
 * @param ptr Pointer to the memory to free.
 */
void mctp_heap_free(void *ptr);

#endif /* ZEPHYR_MCTP_MEMORY_H_ */
