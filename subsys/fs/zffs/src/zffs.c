/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/zffs.h"
#include "zffs/area.h"
#include "zffs/dir.h"
#include "zffs/file.h"
#include "zffs/misc.h"
#include "zffs/path.h"
#include <crc16.h>
#include <fs.h>
#include <string.h>

int zffs_mount(struct zffs_data *zffs)
{
	off_t offset = 0, length;
	int rc = 0;

	k_mutex_init(&zffs->lock);

	zffs_misc_lock(zffs);

	sys_slist_init(&zffs->opened);
	zffs->area_num = 0;
	zffs->area[0] = NULL;
	zffs->swap_area[0] = NULL;
	zffs->next_id = 0;

	for (; zffs->area_num < ZFFS_CONFIG_AREA_MAX; offset += length) {
		rc = zffs_area_load(zffs, offset, &length);
		if (rc == -ENOTSUP) {
			goto err;
		} else if (rc) {
			break;
		}
	}

	if (zffs->area_num < ZFFS_CONFIG_AREA_MAX) {
		struct flash_sector s;
		size_t sc = flash_area_get_sector_count(zffs->flash->fa_id);

		rc = flash_area_get_sector_info_by_idx(zffs->flash->fa_id,
						       sc - 1, &s);
		if (rc) {
			goto err;
		}

		size_t area_space = s.fs_off + s.fs_size - offset;
		size_t a_min = (area_space +
				(ZFFS_CONFIG_AREA_MAX - zffs->area_num) - 1) /
			       (ZFFS_CONFIG_AREA_MAX - zffs->area_num);

		while (area_space && zffs->area_num < ZFFS_CONFIG_AREA_MAX) {
			length = 0;
			while (length < a_min &&
			       (rc = flash_area_get_sector_info_by_offs(
					zffs->flash->fa_id, offset + length,
					&s)) == 0) {
				length += s.fs_size;
			}

			if (rc && rc != -EINVAL) {
				goto err;
			}

			if (length > 0) {
				rc = zffs_area_init(zffs, offset, length);
				if (rc) {
					goto err;
				}
				area_space -= length;
				offset += length;
			} else {
				break;
			}
		}
	}

	zffs_misc_unlock(zffs);

	if (zffs->swap_area[0]) {
		// todo
		// 中断了的 gc 需要恢复
		rc = -ESPIPE;
		goto err;
	}

	rc = zffs_area_list_init(zffs, zffs->swap_area,
				 ZFFS_AREA_ID_TYPE_SWAP);
	if (rc) {
		goto err;
	}

	rc = zffs_area_list_init(zffs, zffs->data_area,
				 ZFFS_AREA_ID_TYPE_DATA);
	if (rc) {
		goto err;
	}

	return zffs_misc_restore(zffs);

err:
	zffs->area_num = 0;
	zffs->area[0] = NULL;
	zffs->swap_area[0] = NULL;
	zffs->data_area[0] = NULL;
	zffs->data_write_addr = 0;
	zffs->swap_write_addr = 0;
	zffs->next_id = 0;

	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_mkdir(struct zffs_data *zffs, const char *path)
{
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);
	struct zffs_node_data node_data;
	char node_name[ZFFS_CONFIG_NAME_MAX + 1];
	struct zffs_dir *pdir = NULL;
	struct zffs_dir dir;
	int rc;

	zffs_misc_lock(zffs);

	if (*path != '/') {
		rc = -ESPIPE;
		goto unlock;
	}

	node_data.name = node_name;
	do {
		rc = zffs_path_step(zffs, &pointer, &path, &node_data,
				    (sys_snode_t **)&pdir);
		if (path == NULL) {
			rc = -EEXIST;
			goto unlock;
		}
	} while (rc == 0);

	if (rc == -ECHILD) {
		rc = 0;
	}

	if (rc) {
		goto unlock;
	}

	if (strchr(path, '/')) {
		rc = -ENOENT;
		goto unlock;
	}

	if (pdir == NULL) {
		rc = zffs_dir_open(zffs, &pointer, &node_data, &dir);
		if (rc) {
			goto unlock;
		}
		pdir = &dir;
	}

	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	node_data.name = (char *)path;
	node_data.id = zffs_misc_get_id(zffs);
	node_data.type = ZFFS_TYPE_DIR;
	node_data.dir.head = ZFFS_NULL;

	rc = zffs_dir_append(zffs, &pointer, pdir, &node_data);

	if (pdir == &dir) {
		int buf_rc = zffs_dir_close(zffs, &pointer, pdir);

		if (rc == 0) {
			rc = buf_rc;
		}
	}

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
unlock:
	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_opendir(struct zffs_data *zffs, void *dir, const char *path)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);
	struct zffs_node_data node_data;
	char node_name[ZFFS_CONFIG_NAME_MAX + 1];
	struct zffs_dir *pdir = NULL;

	zffs_misc_lock(zffs);

	if (*path != '/') {
		rc = -ESPIPE;
		goto err;
	}

	node_data.name = node_name;
	do {
		rc = zffs_path_step(zffs, &pointer, &path, &node_data,
				    (sys_snode_t **)&pdir);
		if (path == NULL) {
			if (pdir) {
				rc = -EBUSY;
			}
			break;
		}
	} while (rc == 0);

	if (rc) {
		goto err;
	}

	rc = zffs_dir_open(zffs, &pointer, &node_data, dir);
