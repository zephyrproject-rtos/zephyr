/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/dir.h"
#include "zffs/file.h"
#include "zffs/misc.h"
#include "zffs/object.h"
#include "zffs/path.h"
#include "zffs/queue.h"
#include <string.h>

struct dir_root_disk {
	zffs_disk(u8_t, object_type);
	zffs_disk(u32_t, head);
	zffs_disk(u16_t, crc);
};

struct dir_slist_dir_node_disk {
	zffs_disk(u8_t, type);
	zffs_disk(u32_t, head);
	u8_t name[ZFFS_CONFIG_NAME_MAX];
};

struct dir_slist_file_node_disk {
	zffs_disk(u8_t, type);
	zffs_disk(u32_t, head);
	zffs_disk(u32_t, tail);
	zffs_disk(u32_t, size);
	zffs_disk(u32_t, next_id);
	u8_t name[ZFFS_CONFIG_NAME_MAX];
};

static int dir_update_node(struct zffs_data *zffs,
			   struct zffs_area_pointer *pointer,
			   const struct zffs_node_data *node_data)
{
	struct zffs_area_pointer rp = *pointer;
	int rc;
	u32_t len;

	union {
		zffs_disk(u8_t, type);
		struct dir_slist_dir_node_disk dir;
		struct dir_slist_file_node_disk file;
	} disk;

	rc = zffs_slist_open_ex(zffs, &rp, node_data->id);
	if (rc < 0) {
		return rc;
	}

	len = rc;

	if (len > sizeof(disk)) {
		return -EIO;
	}

	rc = zffs_area_read(zffs, &rp, &disk, len);
	if (rc) {
		return rc;
	}

	if (*disk.type != node_data->type) {
		return -EIO;
	}

	switch (node_data->type) {
	case ZFFS_TYPE_DIR:
		if (len < offsetof(struct dir_slist_dir_node_disk, name)) {
			return -EIO;
		}

		if (node_data->dir.name) {
			strcpy(disk.dir.name, node_data->dir.name);
			len = offsetof(struct dir_slist_dir_node_disk, name) +
			      strlen(node_data->dir.name);
		}

		sys_put_le32(node_data->dir.head, disk.dir.head);

		break;
	case ZFFS_TYPE_FILE:
		if (len < offsetof(struct dir_slist_file_node_disk, name)) {
			return -EIO;
		}

		if (node_data->file.name) {
			strcpy(disk.file.name, node_data->file.name);
			len = offsetof(struct dir_slist_file_node_disk, name) +
			      strlen(node_data->file.name);
		}

		sys_put_le32(node_data->file.head, disk.file.head);
		sys_put_le32(node_data->file.tail, disk.file.tail);
		sys_put_le32(node_data->file.size, disk.file.size);
		sys_put_le32(node_data->file.next_id, disk.file.next_id);

		break;
	default:
		return -EIO;
	}

	return zffs_slist_updata_ex(zffs, pointer, node_data->id, &disk, len);
}

int zffs_dir_update_node(struct zffs_data *zffs,
			 struct zffs_area_pointer *pointer,
			 const struct zffs_node_data *node_data)
{
	if (node_data->id == ZFFS_ROOT_ID) {
		struct dir_root_disk disk;
		int rc;

		*disk.object_type = ZFFS_OBJECT_TYPE_ROOT;
		sys_put_le32(node_data->dir.head, disk.head);
		sys_put_le16(crc16_ccitt(0, (const void *)&disk,
					 offsetof(struct dir_root_disk, crc)),
			     disk.crc);
		rc = zffs_object_update(zffs, pointer, ZFFS_ROOT_ID,
					sizeof(disk));
		if (rc) {
			return rc;
		}

		return zffs_area_write(zffs, pointer, &disk, sizeof(disk));
	}

	return dir_update_node(zffs, pointer, node_data);
}

static int dir_save(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, struct zffs_dir *dir)
{
	if (dir->list.wait_update) {
		struct zffs_node_data node_data;
		int rc;

		node_data.type = ZFFS_TYPE_DIR;
		node_data.id = dir->id;
		node_data.name = NULL;
		node_data.dir.head = dir->list.head;

		rc = zffs_dir_update_node(zffs, pointer, &node_data);
		if (rc) {
			return rc;
		}

		dir->list.wait_update = false;
	}

	return 0;
}

