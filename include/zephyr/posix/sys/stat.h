/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_POSIX_SYS_STAT_H_
#define ZEPHYR_POSIX_SYS_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define S_IFBLK	 0060000
#define S_IFCHR	 0020000
#define S_IFIFO	 0010000
#define S_IFREG	 0100000
#define S_IFDIR	 0040000
#define S_IFLNK	 0120000
#define S_IFSOCK 0140000

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

/* File open modes */
#define O_ACCMODE	   0003
#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02

struct stat {
	unsigned long st_size;
	unsigned long st_blksize;
	unsigned long st_blocks;
	unsigned long st_mode;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_POSIX_SYS_STAT_H_ */
