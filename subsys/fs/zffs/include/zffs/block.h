/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_BLOCK_
#define H_ZFFS_BLOCK_

#include "area.h"
#include "zffs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct zffs_block {
	u32_t id;
};

int zffs_block_open(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, u32_t id,
		     struct zffs_block *block);

int zffs_block_make(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, u32_t id, u32_t size,
		     struct zffs_block *block, u16_t *crc);

#ifdef __cplusplus
}
#endif

#endif
