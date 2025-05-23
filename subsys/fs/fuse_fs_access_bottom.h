/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_FS_FUSE_FS_ACCESS_BOTTOM_H
#define SUBSYS_FS_FUSE_FS_ACCESS_BOTTOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_FILE_HANDLE (INT32_MAX)

struct ffa_dirent {
	bool is_directory;
	char *name;
	size_t size;
};

struct ffa_op_callbacks {
	int (*stat)(const char *path, struct ffa_dirent *entry);
	int (*readmount)(int *mnt_nbr, const char **mnt_name);
	int (*readdir_start)(const char *path);
	int (*readdir_read_next)(struct ffa_dirent *entry);
	void (*readdir_end)(void);
	int (*mkdir)(const char *path);
	int (*create)(const char *path, uint64_t *fh);
	int (*release)(uint64_t fh);
	int (*read)(uint64_t fh, char *buf, size_t size, off_t off);
	int (*write)(uint64_t fh, const char *buf, size_t size, off_t off);
	int (*ftruncate)(uint64_t fh, off_t size);
	int (*truncate)(const char *path, off_t size);
	int (*unlink)(const char *path);
	int (*rmdir)(const char *path);
};

void ffsa_init_bottom(const char *fuse_mountpoint, struct ffa_op_callbacks *op_cbs);
void ffsa_cleanup_bottom(const char *fuse_mountpoint);

bool ffa_is_op_pended(void);
void ffa_run_pending_op(void);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_FS_FUSE_FS_ACCESS_BOTTOM_H */