int zffs_dir_append(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, struct zffs_dir *dir,
		    const struct zffs_node_data *node_data)
{
	struct zffs_slist_node node;
	int rc;

	union {
		zffs_disk(u8_t, type);
		struct dir_slist_dir_node_disk dir;
		struct dir_slist_file_node_disk file;
	} disk;

	node.id = node_data->id;
	node.next = ZFFS_NULL;

	switch (node_data->type) {
	case ZFFS_TYPE_DIR:
		*disk.type = ZFFS_TYPE_DIR;
		sys_put_le32(node_data->dir.head, disk.dir.head);
		memcpy(disk.dir.name, node_data->name, strlen(node_data->name));

		rc = zffs_slist_prepend(
			zffs, pointer, &dir->list, &node, &disk.dir,
			offsetof(struct dir_slist_dir_node_disk, name) +
			strlen(node_data->name));
		break;

	case ZFFS_TYPE_FILE:
		*disk.type = ZFFS_TYPE_FILE;
		sys_put_le32(node_data->file.head, disk.file.head);
		sys_put_le32(node_data->file.tail, disk.file.tail);
		sys_put_le32(node_data->file.size, disk.file.size);
		sys_put_le32(node_data->file.next_id, disk.file.next_id);
		memcpy(disk.file.name, node_data->name,
		       strlen(node_data->name));

		rc = zffs_slist_prepend(
			zffs, pointer, &dir->list, &node, &disk.dir,
			offsetof(struct dir_slist_file_node_disk, name) +
			strlen(node_data->name));
		break;

	default:
		return -EIO;
	}

	return rc;
}

int zffs_dir_unlink(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, struct zffs_dir *dir,
		    const struct zffs_node_data *node_data)
{
	sys_snode_t *snode;
	struct zffs_slist_node node;
	int rc;

	SYS_SLIST_FOR_EACH_NODE(&zffs->opened, snode){
		if (((struct zffs_dir *)snode)->id == node_data->id) {
			return -EBUSY;
		}
	}

	node.id = node_data->id;

	rc = zffs_slist_remove(zffs, pointer, &dir->list, &node);
	if (rc) {
		return rc;
	}

	if (dir->node.id == node_data->id) {
		dir->node = node;
	}

	return 0;
}

static int load_dir(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer,
		    struct zffs_dir_data *data, u32_t len)
{
	int rc = 0;

	zffs_disk(u32_t, head);

	if (len <= sizeof(head)) {
		return -EIO;
	}

	rc = zffs_area_read(zffs, pointer, &head, sizeof(head));
	if (rc) {
		return rc;
	}
	data->head = sys_get_le32(head);

	data->name[len - sizeof(head)] = '\0';

	return zffs_area_read(zffs, pointer, data->name, len - sizeof(head));
}

static int load_file(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer,
		     struct zffs_file_data *data, u32_t len)
{
	struct {
		zffs_disk(u32_t, head);
		zffs_disk(u32_t, tail);
		zffs_disk(u32_t, size);
		zffs_disk(u32_t, next_id);
	} disk;

	int rc = 0;

	if (len <= sizeof(disk)) {
		return -EIO;
	}

	rc = zffs_area_read(zffs, pointer, &disk, sizeof(disk));
	if (rc) {
		return rc;
	}

	data->head = sys_get_le32(disk.head);
	data->tail = sys_get_le32(disk.tail);
	data->size = sys_get_le32(disk.size);
	data->next_id = sys_get_le32(disk.next_id);

	data->name[len - sizeof(disk)] = '\0';

	return zffs_area_read(zffs, pointer, data->name, len - sizeof(disk));
}

int zffs_dir_load_node(struct zffs_data *zffs,
		       struct zffs_area_pointer *pointer, u32_t object_size,
		       struct zffs_node_data *data)
{
	int rc;

	zffs_disk(u8_t, type);

	if (data->id == ZFFS_ROOT_ID) {
		struct dir_root_disk disk;

		if (object_size != sizeof(disk)) {
			return -EIO;
		}

		rc = zffs_area_read(zffs, pointer, &disk, sizeof(disk));
		if (rc) {
			return rc;
		}

		if (*disk.object_type != ZFFS_OBJECT_TYPE_ROOT ||
		    crc16_ccitt(0, (const void *)&disk, sizeof(disk))) {
			return -EIO;
		}

		data->type = ZFFS_TYPE_DIR;
		data->name[0] = '\0';
		data->dir.head = sys_get_le32(disk.head);

		return 0;
	}

