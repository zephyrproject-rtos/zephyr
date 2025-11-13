/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_DIRENT_H_
#define ZEPHYR_INCLUDE_POSIX_DIRENT_H_

#include <limits.h>

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(NAME_MAX) && defined(_XOPEN_SOURCE)
#define NAME_MAX _XOPEN_NAME_MAX
#endif

#if !defined(NAME_MAX) && defined(_POSIX_C_SOURCE)
#define NAME_MAX _POSIX_NAME_MAX
#endif

typedef void DIR;

struct dirent {
	unsigned int d_ino;
	char d_name[NAME_MAX + 1];
};

#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
int alphasort(const struct dirent **d1, const struct dirent **d2);
#endif
int closedir(DIR *dirp);
#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
int dirfd(DIR *dirp);
#endif
DIR *fdopendir(int fd);
DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dirp);
#if (_POSIX_C_SOURCE >= 199506L) || (_XOPEN_SOURCE >= 500)
int readdir_r(DIR *ZRESTRICT dirp, struct dirent *ZRESTRICT entry,
	      struct dirent **ZRESTRICT result);
#endif
void rewinddir(DIR *dirp);
#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
int scandir(const char *dir, struct dirent ***namelist, int (*sel)(const struct dirent *),
	    int (*compar)(const struct dirent **, const struct dirent **));
#endif
#if defined(_XOPEN_SOURCE)
void seekdir(DIR *dirp, long loc);
long telldir(DIR *dirp);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_DIRENT_H_ */
