/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_MISC_
#define H_ZFFS_MISC_

#include "area.h"
#include "zffs.h"

#ifdef __cplusplus
extern "C" {
#endif

void zffs_misc_lock(struct zffs_data *zffs);
void zffs_misc_unlock(struct zffs_data *zffs);
u32_t zffs_misc_get_id(struct zffs_data *zffs);
u32_t zffs_misc_next_id(struct zffs_data *zffs, u32_t *id);
int zffs_misc_restore(struct zffs_data *zffs);
int zffs_misc_load_node(struct zffs_data *zffs,
			 struct zffs_area_pointer *pointer, u32_t object_size,
			 struct zffs_node_data *data, sys_snode_t **snode);

#ifdef __cplusplus
}
#endif

#endif
