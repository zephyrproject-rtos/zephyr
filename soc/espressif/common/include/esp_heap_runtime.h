/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Allocate memory from the esp_heap_runtime.
 *
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void *esp_heap_runtime_malloc(size_t size);

/**
 * @brief Allocate memory from esp_heap_runtime, array style
 *
 * @param n Number of elements in the requested array
 * @param size Size of each array element (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
void *esp_heap_runtime_calloc(size_t n, size_t size);

/**
 * @brief Reallocate memory from a esp_heap_runtime
 *
 * @param ptr Original pointer returned from a previous allocation
 * @param bytes Desired size of block to allocate
 *
 * @return Pointer to memory the caller can now use, or NULL
 */
void *esp_heap_runtime_realloc(void *ptr, size_t bytes);

/**
 * @brief Free memory allocated from esp_heap_runtime.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 */
void esp_heap_runtime_free(void *mem);
