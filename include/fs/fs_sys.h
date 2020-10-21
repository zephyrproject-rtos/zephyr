/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_FS_SYS_H_
#define ZEPHYR_INCLUDE_FS_FS_SYS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup file_system_api
 * @{
 */

#define FILE_SYSTEM_DEFINE_UNSUPPORTED_OPERATION(type) 			\
	static type file_system_unsupported_operation_##type(void)	\
	{								\
		return -ENOTSUP;					\
	}

FILE_SYSTEM_DEFINE_UNSUPPORTED_OPERATION(off_t);
FILE_SYSTEM_DEFINE_UNSUPPORTED_OPERATION(ssize_t);
FILE_SYSTEM_DEFINE_UNSUPPORTED_OPERATION(int);

#define FILE_SYSTEM_UNSUPPORTED_OPERATION(type)				\
	(void *)file_system_unsupported_operation_##type

/**
 * @brief Define file_system_t API callback table for file system
 *
 * The macro defines static file_system_t object, and assigns provided
 * callback functions to operations; operations that are not assigned
 * any function will be assigned default function returning -ENOTSUP
 * with exception to mount and unmount which are assigned NULL.
 *
 * @param name name of file_system_t object
 * @param ... list of designated member initializers of file_system_t
 */
#define FILE_SYSTEM_DEFINE(name, ...)					\
	static struct fs_file_system_t name = {				\
		.open = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),		\
		.read = FILE_SYSTEM_UNSUPPORTED_OPERATION(ssize_t),	\
		.write = FILE_SYSTEM_UNSUPPORTED_OPERATION(ssize_t),	\
		.lseek = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.tell = FILE_SYSTEM_UNSUPPORTED_OPERATION(off_t),	\
		.truncate = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.sync = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),		\
		.close = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.opendir = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.readdir = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.closedir = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.mount = NULL,						\
		.unmount = NULL,					\
		.unlink = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.rename = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.mkdir = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		.stat = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),		\
		.statvfs = FILE_SYSTEM_UNSUPPORTED_OPERATION(int),	\
		__VA_ARGS__						\
	}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FS_SYS_H_ */
