/*
 * Copyright (c) 2022 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_CALLS_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_CALLS_H_

#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
__syscall int zephyr_open(const char *path, int oflag, va_list args);
__syscall int zephyr_close(int fd);
__syscall ssize_t zephyr_read(int fd, void *buffer, size_t count);
__syscall ssize_t zephyr_write(int fd, const void *buffer, size_t count);
__syscall off_t zephyr_lseek(int fd, off_t offset, int whence);
__syscall int zephyr_fcntl(int fd, int cmd, va_list args);
__syscall int zephyr_fsync(int fd);
#endif /* #if defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES) */

#ifdef __cplusplus
}
#endif

#include <syscalls/unistd_calls.h>
#endif
