/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_PTHREAD_KEY_H_
#define ZEPHYR_INCLUDE_POSIX_PTHREAD_KEY_H_

#ifdef CONFIG_PTHREAD_IPC
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MINIMAL_LIBC) || defined(CONFIG_PICOLIBC)
typedef struct {
	int is_initialized;
	int init_executed;
} pthread_once_t;
#endif

/* pthread_key */
typedef uint32_t pthread_key_t;

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_PTHREAD_IPC */

#endif /* ZEPHYR_INCLUDE_POSIX_PTHREAD_KEY_H_*/
