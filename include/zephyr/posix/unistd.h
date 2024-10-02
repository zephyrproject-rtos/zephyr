/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_H_

#include "posix_types.h"

#ifdef CONFIG_POSIX_API
#include <zephyr/fs/fs.h>
#endif
#ifdef CONFIG_NETWORKING
/* For zsock_gethostname() */
#include <zephyr/net/socket.h>
#include <zephyr/net/hostname.h>
#endif
#include <zephyr/posix/sys/confstr.h>
#include <zephyr/posix/sys/stat.h>
#include <zephyr/posix/sys/sysconf.h>

#include "posix_features.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POSIX_API
/* File related operations */
int close(int file);
ssize_t write(int file, const void *buffer, size_t count);
ssize_t read(int file, void *buffer, size_t count);
off_t lseek(int file, off_t offset, int whence);
int fsync(int fd);
int ftruncate(int fd, off_t length);

#ifdef CONFIG_POSIX_SYNCHRONIZED_IO
int fdatasync(int fd);
#endif /* CONFIG_POSIX_SYNCHRONIZED_IO */

/* File System related operations */
int rename(const char *old, const char *newp);
int unlink(const char *path);
int stat(const char *path, struct stat *buf);
int mkdir(const char *path, mode_t mode);
int rmdir(const char *path);

FUNC_NORETURN void _exit(int status);

#ifdef CONFIG_POSIX_PIPE
int pipe(int fildes[2]);
#endif

#ifdef CONFIG_NETWORKING
static inline int gethostname(char *buf, size_t len)
{
	return zsock_gethostname(buf, len);
}
#endif /* CONFIG_NETWORKING */

#endif /* CONFIG_POSIX_API */

#ifdef CONFIG_POSIX_C_LIB_EXT
int getopt(int argc, char *const argv[], const char *optstring);
extern char *optarg;
extern int opterr, optind, optopt;
#endif

int getentropy(void *buffer, size_t length);
pid_t getpid(void);
unsigned sleep(unsigned int seconds);
int usleep(useconds_t useconds);
#if _POSIX_C_SOURCE >= 2
size_t confstr(int name, char *buf, size_t len);
#endif

#ifdef CONFIG_POSIX_SYSCONF_IMPL_MACRO
#define sysconf(x) (long)CONCAT(__z_posix_sysconf, x)
#else
long sysconf(int opt);
#endif /* CONFIG_POSIX_SYSCONF_IMPL_FULL */

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
