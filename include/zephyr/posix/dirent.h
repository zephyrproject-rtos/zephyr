/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_DIRENT_H_
#define ZEPHYR_INCLUDE_POSIX_DIRENT_H_

#include <limits.h>
#include "posix_types.h"

#include <zephyr/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _XOPEN_SOURCE >= 700 || _POSIX_C_SOURCE >= 200809L

typedef void DIR;

struct dirent {
	unsigned int d_ino;
	char d_name[PATH_MAX + 1];
};

/* Directory related operations */
DIR *opendir(const char *dirname);
int closedir(DIR *dirp);
struct dirent *readdir(DIR *dirp);

#endif

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_DIRENT_H_ */
