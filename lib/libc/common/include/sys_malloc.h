/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr common libc malloc extensions
 */

#ifndef ZEPHYR_LIB_LIBC_COMMON_INCLUDE_SYS_MALLOC_H_
#define ZEPHYR_LIB_LIBC_COMMON_INCLUDE_SYS_MALLOC_H_

#include <zephyr/sys/sys_heap.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the runtime statistics of the malloc heap
 *
 * @kconfig_dep{CONFIG_SYS_HEAP_RUNTIME_STATS}
 *
 * @param stats Pointer to struct to copy statistics into
 * @return -EINVAL if null pointers, otherwise 0
 */
int malloc_runtime_stats_get(struct sys_memory_stats *stats);

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_COMMON_INCLUDE_SYS_MALLOC_H_ */
