/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/object.h"
#include "zffs/area.h"
#include "zffs/tree.h"
#include <crc16.h>

struct zffs_object_disk {
	u8_t id[sizeof(u32_t)];
	u8_t size[sizeof(u32_t)];
	u8_t crc[2];
};

static int object_make(struct zffs_data *zffs,
		       struct zffs_area_pointer *pointer, u32_t id, u32_t size,
		       bool is_update)
{
	struct zffs_object_disk disk;
	u32_t offset;
	int rc;

	sys_put_le32(id, disk.id);
	sys_put_le32(size, disk.size);
	sys_put_le16(crc16_ccitt(0, (const void *)&disk,
				 offsetof(struct zffs_object_disk, crc)),
		     disk.crc);

	offset = zffs_area_pointer_to_addr(zffs, pointer);

	rc = zffs_area_write(zffs, pointer, &disk, sizeof(disk));
	if (rc) {
		return rc;
	}

	rc = zffs_tree_insert(zffs, id, offset);
	if (is_update && rc == -EEXIST) {
		rc = zffs_tree_update(zffs, id, offset);
	}

	return rc;
}

int zffs_object_new(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, u32_t id, u32_t size)
{
	return object_make(zffs, pointer, id, size, false);
}

int zffs_object_update(struct zffs_data *zffs,
		       struct zffs_area_pointer *pointer, u32_t id,
		       u32_t size)
{
	return object_make(zffs, pointer, id, size, true);
}

static int object_open(struct zffs_data *zffs,
		       struct zffs_area_pointer *pointer, u32_t id, u32_t addr)
{
	int rc;
	struct zffs_object_disk disk;

	zffs_area_addr_to_pointer(zffs, addr, pointer);

	rc = zffs_area_read(zffs, pointer, &disk, sizeof(disk));
	if (rc) {
		return rc;
	}
	if (sys_get_le32(disk.id) != id ||
	    crc16_ccitt(0, (const void *)&disk, sizeof(disk)) != 0) {
		return -EIO;
	}

	return sys_get_le32(disk.size);
}

int zffs_object_open(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, u32_t id,
		     u32_t *out_addr)
{
	u32_t addr;
	int rc;

	rc = zffs_tree_search(zffs, id, &addr);
	if (rc) {
		return rc;
	}

	rc = object_open(zffs, pointer, id, addr);
	if (rc < 0) {
		return rc;
	}

	if (out_addr) {
		*out_addr = addr;
	}

	return rc;
}

int zffs_object_check(struct zffs_data *zffs,
		      struct zffs_area_pointer *pointer, u32_t *id)
{
	struct zffs_object_disk disk;
	u8_t *p = (u8_t *)&disk;
	int rc, i;
	u16_t crc = 0;

	rc = zffs_area_read(zffs, pointer, &disk, sizeof(disk));
	if (rc) {
		return rc;
	}

	for (i = 0; i < sizeof(disk); i++) {
		if (p[i] != 0xff) {
			break;
		}
	}

	if (i == sizeof(disk)) {
		return -ENOENT;
	}

	if (crc16_ccitt(0, (const void *)&disk, sizeof(disk)) != 0) {
		return -EIO;
	}

	rc = zffs_area_crc(zffs, pointer, sys_get_le32(disk.size), &crc);

	if (rc) {
		return rc;
	}

	if (id) {
		*id = sys_get_le32(disk.id);
	}

	return 0;
}

struct object_foreach_data {
	void *data;
	struct zffs_area_pointer *pointer;
	zffs_object_callback_t callback;
};

static int object_foreach_cb(struct zffs_data *zffs, u32_t key, u32_t value,
			     struct object_foreach_data *data)
{
	int rc = object_open(zffs, data->pointer, key, value);

	if (rc < 0) {
		return rc;
	}

	return data->callback(zffs, key, data->pointer, rc, value, data->data);
}

int zffs_object_foreach(struct zffs_data *zffs, void *data,
			zffs_object_callback_t object_cb)
{
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);
	struct object_foreach_data cb_data = {
		.data = data, .pointer = &pointer, .callback = object_cb
	};

	return zffs_tree_foreach(zffs, &cb_data, (void *)object_foreach_cb);
}

int zffs_object_delete(struct zffs_data *zffs,
		       struct zffs_area_pointer *pointer, u32_t id)
{
	// todo
	return 0;
}
