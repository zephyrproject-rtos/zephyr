/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Utility functions for use by filesystem implementations. */

#ifndef ZEPHYR_SUBSYS_FS_FS_IMPL_H_
#define ZEPHYR_SUBSYS_FS_FS_IMPL_H_

#include <fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Strip the mount point prefix from a path.
 *
 * @param path an absolute path beginning with a mount point.
 *
 * @param mp a pointer to the mount point within which @p path is found
 *
 * @return the absolute path within the mount point.  Behavior is
 * undefined if @p path does not start with the mount point prefix.
 */
const char *fs_impl_strip_prefix(const char *path,
				 const struct fs_mount_t *mp);

/**
 * @brief Searches for file system type FS_* identifier, as defined in fs/fs.h,
 *        compatible with string representation of type.
 *
 * @param string representation of type.
 *
 * @return fs type or FS_UNKNOWN.
 */
int fs_get_compatible(const char *type_sz);

/**
 * @breof Searches for API of file system compatible with string form of type.
 *
 * First system that confirms compatibility will be returned.
 *
 * @param type_sz string representation of file system type
 *
 * @return pointer to API structure fs_file_system_t or NULL if compatible
 * system has not been found.
 */
const struct fs_file_system_t *fs_get_compatible_api(const char *type_sz);

/**
 * @brief Searches for API definition of file system by its numeric type.
 *
 * Pointer to API of first system that confirms compatibility will be returned.
 * @param fs type to search for.
 *
 * @return pointer to API structure fs_file_system_t or NULL if system
 * does not seem to be registered yet.
 */
const struct fs_file_system_t *fs_get_api(int t);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_FS_FS_IMPL_H_ */
