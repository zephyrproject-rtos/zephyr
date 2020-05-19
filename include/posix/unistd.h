/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_H_

#include "posix_types.h"
#include "sys/stat.h"
#ifdef CONFIG_NETWORKING
/* For zsock_gethostname() */
#include "net/socket.h"
#endif

#ifdef CONFIG_POSIX_API
#include <fs/fs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POSIX_API
/* File related operations. Convention: "sys_name" is a syscall (needs
 * prototype in this file for usage). "name" is a normal userspace
 * function (implemented as a wrapper for syscall), usable even
 * without prototype, per classical C handling. This distinction
 * is however implemented on demand, based on the actual usecases seen.
 */
__syscall int sys_close(int file);
__syscall ssize_t sys_write(int file, const void *buffer, size_t count);
__syscall ssize_t sys_read(int file, void *buffer, size_t count);
extern int close(int file);
extern ssize_t write(int file, const void *buffer, size_t count);
extern ssize_t read(int file, void *buffer, size_t count);
extern off_t lseek(int file, off_t offset, int whence);

/* File System related operations */
extern int rename(const char *old, const char *newp);
extern int unlink(const char *path);
extern int stat(const char *path, struct stat *buf);
extern int mkdir(const char *path, mode_t mode);

#ifdef CONFIG_NETWORKING
static inline int gethostname(char *buf, size_t len)
{
	return zsock_gethostname(buf, len);
}
#endif /* CONFIG_NETWORKING */

#endif /* CONFIG_POSIX_API */

unsigned sleep(unsigned int seconds);
int usleep(useconds_t useconds);

#ifdef __cplusplus
}
#endif

#ifndef CONFIG_ARCH_POSIX
#include <syscalls/unistd.h>
#endif /* CONFIG_ARCH_POSIX */

#endif	/* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