	if (object_size < sizeof(type)) {
		return -EIO;
	}

	rc = zffs_area_read(zffs, pointer, type, sizeof(type));
	if (rc) {
		return rc;
	}

	data->type = *type;

	switch (*type) {
	case ZFFS_TYPE_DIR:
		return load_dir(zffs, pointer, &data->dir,
				object_size - sizeof(type));
	case ZFFS_TYPE_FILE:
		return load_file(zffs, pointer, &data->file,
				 object_size - sizeof(type));
	default:
		return -EIO;
	}
}

struct dir_compar_data {
	struct zffs_node_data *node_data;
	sys_snode_t **snode;
};

static int dir_compar(struct zffs_data *zffs,
		      struct zffs_area_pointer *pointer, const void *node,
		      u32_t len, void *data, const void *name)
{
	struct dir_compar_data *dcd = data;
	int rc;

	dcd->node_data->id = ((struct zffs_slist_node *)node)->id;
	rc = zffs_misc_load_node(zffs, pointer, len, dcd->node_data,
				 dcd->snode);
	if (rc) {
		return 1;
	}

	return strcmp(dcd->node_data->name, name) ? 1 : 0;
}

int zffs_dir_search(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer,
		    const struct zffs_dir *dir, const char *name,
		    struct zffs_node_data *node_data, sys_snode_t **snode)
{
	struct zffs_slist_node node;
	struct dir_compar_data data = { .node_data = node_data, .snode = snode };

	return zffs_slist_search(zffs, *pointer, &dir->list, &node, &data,
				 name, dir_compar);
}

int zffs_dir_search_for_node_data(struct zffs_data *zffs,
				  struct zffs_area_pointer *pointer,
				  struct zffs_node_data *node_data,
				  const char *name, sys_snode_t **snode)
{
	int rc;

	if (*name == '\0') {
		struct zffs_area_pointer rp = *pointer;

		rc = zffs_object_open(zffs, &rp, ZFFS_ROOT_ID, NULL);
		if (rc < 0) {
			return rc;
		}

		node_data->id = ZFFS_ROOT_ID;

		rc = zffs_misc_load_node(zffs, &rp, rc, node_data, snode);
	} else if (node_data->type == ZFFS_TYPE_DIR) {
		struct dir_compar_data data = { .node_data = node_data,
						.snode = snode };
		struct zffs_slist list;
		struct zffs_slist_node node;

		zffs_slist_init(&list);

		list.head = node_data->dir.head;

		rc = zffs_slist_search(zffs, *pointer, &list, &node, &data,
				       name, dir_compar);
	} else {
		rc = -ENOTDIR;
	}

	return rc;
}

int zffs_dir_open(struct zffs_data *zffs, struct zffs_area_pointer *pointer,
		  const struct zffs_node_data *node_data,
		  struct zffs_dir *dir)
{
	sys_snode_t *snode;

	if (node_data->type != ZFFS_TYPE_DIR) {
		return -ENOTDIR;
	}

	SYS_SLIST_FOR_EACH_NODE(&zffs->opened, snode){
		if (((struct zffs_dir *)snode)->id == node_data->id) {
			return -EBUSY;
		}
	}

	dir->id = node_data->id;
	dir->list.head = node_data->dir.head;

	dir->list.wait_update = false;
	dir->node.id = ZFFS_NULL;
	dir->node.next = dir->list.head;

	sys_slist_append(&zffs->opened, (void *)dir);

	return 0;
}

int zffs_dir_read(struct zffs_data *zffs, struct zffs_area_pointer *pointer,
		  struct zffs_dir *dir, struct zffs_node_data *data)
{
	int rc;
	sys_snode_t *snode;

	if (zffs_slist_is_tail(&dir->node)) {
		return -ENOENT;
	}

	rc = zffs_slist_next(zffs, pointer, &dir->node);
	if (rc < 0) {
		return rc;
	}

	data->id = dir->node.id;
	return zffs_misc_load_node(zffs, pointer, rc, data, &snode);
}

int zffs_dir_close(struct zffs_data *zffs,
		   struct zffs_area_pointer *pointer, struct zffs_dir *dir)
{
	int rc = dir_save(zffs, pointer, dir);

	if (rc) {
		return rc;
	}

	sys_slist_find_and_remove(&zffs->opened, (void *)dir);

	return 0;
}