err:
	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_readdir(struct zffs_data *zffs, void *dir,
		 struct zffs_node_data *node_data)
{
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);
	int rc;

	zffs_misc_lock(zffs);
	rc = zffs_dir_read(zffs, &pointer, dir, node_data);
	zffs_misc_unlock(zffs);

	return rc;
}

int zffs_closedir(struct zffs_data *zffs, void *dir)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);

	zffs_misc_lock(zffs);
	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	rc = zffs_dir_close(zffs, &pointer, dir);

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
	zffs_misc_unlock(zffs);

	return rc;
}

int zffs_open(struct zffs_data *zffs, void *file, const char *path)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);
	struct zffs_node_data node_data;
	char node_name[ZFFS_CONFIG_NAME_MAX + 1];
	struct zffs_dir dir;

	union {
		struct zffs_file *file;
		struct zffs_dir *dir;
	} file_or_dir;

	zffs_misc_lock(zffs);

	if (*path != '/') {
		rc = -ESPIPE;
		goto unlock;
	}

	node_data.name = node_name;
	do {
		rc = zffs_path_step(zffs, &pointer, &path, &node_data,
				    (sys_snode_t **)&file_or_dir);
		if (path == NULL) {
			if (file_or_dir.file) {
				rc = -EBUSY;
			}
			break;
		}
	} while (rc == 0);

	if (rc == -ECHILD) {
		if (strchr(path, '/')) {
			rc = -ENOENT;
			goto unlock;
		}

		if (file_or_dir.dir == NULL) {
			rc = zffs_dir_open(zffs, &pointer, &node_data, &dir);
			if (rc) {
				goto unlock;
			}
			file_or_dir.dir = &dir;
		}

		node_data.id = zffs_misc_get_id(zffs);
		node_data.type = ZFFS_TYPE_FILE;
		node_data.file.name = (char *)path;
		node_data.file.head = ZFFS_NULL;
		node_data.file.tail = ZFFS_NULL;
		node_data.file.size = 0;

		zffs_area_addr_to_pointer(zffs, zffs->data_write_addr,
					  &pointer);

		rc = zffs_file_make(zffs, &pointer, file_or_dir.dir,
				    &node_data, file);

		if (file_or_dir.dir == &dir) {
			int buf_rc =
				zffs_dir_close(zffs, &pointer, file_or_dir.dir);
			if (rc == 0) {
				rc = buf_rc;
			}
		}

		zffs->data_write_addr =
			zffs_area_pointer_to_addr(zffs, &pointer);
	} else {
		rc = zffs_file_open(zffs, &pointer, &node_data, file);
	}

unlock:
	zffs_misc_unlock(zffs);

	return rc;
}

int zffs_sync(struct zffs_data *zffs, void *file)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);

	zffs_misc_lock(zffs);
	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	rc = zffs_file_sync(zffs, &pointer, file);

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
	zffs_misc_unlock(zffs);

	return rc;
}

int zffs_close(struct zffs_data *zffs, void *file)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);

	zffs_misc_lock(zffs);
	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	rc = zffs_file_close(zffs, &pointer, file);

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
	zffs_misc_unlock(zffs);

	return rc;
}

int zffs_write(struct zffs_data *zffs, void *file, const void *data,
	       size_t len)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);

	zffs_misc_lock(zffs);
	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	rc = zffs_file_write(zffs, &pointer, file, data, len);

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_read(struct zffs_data *zffs, void *file, void *data, size_t len)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);

	zffs_misc_lock(zffs);
	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	rc = zffs_file_read(zffs, &pointer, file, data, len);

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
	zffs_misc_unlock(zffs);
	return rc;
}

off_t zffs_tell(struct zffs_data *zffs, void *file)
{
	off_t off;

	zffs_misc_lock(zffs);

	off =  ((struct zffs_file *)file)->offset;

	zffs_misc_unlock(zffs);

	return off;
}

int zffs_lseek(struct zffs_data *zffs, void *file, off_t off, int whence)
{
	int rc, zw;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);

	zffs_misc_lock(zffs);
	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	switch (whence) {
	case FS_SEEK_CUR:
		zw = ZFFS_FILE_SEEK_CUR;
		break;
	case FS_SEEK_END:
		zw = ZFFS_FILE_SEEK_END;
		break;
	case FS_SEEK_SET:
		zw = ZFFS_FILE_SEEK_SET;
		break;
	default:
		return -ESPIPE;
	}

	rc = zffs_file_seek(zffs, &pointer, file, zw, off);

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_stat(struct zffs_data *zffs, const char *path,
	      struct zffs_node_data *node_data)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);

	zffs_misc_lock(zffs);

	if (*path != '/') {
		rc = -ESPIPE;
		goto err;
	}

	do {
		rc = zffs_path_step(zffs, &pointer, &path, node_data, NULL);
		if (path == NULL) {
			break;
		}
	} while (rc == 0);

