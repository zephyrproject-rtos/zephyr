/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_OBJECT_
#define H_ZFFS_OBJECT_

#include "area.h"
#include "zffs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZFFS_OBJECT_TYPE_NONE 0xff
#define ZFFS_OBJECT_TYPE_ROOT 0x00
#define ZFFS_OBJECT_TYPE_SLIST_NODE 0x01
#define ZFFS_OBJECT_TYPE_DLIST_NODE 0x02
#define ZFFS_OBJECT_TYPE_BLOCK 0x03
#define ZFFS_OBJECT_TYPE_DEL_TAG 0x04

typedef int (*zffs_object_callback_t)(struct zffs_data *zffs, u32_t id,
				       struct zffs_area_pointer *pointer,
				       u32_t size, u32_t addr, void *data);

int zffs_object_new(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, u32_t id, u32_t size);

int zffs_object_update(struct zffs_data *zffs,
			struct zffs_area_pointer *pointer, u32_t id,
			u32_t size);

int zffs_object_open(struct zffs_data *zffs,
		      struct zffs_area_pointer *pointer, u32_t id,
		      u32_t *addr);

int zffs_object_check(struct zffs_data *zffs,
		       struct zffs_area_pointer *pointer, u32_t *id);

int zffs_object_delete(struct zffs_data *zffs,
			struct zffs_area_pointer *pointer, u32_t id);

int zffs_object_foreach(struct zffs_data *zffs, void *data,
			 zffs_object_callback_t object_cb);

#ifdef __cplusplus
}
#endif

#endif
