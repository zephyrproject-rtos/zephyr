/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_UNISTD_H_
#define ZEPHYR_INCLUDE_POSIX_UNISTD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sys/types.h"
#include "sys/stat.h"

#ifdef CONFIG_POSIX_FS
#include <fs.h>

typedef struct fs_dir_t DIR;
typedef unsigned int mode_t;

struct dirent {
	unsigned int d_ino;
	char d_name[PATH_MAX + 1];
};

/* File related operations */
extern int open(const char *name, int flags);
extern int close(int file);
extern ssize_t write(int file, char *buffer, unsigned int count);
extern ssize_t read(int file, char *buffer, unsigned int count);
extern int lseek(int file, int offset, int whence);

/* Directory related operations */
extern DIR *opendir(const char *dirname);
extern int closedir(DIR *dirp);
extern struct dirent *readdir(DIR *dirp);

/* File System related operations */
extern int rename(const char *old, const char *newp);
extern int unlink(const char *path);
extern int stat(const char *path, struct stat *buf);
extern int mkdir(const char *path, mode_t mode);
#endif

unsigned sleep(unsigned int seconds);
int usleep(useconds_t useconds);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_UNISTD_H_ */
