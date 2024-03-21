/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_POSIX_CLOCK_H_
#define ZEPHYR_LIB_POSIX_POSIX_CLOCK_H_

#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/posix_types.h>

__syscall int __posix_clock_get_base(clockid_t clock_id, struct timespec *ts);

#include <syscalls/posix_clock.h>

#endif
