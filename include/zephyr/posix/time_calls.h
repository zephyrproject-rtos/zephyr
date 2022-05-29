/*
 * Copyright (c) 2022 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_TIME_CALLS_H_
#define ZEPHYR_INCLUDE_POSIX_TIME_CALLS_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POSIX_CLOCK
__syscall int zephyr_clock_gettime(clockid_t clock_id, struct timespec *ts);
#endif

#ifdef __cplusplus
}
#endif

#include <syscalls/time_calls.h>

#endif