err:
	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_unlink(struct zffs_data *zffs, const char *path)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);
	struct zffs_node_data node_data;
	char node_name[ZFFS_CONFIG_NAME_MAX + 1];
	struct zffs_dir *pdir = NULL;
	struct zffs_dir dir;

	union {
		struct zffs_file *file;
		struct zffs_dir *dir;
	} file_or_dir;

	zffs_misc_lock(zffs);

	if (*path != '/') {
		rc = -ESPIPE;
		goto err;
	}

	node_data.name = node_name;

	do {
		rc = zffs_path_step(zffs, &pointer, &path, &node_data,
				    (sys_snode_t **)&pdir);
	} while (rc == 0 || strchr(path, '/'));

	if (rc) {
		goto err;
	}

	if (pdir == NULL) {
		rc = zffs_dir_open(zffs, &pointer, &node_data, &dir);
		if (rc) {
			goto err;
		}
		pdir = &dir;
	}

	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	rc = zffs_path_step(zffs, &pointer, &path, &node_data,
			    (sys_snode_t **)&file_or_dir);
	if (rc) {
		goto close_dir;
	}

	if (*(void **)&file_or_dir) {
		rc = -EBUSY;
		goto close_dir;
	}

	if (node_data.type == ZFFS_TYPE_DIR &&
	    node_data.dir.head != ZFFS_NULL) {
		rc = -ENOTEMPTY;
		goto close_dir;
	}

	rc = zffs_dir_unlink(zffs, &pointer, pdir, &node_data);

close_dir:
	if (pdir == &dir) {
		int buf_rc = zffs_dir_close(zffs, &pointer, pdir);
		if (rc == 0) {
			rc = buf_rc;
		}
	}

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
err:
	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_rename(struct zffs_data *zffs, const char *from, const char *to)
{
	int rc;
	struct zffs_area_pointer pointer = zffs_data_pointer(zffs);
	struct zffs_node_data node_data;
	char node_name[ZFFS_CONFIG_NAME_MAX + 1];
	struct zffs_dir *p_from_dir = NULL, *p_to_dir = NULL;
	struct zffs_dir from_dir, to_dir;

	union {
		struct zffs_file *file;
		struct zffs_dir *dir;
	} file_or_dir;

	zffs_misc_lock(zffs);

	if (*from != '/' || *to != '/') {
		rc = -ESPIPE;
		goto err;
	}

	node_data.name = node_name;

	zffs_area_addr_to_pointer(zffs, zffs->data_write_addr, &pointer);

	do {
		rc = zffs_path_step(zffs, &pointer, &to, &node_data,
				    (sys_snode_t **)&p_to_dir);
	} while (rc == 0 || strchr(to, '/'));

	if (rc) {
		goto close_dir;
	}

	if (p_to_dir == NULL) {
		rc = zffs_dir_open(zffs, &pointer, &node_data, &to_dir);
		if (rc) {
			goto close_dir;
		}
		p_to_dir = &to_dir;
	}

	rc = zffs_path_step(zffs, &pointer, &to, &node_data,
			    (sys_snode_t **)&file_or_dir);
	if (rc == 0) {
		rc = -EEXIST;
		goto close_dir;
	} else if (rc != -ECHILD) {
		goto close_dir;
	}

	do {
		rc = zffs_path_step(zffs, &pointer, &from, &node_data,
				    (sys_snode_t **)&p_from_dir);
	} while (rc == 0 || strchr(from, '/'));

	if (rc) {
		goto close_dir;
	}

	if (p_from_dir == NULL) {
		rc = zffs_dir_open(zffs, &pointer, &node_data, &from_dir);
		if (rc) {
			goto close_dir;
		}
		p_from_dir = &from_dir;
	}

	rc = zffs_path_step(zffs, &pointer, &from, &node_data,
			    (sys_snode_t **)&file_or_dir);
	if (rc) {
		goto close_dir;
	}

	rc = zffs_dir_unlink(zffs, &pointer, p_from_dir, &node_data);
	if (rc) {
		goto close_dir;
	}

	rc = zffs_dir_append(zffs, &pointer, p_to_dir, &node_data);

close_dir:
	if (p_from_dir == &from_dir) {
		int buf_rc = zffs_dir_close(zffs, &pointer, p_from_dir);
		if (rc == 0) {
			rc = buf_rc;
		}
	}

	if (p_to_dir == &to_dir) {
		int buf_rc = zffs_dir_close(zffs, &pointer, p_to_dir);
		if (rc == 0) {
			rc = buf_rc;
		}
	}

	zffs->data_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);
err:
	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_unmount(struct zffs_data *zffs)
{
	int rc;

	zffs_misc_lock(zffs);

	rc = sys_slist_is_empty(&zffs->opened) ? 0 : -ESPIPE;

	zffs_misc_unlock(zffs);

	return rc;
}

int zffs_truncate(struct zffs_data *zffs, void *file, off_t length)
{
	return -ESPIPE;
}
