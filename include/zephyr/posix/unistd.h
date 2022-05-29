/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_H_

#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifdef CONFIG_POSIX_FS
#include <zephyr/fs/fs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __useconds_t_defined
#ifdef CONFIG_NEWLIB_LIBC
#define __useconds_t_defined
typedef __useconds_t useconds_t;
#endif
#endif

#ifndef _USECONDS_T_DECLARED
#ifdef CONFIG_PICOLIBC
#define _USECONDS_T_DECLARED
typedef __useconds_t useconds_t;
#endif
#endif

#if !(defined(__useconds_t_defined) || defined(_USECONDS_T_DECLARED))
typedef unsigned int useconds_t;
#endif

#if defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
/* File or Socket related operations */
int open(const char *path, int oflag, ...);
int close(int fd);
ssize_t read(int fd, void *buffer, size_t count);
ssize_t write(int fd, const void *buffer, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int fcntl(int fd, int cmd, ...);
int fsync(int fd);
#endif /* defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES) */

#ifdef CONFIG_POSIX_API
/* File System related operations */
int rename(const char *old, const char *newp);
int unlink(const char *path);
int stat(const char *path, struct stat *buf);
int mkdir(const char *path, mode_t mode);
#endif /* CONFIG_POSIX_API */

unsigned sleep(unsigned int seconds);
int usleep(useconds_t useconds);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
