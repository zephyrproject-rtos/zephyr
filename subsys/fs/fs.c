/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/sys/check.h>


#define LOG_LEVEL CONFIG_FS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fs);

/* list of mounted file systems */
static sys_dlist_t fs_mnt_list;

/* lock to protect mount list operations */
static struct k_mutex mutex;

/* Maps an identifier used in mount points to the file system
 * implementation.
 */
struct registry_entry {
	int type;
	const struct fs_file_system_t *fstp;
};
static struct registry_entry registry[CONFIG_FILE_SYSTEM_MAX_TYPES];

static inline void registry_clear_entry(struct registry_entry *ep)
{
	ep->fstp = NULL;
}

static int registry_add(int type,
			const struct fs_file_system_t *fstp)
{
	int rv = -ENOSPC;

	for (size_t i = 0; i < ARRAY_SIZE(registry); ++i) {
		struct registry_entry *ep = &registry[i];

		if (ep->fstp == NULL) {
			ep->type = type;
			ep->fstp = fstp;
			rv = 0;
			break;
		}
	}

	return rv;
}

static struct registry_entry *registry_find(int type)
{
	for (size_t i = 0; i < ARRAY_SIZE(registry); ++i) {
		struct registry_entry *ep = &registry[i];

		if ((ep->fstp != NULL) && (ep->type == type)) {
			return ep;
		}
	}
	return NULL;
}

static const struct fs_file_system_t *fs_type_get(int type)
{
	struct registry_entry *ep = registry_find(type);

	return (ep != NULL) ? ep->fstp : NULL;
}

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
int fs_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t flags)
{
	struct fs_mount_t *mp;
	int rc = -EINVAL;

	if ((file_name == NULL) ||
			(strlen(file_name) <= 1) || (file_name[0] != '/')) {
		LOG_ERR("invalid file name!!");
		return -EINVAL;
	}

	if (zfp->mp != NULL) {
		return -EBUSY;
	}

	rc = fs_get_mnt_point(&mp, file_name, NULL);
	if (rc < 0) {
		LOG_ERR("mount point not found!!");
		return rc;
	}

	if (((mp->flags & FS_MOUNT_FLAG_READ_ONLY) != 0) &&
	    (flags & FS_O_CREATE || flags & FS_O_WRITE)) {
		return -EROFS;
	}

	CHECKIF(mp->fs->open == NULL) {
		return -ENOTSUP;
	}

	zfp->mp = mp;
	rc = mp->fs->open(zfp, file_name, flags);
	if (rc < 0) {
		LOG_ERR("file open error (%d)", rc);
		zfp->mp = NULL;
		return rc;
	}

	/* Copy flags to zfp for use with other fs_ API calls */
	zfp->flags = flags;

	return rc;
}

int fs_close(struct fs_file_t *zfp)
{
	int rc = -EINVAL;

	if (zfp->mp == NULL) {
		return 0;
	}

	CHECKIF(zfp->mp->fs->close == NULL) {
		return -ENOTSUP;
	}

	rc = zfp->mp->fs->close(zfp);
	if (rc < 0) {
		LOG_ERR("file close error (%d)", rc);
		return rc;
	}

	zfp->mp = NULL;

	return rc;
}

ssize_t fs_read(struct fs_file_t *zfp, void *ptr, size_t size)
{
	int rc = -EINVAL;

	if (zfp->mp == NULL) {
		return -EBADF;
	}

	CHECKIF(zfp->mp->fs->read == NULL) {
		return -ENOTSUP;
	}

	rc = zfp->mp->fs->read(zfp, ptr, size);
	if (rc < 0) {
		LOG_ERR("file read error (%d)", rc);
	}

	return rc;
}

ssize_t fs_write(struct fs_file_t *zfp, const void *ptr, size_t size)
{
	int rc = -EINVAL;

	if (zfp->mp == NULL) {
		return -EBADF;
	}

	CHECKIF(zfp->mp->fs->write == NULL) {
		return -ENOTSUP;
	}

	rc = zfp->mp->fs->write(zfp, ptr, size);
	if (rc < 0) {
		LOG_ERR("file write error (%d)", rc);
	}

	return rc;
}

int fs_seek(struct fs_file_t *zfp, off_t offset, int whence)
{
	int rc = -ENOTSUP;

	if (zfp->mp == NULL) {
		return -EBADF;
	}

	CHECKIF(zfp->mp->fs->lseek == NULL) {
		return -ENOTSUP;
	}

	rc = zfp->mp->fs->lseek(zfp, offset, whence);
	if (rc < 0) {
		LOG_ERR("file seek error (%d)", rc);
	}

	return rc;
}

off_t fs_tell(struct fs_file_t *zfp)
{
	int rc = -ENOTSUP;

	if (zfp->mp == NULL) {
		return -EBADF;
	}

	CHECKIF(zfp->mp->fs->tell == NULL) {
		return -ENOTSUP;
	}

	rc = zfp->mp->fs->tell(zfp);
	if (rc < 0) {
		LOG_ERR("file tell error (%d)", rc);
	}

	return rc;
}

