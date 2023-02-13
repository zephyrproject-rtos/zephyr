/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_DIRENT_H_
#define ZEPHYR_INCLUDE_POSIX_DIRENT_H_

#include <limits.h>
#include "posix_types.h"

#ifdef CONFIG_POSIX_FS
#include <zephyr/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void DIR;

struct dirent {
	unsigned int d_ino;
	char d_name[PATH_MAX + 1];
};

/* Directory related operations */
extern DIR *opendir(const char *dirname);
extern int closedir(DIR *dirp);
extern struct dirent *readdir(DIR *dirp);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_POSIX_FS */

#endif	/* ZEPHYR_INCLUDE_POSIX_DIRENT_H_ */
