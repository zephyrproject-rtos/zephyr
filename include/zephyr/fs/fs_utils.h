/*
 * Copyright (c) 2026 Rishav Chakraborty.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief High-level file system helpers built on top of the fs API.
 * @ingroup file_system_api
 */

#ifndef ZEPHYR_INCLUDE_FS_FS_UTILS_H_
#define ZEPHYR_INCLUDE_FS_FS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup file_system_api
 * @{
 */

/**
 * @brief Create a directory and any missing parent directories.
 *
 * Works like @c mkdir @c -p. Components that already exist as directories are skipped.
 *
 * @param abs_path Absolute path of the directory to create, inside a mounted file system.
 *
 * @retval 0 Directory exists on return. The root path is a no-op.
 * @retval -EINVAL @p abs_path is NULL or is not absolute.
 * @retval -ENAMETOOLONG @p abs_path is not shorter than
 *         @kconfig{CONFIG_FILE_SYSTEM_UTILS_MAX_PATH_LEN}.
 * @retval -ENOTDIR A component of @p abs_path exists and is not a directory.
 * @retval <0 another negative errno code on error
 */
int fs_mkdir_p(const char *abs_path);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FS_UTILS_H_ */
