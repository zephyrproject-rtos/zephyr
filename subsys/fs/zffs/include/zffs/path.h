/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_PATH_
#define H_ZFFS_PATH_

#include "dir.h"
#include "area.h"
#include "zffs.h"

#ifdef __cplusplus
extern "C" {
#endif

int zffs_path_step(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, const char **path,
		    struct zffs_node_data *node_data, sys_snode_t **snode);

// int zffs_path_find_step(struct zffs_data *zffs,
// 			 struct zffs_area_pointer *pointer, const char **path,
// 			 struct zffs_dir *dir);

#ifdef __cplusplus
}
#endif

#endif
