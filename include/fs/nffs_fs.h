/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NFFS_FS_H_
#define _NFFS_FS_H_

#include <sys/types.h>
#include <nffs/nffs.h>

#ifdef __cplusplus
extern "C" {
#endif

FS_FILE_DEFINE(struct nffs_file *fp);
FS_DIR_DEFINE(struct nffs_dir *dp);

#define MAX_FILE_NAME 256

#ifdef __cplusplus
}
#endif

#endif /* _NFFS_FS_H_ */
