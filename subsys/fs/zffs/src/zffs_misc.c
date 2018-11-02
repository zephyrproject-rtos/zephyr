/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/misc.h"
#include "zffs/dir.h"
#include "zffs/file.h"
#include "zffs/object.h"
#include "zffs/path.h"
#include "zffs/tree.h"

void zffs_misc_lock(struct zffs_data *zffs)
{
	k_mutex_lock(&zffs->lock, K_FOREVER);
}

void zffs_misc_unlock(struct zffs_data *zffs)
{
	k_mutex_unlock(&zffs->lock);
}

u32_t zffs_misc_get_id(struct zffs_data *zffs)
{
	int lock = k_mutex_lock(&zffs->lock, K_NO_WAIT);
	int id;

	if (zffs->next_id == ZFFS_ROOT_ID) {
		zffs->next_id += ZFFS_CONFIG_MISC_ID_STEP;
	}

	id = zffs->next_id;
	zffs->next_id += ZFFS_CONFIG_MISC_ID_STEP;

	if (!lock) {
		k_mutex_unlock(&zffs->lock);
	}

	return id;
}

u32_t zffs_misc_next_id(struct zffs_data *zffs, u32_t *id)
{
	if ((*id + 1) % ZFFS_CONFIG_MISC_ID_STEP == 0) {
		*id = zffs_misc_get_id(zffs);
	} else {
		(*id)++;
	}

	return *id;
}

int zffs_misc_restore(struct zffs_data *zffs)
{
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);
	u32_t id, addr;
	u32_t key_count, key_max;
	struct zffs_node_data node_data;
	int rc;

	zffs->data_write_addr = 0;
	zffs->swap_write_addr = 0;

	rc = zffs_tree_init(zffs);
	if (rc) {
		return rc;
	}

	rc = zffs_tree_info(zffs, &key_count, &key_max, NULL,
			    &zffs->data_write_addr);
	if (rc) {
		return rc;
	}

	if (key_count > 0) {
		zffs_area_addr_to_pointer(zffs, zffs->data_write_addr,
					  &pointer);
		rc = zffs_object_check(zffs, &pointer, &id);
		if (rc) {
			// 删除这个键 地址块被遗弃
			key_count--;
			return rc;
		}
	} else {
		zffs->data_write_addr = 0;
		zffs_area_addr_to_pointer(zffs, zffs->data_write_addr,
					  &pointer);
	}

	do {
		addr = zffs_area_pointer_to_addr(zffs, &pointer);
		rc = zffs_object_check(zffs, &pointer, &id);
		if (rc == 0) {
			rc = zffs_tree_insert(zffs, id, addr);

			if (rc == -EEXIST) {
				rc = zffs_tree_update(zffs, id, addr);
			}

			if (rc) {
				return rc;
			}

			if (key_max < id) {
				key_max = id;
			}
			key_count++;
		}
	} while (rc != -ENOENT);

	zffs_area_addr_to_pointer(zffs, addr, &pointer);

	if (key_count) {
		zffs->next_id = (key_max & ~(ZFFS_CONFIG_MISC_ID_STEP - 1)) +
				ZFFS_CONFIG_MISC_ID_STEP;
	} else {
		zffs->next_id = 0;
	}

	rc = zffs_dir_search_for_node_data(zffs, &pointer, &node_data, "",
					   NULL);

	if (rc == -ENOENT) {
		node_data.type = ZFFS_TYPE_DIR;
		node_data.id = ZFFS_ROOT_ID;
		node_data.name = NULL;
		node_data.dir.head = ZFFS_NULL;

		rc = zffs_dir_update_node(zffs, &pointer, &node_data);
	}

	if (rc) {
		return rc;
	}

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);

	return 0;
}

int zffs_misc_load_node(struct zffs_data *zffs,
			struct zffs_area_pointer *pointer, u32_t object_size,
			struct zffs_node_data *data, sys_snode_t **snode)
{
	int rc = zffs_dir_load_node(zffs, pointer, object_size, data);
	sys_snode_t *sn;

	if (!snode || rc) {
		return rc;
	}

	*snode = NULL;

	switch (data->type) {
	case ZFFS_TYPE_DIR:
		SYS_SLIST_FOR_EACH_NODE(&zffs->opened, sn) {
			struct zffs_dir *dir = (struct zffs_dir *)sn;

			if (dir->id == data->id) {
				data->dir.head = dir->list.head;
				*snode = sn;
				break;
			}
		}
		break;
	case ZFFS_TYPE_FILE:
		SYS_SLIST_FOR_EACH_NODE(&zffs->opened, sn) {
			struct zffs_file *file = (struct zffs_file *)sn;

			if (file->id == data->id) {
				data->file.head = file->list.head;
				data->file.tail = file->list.tail;
				data->file.tail = file->list.tail;
				data->file.size = file->size;
				data->file.size = file->next_id;
				*snode = sn;
				break;
			}
		}
		break;
	default:
		rc = -EIO;
	}

	return rc;
}