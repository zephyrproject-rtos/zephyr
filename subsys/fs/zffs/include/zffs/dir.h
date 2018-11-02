/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_DIR_
#define H_ZFFS_DIR_

#include "queue.h"
#include "zffs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct zffs_dir {
	sys_snode_t _snode;
	u32_t id;

	struct zffs_slist list;
	struct zffs_slist_node node;
};

int zffs_dir_append(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, struct zffs_dir *dir,
		     const struct zffs_node_data *node_data);

int zffs_dir_unlink(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, struct zffs_dir *dir,
		     const struct zffs_node_data *node_data);

int zffs_dir_search(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer,
		     const struct zffs_dir *dir, const char *name,
		     struct zffs_node_data *node_data, sys_snode_t **snode);

int zffs_dir_search_for_node_data(struct zffs_data *zffs,
				   struct zffs_area_pointer *pointer,
				   struct zffs_node_data *node_data,
				   const char *name, sys_snode_t **snode);

int zffs_dir_open(struct zffs_data *zffs, struct zffs_area_pointer *pointer,
		   const struct zffs_node_data *node_data,
		   struct zffs_dir *dir);

int zffs_dir_read(struct zffs_data *zffs, struct zffs_area_pointer *pointer,
		   struct zffs_dir *dir, struct zffs_node_data *data);

int zffs_dir_close(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, struct zffs_dir *dir);

int zffs_dir_update_node(struct zffs_data *zffs,
			  struct zffs_area_pointer *pointer,
			  const struct zffs_node_data *node_data);

int zffs_dir_load_node(struct zffs_data *zffs,
			struct zffs_area_pointer *pointer, u32_t object_size,
			struct zffs_node_data *data);

#ifdef __cplusplus
}
#endif

#endif