int fs_truncate(struct fs_file_t *zfp, off_t length)
{
	int rc = -EINVAL;

	if (zfp->mp == NULL) {
		return -EBADF;
	}

	CHECKIF(zfp->mp->fs->truncate == NULL) {
		return -ENOTSUP;
	}

	rc = zfp->mp->fs->truncate(zfp, length);
	if (rc < 0) {
		LOG_ERR("file truncate error (%d)", rc);
	}

	return rc;
}

int fs_sync(struct fs_file_t *zfp)
{
	int rc = -EINVAL;

	if (zfp->mp == NULL) {
		return -EBADF;
	}

	CHECKIF(zfp->mp->fs->sync == NULL) {
		return -ENOTSUP;
	}

	rc = zfp->mp->fs->sync(zfp);
	if (rc < 0) {
		LOG_ERR("file sync error (%d)", rc);
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
		LOG_ERR("invalid directory name!!");
		return -EINVAL;
	}

	if (zdp->mp != NULL || zdp->dirp != NULL) {
		return -EBUSY;
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
		LOG_ERR("mount point not found!!");
		return rc;
	}

	CHECKIF(mp->fs->opendir == NULL) {
		return -ENOTSUP;
	}

	zdp->mp = mp;
	rc = zdp->mp->fs->opendir(zdp, abs_path);
	if (rc < 0) {
		zdp->mp = NULL;
		zdp->dirp = NULL;
		LOG_ERR("directory open error (%d)", rc);
	}

	return rc;
}

