/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_HEAP_HEAP_CUSTOM_HEADER_H_
#define ZEPHYR_LIB_HEAP_HEAP_CUSTOM_HEADER_H_

/**
 * @file
 * @brief Internal heap custom header and per-thread statistics
 *
 * This file contains internal functions for managing custom heap chunk headers
 * and per-thread allocation statistics. These functions are not part of the
 * public API and should only be used within the heap subsystem implementation.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/util.h>
#include <string.h>

#include "heap.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set custom header for a heap chunk
 *
 * Stores custom header information in the chunk's metadata area. This function
 * validates the heap pointer, chunk ID, and custom header pointer before
 * performing the operation.
 *
 * @param h Pointer to the heap structure
 * @param chunk_id Chunk identifier
 * @param custom_hdr Pointer to custom header data to store
 */
void z_heap_set_custom_header(struct z_heap *h, chunkid_t chunk_id,
				   const struct z_heap_custom_header *custom_hdr);

/**
 * @brief Get thread identifier and update allocation statistics
 *
 * Obtains a per-thread identifier on first allocation and updates the
 * allocation statistics for the current thread. If this is the first
 * allocation for the thread, a new thread ID is assigned and the thread
 * name is recorded.
 *
 * @param heap Pointer to the sys_heap instance
 * @param actual_allocated_chunk_sz Size of allocated chunk in CHUNK_UNIT units
 *
 * @retval UINT32_MAX On error or if statistics are disabled
 * @retval 0..CONFIG_SYS_HEAP_STATS_MAX_THREADS-1 Thread identifier
 */
uint32_t z_heap_get_tid_and_update_stats_add(struct sys_heap *heap,
						  chunksz_t actual_allocated_chunk_sz);

/**
 * @brief Update allocation statistics on deallocation
 *
 * Subtracts the size of the freed chunk from the per-thread allocation
 * statistics. The thread ID is retrieved from the chunk's custom header.
 *
 * @param heap Pointer to the sys_heap instance
 * @param c Chunk identifier of the freed chunk
 */
void z_heap_update_stats_sub(struct sys_heap *heap, chunkid_t c);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_HEAP_HEAP_CUSTOM_HEADER_H_ */
