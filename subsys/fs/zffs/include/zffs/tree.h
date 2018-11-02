/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_TREE_
#define H_ZFFS_TREE_

#include "zffs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZFFS_TREE_T ZFFS_CONFIG_TREE_T

int zffs_tree_init(struct zffs_data *zffs);
int zffs_tree_sync(struct zffs_data *zffs);
int zffs_tree_gc(struct zffs_data *zffs);
int zffs_tree_info(struct zffs_data *zffs, u32_t *key_count, u32_t *key_max,
		    u32_t *value_min, u32_t *value_max);
int zffs_tree_foreach(struct zffs_data *zffs, void *data,
		       int (*tree_cb)(struct zffs_data *zffs, u32_t key,
				      u32_t value, void *data));

int zffs_tree_search(struct zffs_data *zffs, u32_t key, u32_t *value);
int zffs_tree_insert(struct zffs_data *zffs, u32_t key, u32_t value);
int zffs_tree_update(struct zffs_data *zffs, u32_t key, u32_t value);
int zffs_tree_delete(struct zffs_data *zffs, u32_t key);

#ifdef __cplusplus
}
#endif

#endif