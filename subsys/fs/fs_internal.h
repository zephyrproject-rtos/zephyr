/*
 * Copyright (c) 2026 Rishav Chakraborty.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Declarations shared between files in subsys/fs; not public API. */

#ifndef ZEPHYR_SUBSYS_FS_FS_INTERNAL_H_
#define ZEPHYR_SUBSYS_FS_FS_INTERNAL_H_

#include <stddef.h>
#include <zephyr/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Find the mount point that contains a path.
 *
 * @param mnt_pntp Receives the mount point with the longest matching prefix.
 * @param name Absolute path.
 * @param match_len Receives the length of the matched mount point; may be NULL.
 *
 * @retval 0 Mount point found.
 * @retval -ENOENT No mount point contains @p name.
 */
int fs_get_mnt_point(struct fs_mount_t **mnt_pntp, const char *name, size_t *match_len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_FS_FS_INTERNAL_H_ */
