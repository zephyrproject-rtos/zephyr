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

#ifdef __cplusplus
extern "C" {
#endif

/* File related operations */
extern int close(int file);
extern ssize_t write(int file, const void *buffer, size_t count);
extern ssize_t read(int file, void *buffer, size_t count);
extern off_t lseek(int file, off_t offset, int whence);

/* File System related operations */
extern int rename(const char *old, const char *newp);
extern int unlink(const char *path);
extern int stat(const char *path, struct stat *buf);
extern int mkdir(const char *path, mode_t mode);
#endif

unsigned sleep(unsigned int seconds);
int usleep(useconds_t useconds);

#ifdef CONFIG_NETWORKING
static inline int gethostname(char *buf, size_t len)
{
	return zsock_gethostname(buf, len);
}
#endif

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
