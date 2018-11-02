/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_FILE_
#define H_ZFFS_FILE_

#include "block.h"
#include "dir.h"
#include "queue.h"
#include "zffs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct zffs_file {
	sys_snode_t _snode;
	u32_t id;

	u32_t size;
	u32_t offset;
	u32_t next_id;

	struct zffs_dlist list;

	struct zffs_dlist_node node;

	struct zffs_area_pointer pointer;
	struct zffs_block block;

	struct {
		struct zffs_area_pointer pointer;
		struct zffs_block block;
		u16_t crc;
	} write;
};

int zffs_file_make(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, struct zffs_dir *dir,
		    const struct zffs_node_data *node_data,
		    struct zffs_file *file);

int zffs_file_open(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer,
		    const struct zffs_node_data *node_data,
		    struct zffs_file *file);

int zffs_file_read(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, struct zffs_file *file,
		    void *data, u32_t len);

int zffs_file_write(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer,
		     struct zffs_file *file, const void *data, u32_t len);

int zffs_file_sync(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer,
		    struct zffs_file *file);

int zffs_file_close(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer,
		     struct zffs_file *file);

int zffs_file_seek(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, struct zffs_file *file,
		    int whence, int offset);

bool zffs_file_is_open(struct zffs_data *zffs, u32_t id,
			struct zffs_file **file);

#ifdef __cplusplus
}
#endif

#endif
