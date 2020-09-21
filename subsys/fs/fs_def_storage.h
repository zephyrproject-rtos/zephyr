/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_FS_FS_DEF_STORAGE_H_
#define ZEPHYR_SUBSYS_FS_FS_DEF_STORAGE_H_

/* Defines identifier for default data storage allocator. */
#define DEFAULT_FSDATA_STORAGE_ALLOCATOR fs_default_fsdata_alloc

/* Macro defines default file system data storage function allocator, named
 * by identifier defined by DEFAULT_AUTOMOUNT_ALLOCATOR.
 * This allocator holds static array of data structures and returns pointer
 * to current free until it runs out of free structures, in which case it
 * returns null.
 */
#define DEFINE_DEFAULT_FSDATA_STORAGE(fs_data_type, N, fs_data_init_fun)\
	static void *DEFAULT_FSDATA_STORAGE_ALLOCATOR(void)		\
	{								\
		static int index;					\
		static struct fs_data_type resources[N] = { 0 };	\
		if (index >= N) {					\
			errno = ENOSPC;					\
			return NULL;					\
		} else {						\
			errno = 0;					\
		}							\
		fs_data_init_fun(&resources[index]);			\
		return &resources[index++];				\
	}

#endif /* ZEPHYR_SUBSYS_FS_FS_DEF_STORAGE_H_ */
