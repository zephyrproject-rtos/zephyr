/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Memory Statistics
 */

#ifndef ZEPHYR_INCLUDE_SYS_MEM_STATS_H_
#define ZEPHYR_INCLUDE_SYS_MEM_STATS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* A common structure used to report runtime memory usage statistics */

struct sys_memory_stats {
	size_t  free_bytes;
	size_t  allocated_bytes;
	size_t  max_allocated_bytes;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MEM_STATS_H_ */
