/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_H_

#include "posix_types.h"
#include <zephyr/posix/sys/stat.h>
#ifdef CONFIG_NETWORKING
/* For zsock_gethostname() */
#include <zephyr/net/socket.h>
#endif

#ifdef CONFIG_POSIX_API
#include <zephyr/fs/fs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POSIX_API
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

FUNC_NORETURN void _exit(int status);

#ifdef CONFIG_NETWORKING
static inline int gethostname(char *buf, size_t len)
{
	return zsock_gethostname(buf, len);
}
#endif /* CONFIG_NETWORKING */

#endif /* CONFIG_POSIX_API */

#ifdef CONFIG_GETOPT
int getopt(int argc, char *const argv[], const char *optstring);
extern char *optarg;
extern int opterr, optind, optopt;
#endif

unsigned sleep(unsigned int seconds);
int usleep(useconds_t useconds);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
