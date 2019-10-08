/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>
#include <errno.h>
#include <init.h>
#include <fs/fs.h>


#define LOG_LEVEL CONFIG_FS_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(fs);

/* list of mounted file systems */
static sys_dlist_t fs_mnt_list;

/* lock to protect mount list operations */
static struct k_mutex mutex;

/* file system map table */
static struct fs_file_system_t *fs_map[FS_TYPE_END];

static int fs_get_mnt_point(struct fs_mount_t **mnt_pntp,
			    const char *name, size_t *match_len)
{
	struct fs_mount_t *mnt_p = NULL, *itr;
	size_t longest_match = 0;
	size_t len, name_len = strlen(name);
	sys_dnode_t *node;

	k_mutex_lock(&mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_NODE(&fs_mnt_list, node) {
		itr = CONTAINER_OF(node, struct fs_mount_t, node);
		len = itr->mountp_len;

		/*
		 * Move to next node if mount point length is
		 * shorter than longest_match match or if path
		 * name is shorter than the mount point name.
		 */
		if ((len < longest_match) || (len > name_len)) {
			continue;
		}

		/*
		 * Move to next node if name does not have a directory
		 * separator where mount point name ends.
		 */
		if ((len > 1) && (name[len] != '/') && (name[len] != '\0')) {
			continue;
		}

		/* Check for mount point match */
		if (strncmp(name, itr->mnt_point, len) == 0) {
			mnt_p = itr;
			longest_match = len;
		}
	}
	k_mutex_unlock(&mutex);

	if (mnt_p == NULL) {
		return -ENOENT;
	}

	*mnt_pntp = mnt_p;
	if (match_len)
		*match_len = mnt_p->mountp_len;

	return 0;
}

/* File operations */
int fs_open(struct fs_file_t *zfp, const char *file_name)
{
	struct fs_mount_t *mp;
	int rc = -EINVAL;

	if ((file_name == NULL) ||
			(strlen(file_name) <= 1) || (file_name[0] != '/')) {
		LOG_ERR("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, file_name, NULL);
	if (rc < 0) {
		LOG_ERR("%s:mount point not found!!", __func__);
		return rc;
	}

	zfp->mp = mp;

	if (zfp->mp->fs->open != NULL) {
		rc = zfp->mp->fs->open(zfp, file_name);
		if (rc < 0) {
			LOG_ERR("file open error (%d)", rc);
			return rc;
		}
	}

	return rc;
}

int fs_close(struct fs_file_t *zfp)
{
	int rc = -EINVAL;

	if (zfp->mp->fs->close != NULL) {
		rc = zfp->mp->fs->close(zfp);
		if (rc < 0) {
			LOG_ERR("file close error (%d)", rc);
			return rc;
		}
	}

	zfp->mp = NULL;

	return rc;
}

ssize_t fs_read(struct fs_file_t *zfp, void *ptr, size_t size)
{
	int rc = -EINVAL;

	if (zfp->mp->fs->read != NULL) {
		rc = zfp->mp->fs->read(zfp, ptr, size);
		if (rc < 0) {
			LOG_ERR("file read error (%d)", rc);
		}
	}

	return rc;
}

ssize_t fs_write(struct fs_file_t *zfp, const void *ptr, size_t size)
{
	int rc = -EINVAL;

	if (zfp->mp->fs->write != NULL) {
		rc = zfp->mp->fs->write(zfp, ptr, size);
		if (rc < 0) {
			LOG_ERR("file write error (%d)", rc);
		}
	}

	return rc;
}

int fs_seek(struct fs_file_t *zfp, off_t offset, int whence)
{
	int rc = -EINVAL;

	if (zfp->mp->fs->lseek != NULL) {
		rc = zfp->mp->fs->lseek(zfp, offset, whence);
		if (rc < 0) {
			LOG_ERR("file seek error (%d)", rc);
		}
	}

	return rc;
}

off_t fs_tell(struct fs_file_t *zfp)
{
	int rc = -EINVAL;

	if (zfp->mp->fs->tell != NULL) {
		rc = zfp->mp->fs->tell(zfp);
		if (rc < 0) {
			LOG_ERR("file tell error (%d)", rc);
		}
	}

	return rc;
}

int fs_truncate(struct fs_file_t *zfp, off_t length)
{
	int rc = -EINVAL;

	if (zfp->mp->fs->truncate != NULL) {
		rc = zfp->mp->fs->truncate(zfp, length);
		if (rc < 0) {
			LOG_ERR("file truncate error (%d)", rc);
		}
	}

	return rc;
}

int fs_sync(struct fs_file_t *zfp)
{
	int rc = -EINVAL;

	if (zfp->mp->fs->sync != NULL) {
		rc = zfp->mp->fs->sync(zfp);
		if (rc < 0) {
			LOG_ERR("file sync error (%d)", rc);
		}
	}

	return rc;
}

/* Directory operations */
int fs_opendir(struct fs_dir_t *zdp, const char *abs_path)
{
	struct fs_mount_t *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) ||
			(strlen(abs_path) < 1) || (abs_path[0] != '/')) {
		LOG_ERR("invalid file name!!");
		return -EINVAL;
	}

	if (strcmp(abs_path, "/") == 0) {
		/* Open VFS root dir, marked by zdp->mp == NULL */
		k_mutex_lock(&mutex, K_FOREVER);

		zdp->mp = NULL;
		zdp->dirp = sys_dlist_peek_head(&fs_mnt_list);

		k_mutex_unlock(&mutex);

		return 0;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("%s:mount point not found!!", __func__);
		return rc;
	}

	zdp->mp = mp;

	if (zdp->mp->fs->opendir != NULL) {
		rc = zdp->mp->fs->opendir(zdp, abs_path);
		if (rc < 0) {
			LOG_ERR("directory open error (%d)", rc);
		}
	}

	return rc;
}

int fs_readdir(struct fs_dir_t *zdp, struct fs_dirent *entry)
{
	if (zdp->mp) {
		/* Delegate to mounted filesystem */
		int rc = -EINVAL;

		if (zdp->mp->fs->readdir != NULL) {
			/* Loop until error or not special directory */
			while (true) {
				rc = zdp->mp->fs->readdir(zdp, entry);
				if (rc < 0) {
					break;
				}
				if (entry->name[0] == 0) {
					break;
				}
				if (entry->type != FS_DIR_ENTRY_DIR) {
					break;
				}
				if ((strcmp(entry->name, ".") != 0)
				    && (strcmp(entry->name, "..") != 0)) {
					break;
				}
			}
			if (rc < 0) {
				LOG_ERR("directory read error (%d)", rc);
			}
		}

		return rc;
	}

	/* VFS root dir */
	if (zdp->dirp == NULL) {
		/* No more entries */
		entry->name[0] = 0;
		return 0;
	}

	/* Find the current and next entries in the mount point dlist */
	sys_dnode_t *node, *next = NULL;
	bool found = false;

	k_mutex_lock(&mutex, K_FOREVER);

	SYS_DLIST_FOR_EACH_NODE(&fs_mnt_list, node) {
		if (node == zdp->dirp) {
			found = true;

			/* Pull info from current entry */
			struct fs_mount_t *mnt;

			mnt = CONTAINER_OF(node, struct fs_mount_t, node);

			entry->type = FS_DIR_ENTRY_DIR;
			strncpy(entry->name, mnt->mnt_point + 1,
				sizeof(entry->name) - 1);
			entry->name[sizeof(entry->name) - 1] = 0;
			entry->size = 0;

			/* Save pointer to the next one, for later */
			next = sys_dlist_peek_next(&fs_mnt_list, node);
			break;
		}
	}

	k_mutex_unlock(&mutex);

	if (!found) {
		/* Current entry must have been removed before this
		 * call to readdir -- return an error
		 */
		return -ENOENT;
	}

	zdp->dirp = next;
	return 0;
}

int fs_closedir(struct fs_dir_t *zdp)
{
	int rc = -EINVAL;

	if (zdp->mp == NULL) {
		/* VFS root dir */
		zdp->dirp = NULL;
		return 0;
	}

	if (zdp->mp->fs->closedir != NULL) {
		rc = zdp->mp->fs->closedir(zdp);
		if (rc < 0) {
			LOG_ERR("directory close error (%d)", rc);
			return rc;
		}
	}

	zdp->mp = NULL;
	return rc;
}

/* Filesystem operations */
int fs_mkdir(const char *abs_path)
{
	struct fs_mount_t *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) ||
			(strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		LOG_ERR("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("%s:mount point not found!!", __func__);
		return rc;
	}

	if (mp->fs->mkdir != NULL) {
		rc = mp->fs->mkdir(mp, abs_path);
		if (rc < 0) {
			LOG_ERR("failed to create directory (%d)", rc);
		}
	}

	return rc;
}

int fs_unlink(const char *abs_path)
{
	struct fs_mount_t *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) ||
			(strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		LOG_ERR("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("%s:mount point not found!!", __func__);
		return rc;
	}

	if (mp->fs->unlink != NULL) {
		rc = mp->fs->unlink(mp, abs_path);
		if (rc < 0) {
			LOG_ERR("failed to unlink path (%d)", rc);
		}
	}

	return rc;
}

int fs_rename(const char *from, const char *to)
{
	struct fs_mount_t *mp;
	size_t match_len;
	int rc = -EINVAL;

	if ((from == NULL) || (strlen(from) <= 1) || (from[0] != '/') ||
			(to == NULL) || (strlen(to) <= 1) || (to[0] != '/')) {
		LOG_ERR("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, from, &match_len);
	if (rc < 0) {
		LOG_ERR("%s:mount point not found!!", __func__);
		return rc;
	}

	/* Make sure both files are mounted on the same path */
	if (strncmp(from, to, match_len) != 0) {
		LOG_ERR("mount point not same!!");
		return -EINVAL;
	}

	if (mp->fs->rename != NULL) {
		rc = mp->fs->rename(mp, from, to);
		if (rc < 0) {
			LOG_ERR("failed to rename file or dir (%d)", rc);
		}
	}

	return rc;
}

int fs_stat(const char *abs_path, struct fs_dirent *entry)
{
	struct fs_mount_t *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) ||
			(strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		LOG_ERR("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("%s:mount point not found!!", __func__);
		return rc;
	}

	if (mp->fs->stat != NULL) {
		rc = mp->fs->stat(mp, abs_path, entry);
		if (rc < 0) {
			LOG_ERR("failed get file or dir stat (%d)", rc);
		}
	}
	return rc;
}

int fs_statvfs(const char *abs_path, struct fs_statvfs *stat)
{
	struct fs_mount_t *mp;
	int rc;

	if ((abs_path == NULL) ||
			(strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		LOG_ERR("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("%s:mount point not found!!", __func__);
		return rc;
	}

	if (mp->fs->statvfs != NULL) {
		rc = mp->fs->statvfs(mp, abs_path, stat);
		if (rc < 0) {
			LOG_ERR("failed get file or dir stat (%d)", rc);
		}
	}

	return rc;
}

int fs_mount(struct fs_mount_t *mp)
{
	struct fs_mount_t *itr;
	struct fs_file_system_t *fs;
	sys_dnode_t *node;
	int rc = -EINVAL;

	if ((mp == NULL) || (mp->mnt_point == NULL)) {
		LOG_ERR("mount point not initialized!!");
		return -EINVAL;
	}

	k_mutex_lock(&mutex, K_FOREVER);
	/* Check if requested file system is registered */
	if (mp->type >= FS_TYPE_END ||  fs_map[mp->type] == NULL) {
		LOG_ERR("requested file system not registered!!");
		rc = -ENOENT;
		goto mount_err;
	}

	/* Get fs interface from file system map */
	fs = fs_map[mp->type];
	mp->mountp_len = strlen(mp->mnt_point);

	if ((mp->mnt_point[0] != '/') ||
			(strlen(mp->mnt_point) <= 1)) {
		LOG_ERR("invalid mount point!!");
		rc = -EINVAL;
		goto mount_err;
	}

	if (fs->mount == NULL) {
		LOG_ERR("fs ops functions not set!!");
		rc = -EINVAL;
		goto mount_err;
	}

	/* Check if mount point already exists */
	SYS_DLIST_FOR_EACH_NODE(&fs_mnt_list, node) {
		itr = CONTAINER_OF(node, struct fs_mount_t, node);
		/* continue if length does not match */
		if (mp->mountp_len != itr->mountp_len) {
			continue;
		}

		if (strncmp(mp->mnt_point, itr->mnt_point,
					mp->mountp_len) == 0) {
			LOG_ERR("mount Point already exists!!");
			rc = -EBUSY;
			goto mount_err;
		}
	}


	rc = fs->mount(mp);
	if (rc < 0) {
		LOG_ERR("fs mount error (%d)", rc);
		goto mount_err;
	}

	/* set mount point fs interface */
	mp->fs = fs;

	/*  append to the mount list */
	sys_dlist_append(&fs_mnt_list, &mp->node);
	LOG_DBG("fs mounted at %s", log_strdup(mp->mnt_point));

mount_err:
	k_mutex_unlock(&mutex);
	return rc;
}


int fs_unmount(struct fs_mount_t *mp)
{
	int rc = -EINVAL;

	if ((mp == NULL) || (mp->mnt_point == NULL) ||
				(strlen(mp->mnt_point) <= 1)) {
		LOG_ERR("invalid mount point!!");
		return -EINVAL;
	}

	k_mutex_lock(&mutex, K_FOREVER);
	if ((mp->fs == NULL) || mp->fs->unmount == NULL) {
		LOG_ERR("fs ops functions not set!!");
		rc = -EINVAL;
		goto unmount_err;
	}

	rc = mp->fs->unmount(mp);
	if (rc < 0) {
		LOG_ERR("fs unmount error (%d)", rc);
		goto unmount_err;
	}

	/* clear file system interface */
	mp->fs = NULL;

	/* remove mount node from the list */
	sys_dlist_remove(&mp->node);
	LOG_DBG("fs unmounted from %s", log_strdup(mp->mnt_point));

unmount_err:
	k_mutex_unlock(&mutex);
	return rc;
}

int fs_readmount(int *number, const char **name)
{
	sys_dnode_t *node;
	int rc = -ENOENT;
	int cnt = 0;
	struct fs_mount_t *itr = NULL;

	*name = NULL;

	k_mutex_lock(&mutex, K_FOREVER);

	SYS_DLIST_FOR_EACH_NODE(&fs_mnt_list, node) {
		if (*number == cnt) {
			itr = CONTAINER_OF(node, struct fs_mount_t, node);
			break;
		}

		++cnt;
	}

	k_mutex_unlock(&mutex);

	if (itr != NULL) {
		rc = 0;
		*name = itr->mnt_point;
		++(*number);
	}

	return rc;

}

/* Register File system */
int fs_register(enum fs_type type, struct fs_file_system_t *fs)
{
	int rc = 0;

	k_mutex_lock(&mutex, K_FOREVER);
	if (type >= FS_TYPE_END) {
		LOG_ERR("failed to register File system!!");
		rc = -EINVAL;
		goto reg_err;
	}
	fs_map[type] = fs;
	LOG_DBG("fs registered of type(%u)", type);
reg_err:
	k_mutex_unlock(&mutex);
	return rc;
}

/* Unregister File system */
int fs_unregister(enum fs_type type, struct fs_file_system_t *fs)
{
	int rc = 0;

	k_mutex_lock(&mutex, K_FOREVER);
	if ((type >= FS_TYPE_END) ||
			(fs_map[type] != fs)) {
		LOG_ERR("failed to unregister File system!!");
		rc = -EINVAL;
		goto unreg_err;
	}
	fs_map[type] = NULL;
	LOG_DBG("fs unregistered of type(%u)", type);
unreg_err:
	k_mutex_unlock(&mutex);
	return rc;
}

static int fs_init(struct device *dev)
{
	k_mutex_init(&mutex);
	sys_dlist_init(&fs_mnt_list);
	return 0;
}

SYS_INIT(fs_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
