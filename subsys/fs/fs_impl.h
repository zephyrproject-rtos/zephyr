/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Utility functions for use by filesystem implementations. */

#ifndef ZEPHYR_SUBSYS_FS_FS_IMPL_H_
#define ZEPHYR_SUBSYS_FS_FS_IMPL_H_

#include <fs.h>

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


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_FS_FS_IMPL_H_ */