int fs_readdir(struct fs_dir_t *zdp, struct fs_dirent *entry)
{
	if (zdp->mp) {
		/* Delegate to mounted filesystem */
		int rc = -EINVAL;

		CHECKIF(zdp->mp->fs->readdir == NULL) {
			return  -ENOTSUP;
		}

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

	CHECKIF(zdp->mp->fs->closedir == NULL) {
		return -ENOTSUP;
	}

	rc = zdp->mp->fs->closedir(zdp);
	if (rc < 0) {
		LOG_ERR("directory close error (%d)", rc);
		return rc;
	}

	zdp->mp = NULL;
	zdp->dirp = NULL;
	return rc;
}

/* Filesystem operations */
int fs_mkdir(const char *abs_path)
{
	struct fs_mount_t *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) ||
			(strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		LOG_ERR("invalid directory name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("mount point not found!!");
		return rc;
	}

	if (mp->flags & FS_MOUNT_FLAG_READ_ONLY) {
		return -EROFS;
	}

	CHECKIF(mp->fs->mkdir == NULL) {
		return -ENOTSUP;
	}

	rc = mp->fs->mkdir(mp, abs_path);
	if (rc < 0) {
		LOG_ERR("failed to create directory (%d)", rc);
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
		LOG_ERR("mount point not found!!");
		return rc;
	}

	if (mp->flags & FS_MOUNT_FLAG_READ_ONLY) {
		return -EROFS;
	}

	CHECKIF(mp->fs->unlink == NULL) {
		return -ENOTSUP;
	}

	rc = mp->fs->unlink(mp, abs_path);
	if (rc < 0) {
		LOG_ERR("failed to unlink path (%d)", rc);
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
		LOG_ERR("mount point not found!!");
		return rc;
	}

	if (mp->flags & FS_MOUNT_FLAG_READ_ONLY) {
		return -EROFS;
	}

	/* Make sure both files are mounted on the same path */
	if (strncmp(from, to, match_len) != 0) {
		LOG_ERR("mount point not same!!");
		return -EINVAL;
	}

	CHECKIF(mp->fs->rename == NULL) {
		return -ENOTSUP;
	}

	rc = mp->fs->rename(mp, from, to);
	if (rc < 0) {
		LOG_ERR("failed to rename file or dir (%d)", rc);
	}

	return rc;
}

int fs_stat(const char *abs_path, struct fs_dirent *entry)
{
	struct fs_mount_t *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) ||
			(strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		LOG_ERR("invalid file or dir name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("mount point not found!!");
		return rc;
	}

	CHECKIF(mp->fs->stat == NULL) {
		return -ENOTSUP;
	}

	rc = mp->fs->stat(mp, abs_path, entry);
	if (rc == -ENOENT) {
		/* File doesn't exist, which is a valid stat response */
	} else if (rc < 0) {
		LOG_ERR("failed get file or dir stat (%d)", rc);
	}
	return rc;
}

int fs_statvfs(const char *abs_path, struct fs_statvfs *stat)
{
	struct fs_mount_t *mp;
	int rc;

	if ((abs_path == NULL) ||
			(strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		LOG_ERR("invalid file or dir name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		LOG_ERR("mount point not found!!");
		return rc;
	}

	CHECKIF(mp->fs->statvfs == NULL) {
		return -ENOTSUP;
	}

	rc = mp->fs->statvfs(mp, abs_path, stat);
	if (rc < 0) {
		LOG_ERR("failed get file or dir stat (%d)", rc);
	}

	return rc;
}

int fs_mount(struct fs_mount_t *mp)
{
	struct fs_mount_t *itr;
	const struct fs_file_system_t *fs;
	sys_dnode_t *node;
	int rc = -EINVAL;
	size_t len = 0;

	/* Do all the mp checks prior to locking the mutex on the file
	 * subsystem.
	 */
	if ((mp == NULL) || (mp->mnt_point == NULL)) {
		LOG_ERR("mount point not initialized!!");
		return -EINVAL;
	}

	len = strlen(mp->mnt_point);

	if ((len <= 1) || (mp->mnt_point[0] != '/')) {
		LOG_ERR("invalid mount point!!");
		return -EINVAL;
	}

	k_mutex_lock(&mutex, K_FOREVER);

	/* Check if mount point already exists */
	SYS_DLIST_FOR_EACH_NODE(&fs_mnt_list, node) {
		itr = CONTAINER_OF(node, struct fs_mount_t, node);
		/* continue if length does not match */
		if (len != itr->mountp_len) {
			continue;
		}

		if (strncmp(mp->mnt_point, itr->mnt_point, len) == 0) {
			LOG_ERR("mount point already exists!!");
			rc = -EBUSY;
			goto mount_err;
		}
	}

	/* Get file system information */
	fs = fs_type_get(mp->type);
	if (fs == NULL) {
		LOG_ERR("requested file system type not registered!!");
		rc = -ENOENT;
		goto mount_err;
	}

	CHECKIF(fs->mount == NULL) {
		LOG_ERR("fs type %d does not support mounting", mp->type);
		rc = -ENOTSUP;
		goto mount_err;
	}

	if (fs->unmount == NULL) {
		LOG_WRN("mount path %s is not unmountable",
			log_strdup(mp->mnt_point));
	}

	rc = fs->mount(mp);
	if (rc < 0) {
		LOG_ERR("fs mount error (%d)", rc);
		goto mount_err;
	}

	/* Update mount point data and append it to the list */
	mp->mountp_len = len;
	mp->fs = fs;

	sys_dlist_append(&fs_mnt_list, &mp->node);
	LOG_DBG("fs mounted at %s", log_strdup(mp->mnt_point));

mount_err:
	k_mutex_unlock(&mutex);
	return rc;
}


int fs_unmount(struct fs_mount_t *mp)
{
	int rc = -EINVAL;

	if (mp == NULL) {
		return rc;
	}

	k_mutex_lock(&mutex, K_FOREVER);

	if (mp->fs == NULL) {
		LOG_ERR("fs not mounted (mp == %p)", mp);
		goto unmount_err;
	}

	CHECKIF(mp->fs->unmount == NULL) {
		LOG_ERR("fs unmount not supported!!");
		rc = -ENOTSUP;
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

int fs_readmount(int *index, const char **name)
{
	sys_dnode_t *node;
	int rc = -ENOENT;
	int cnt = 0;
	struct fs_mount_t *itr = NULL;

	*name = NULL;

	k_mutex_lock(&mutex, K_FOREVER);

	SYS_DLIST_FOR_EACH_NODE(&fs_mnt_list, node) {
		if (*index == cnt) {
			itr = CONTAINER_OF(node, struct fs_mount_t, node);
			break;
		}

		++cnt;
	}

	k_mutex_unlock(&mutex);

	if (itr != NULL) {
		rc = 0;
		*name = itr->mnt_point;
		++(*index);
	}

	return rc;

}

/* Register File system */
int fs_register(int type, const struct fs_file_system_t *fs)
{
	int rc = 0;

	k_mutex_lock(&mutex, K_FOREVER);

	if (fs_type_get(type) != NULL) {
		rc = -EALREADY;
	} else {
		rc = registry_add(type, fs);
	}

	k_mutex_unlock(&mutex);

	LOG_DBG("fs register %d: %d", type, rc);

	return rc;
}

/* Unregister File system */
int fs_unregister(int type, const struct fs_file_system_t *fs)
{
	int rc = 0;
	struct registry_entry *ep;

	k_mutex_lock(&mutex, K_FOREVER);

	ep = registry_find(type);
	if ((ep == NULL) || (ep->fstp != fs)) {
		rc = -EINVAL;
	} else {
		registry_clear_entry(ep);
	}

	k_mutex_unlock(&mutex);

	LOG_DBG("fs unregister %d: %d", type, rc);
	return rc;
}

static int fs_init(const struct device *dev)
{
	k_mutex_init(&mutex);
	sys_dlist_init(&fs_mnt_list);
	return 0;
}

SYS_INIT(fs_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
