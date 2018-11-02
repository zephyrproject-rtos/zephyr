/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/block.h"
#include "zffs/object.h"

struct block_disk_head {
	zffs_disk(u8_t, object_type);
};

struct block_disk_tail {
	zffs_disk(u16_t, crc);
};

int zffs_block_open(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, u32_t id,
		     struct zffs_block *block)
{
	struct block_disk_head head;
	int rc;
	u32_t obj_size;

	rc = zffs_object_open(zffs, pointer, id, NULL);
	if (rc < 0) {
		return rc;
	}

	obj_size = rc;

	if (obj_size < sizeof(head) + sizeof(struct block_disk_tail)) {
		return -EIO;
	}

	rc = zffs_area_read(zffs, pointer, &head, sizeof(head));
	if (rc) {
		return rc;
	}

	if (*head.object_type != ZFFS_OBJECT_TYPE_BLOCK) {
		return -EIO;
	}

	block->id = id;

	return obj_size - sizeof(head) - sizeof(struct block_disk_tail);
}

int zffs_block_make(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, u32_t id, u32_t size,
		     struct zffs_block *block, u16_t *crc)
{
	struct block_disk_head head;
	int rc;

	*head.object_type = ZFFS_OBJECT_TYPE_BLOCK;

	rc = zffs_object_new(zffs, pointer, id,
			      size + sizeof(head) +
				  sizeof(struct block_disk_tail));
	if (rc) {
		return rc;
	}

	*crc = crc16_ccitt(0, (const void *)&head, sizeof(head));
	block->id = id;

	return zffs_area_write(zffs, pointer, &head, sizeof(head));
}
