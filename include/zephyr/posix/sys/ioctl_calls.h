/*
 * Copyright (c) 2021 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_IOCTL_CALLS_H_
#define ZEPHYR_INCLUDE_POSIX_IOCTL_CALLS_H_

#ifdef __cplusplus
extern "C" {
#endif

__syscall int zephyr_ioctl(int fd, unsigned long request, va_list args);

#ifdef __cplusplus
}
#endif

#include <syscalls/ioctl_calls.h>
#endif
