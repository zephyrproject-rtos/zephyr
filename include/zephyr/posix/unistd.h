/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_H_

#include "posix_types.h"

#include <zephyr/fs/fs.h>
#include <zephyr/posix/sys/socket.h>
#include <signal.h>
#include <zephyr/posix/sys/confstr.h>
#include <zephyr/posix/sys/stat.h>
#include <zephyr/posix/sys/sysconf.h>
#include <zephyr/posix/sys/features.h>

#ifdef __cplusplus
extern "C" {
#endif

/* File related operations */
int close(int file);
ssize_t write(int file, const void *buffer, size_t count);
ssize_t read(int file, void *buffer, size_t count);
off_t lseek(int file, off_t offset, int whence);
#if defined(_POSIX_FSYNC)
int fsync(int fd);
#endif /* defined(_POSIX_FSYNC) */
#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 500
int ftruncate(int fd, off_t length);
#endif

/* File System related operations */
int unlink(const char *path);

FUNC_NORETURN void _exit(int status);

#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 500
int gethostname(char *__name, size_t __len);
#endif

#ifdef CONFIG_GETOPT
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

#endif /* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
