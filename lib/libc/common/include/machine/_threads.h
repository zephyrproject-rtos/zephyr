/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MACHINE__THREADS_H_
#define ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MACHINE__THREADS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ONCE_FLAG_INIT {0}

typedef uint32_t cnd_t;
typedef uint32_t mtx_t;
typedef uint32_t thrd_t;
typedef uint32_t tss_t;
typedef struct {
	char flag;
} once_flag;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MACHINE__THREADS_H_ */
