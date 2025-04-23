/*
 * Copyright (c) 2023, Meta
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_POSIX_CLOCK_H_
#define ZEPHYR_LIB_POSIX_POSIX_CLOCK_H_

#include <stdint.h>
#include <time.h>

uint32_t timespec_to_clock_timeoutms(clockid_t clock_id, const struct timespec *abstime);
uint32_t timespec_to_timeoutms(const struct timespec *abstime);

__syscall int __posix_clock_get_base(clockid_t clock_id, struct timespec *ts);

#include <zephyr/syscalls/posix_clock.h>

#endif
