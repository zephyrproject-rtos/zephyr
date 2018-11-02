/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fs.h>
#include <init.h>
#include <stdio.h>
#include <string.h>
#include <zffs/dir.h>
#include <zffs/file.h>
#include <zffs/zffs.h>

K_MEM_SLAB_DEFINE(zffs_dir_pool, sizeof(struct zffs_dir),
		  CONFIG_FS_ZFFS_NUM_DIRS, sizeof(u32_t));
K_MEM_SLAB_DEFINE(zffs_file_pool, sizeof(struct zffs_file),
		  CONFIG_FS_ZFFS_NUM_FILES, sizeof(u32_t));

static int fs_zffs_mount(struct fs_mount_t *mountp)
{
	struct zffs_data *data = mountp->fs_data;

	data->flash = mountp->storage_dev;

	return zffs_mount(data);
}

static int fs_zffs_unmount(struct fs_mount_t *mountp)
{
	return zffs_unmount(mountp->fs_data);
}

static int fs_zffs_statvfs(struct fs_mount_t *mountp, const char *path,
			   struct fs_statvfs *stat)
{
	return -ESPIPE;
}

static int fs_zffs_mkdir(struct fs_mount_t *mountp, const char *fs_path)
{
	int match_len = strlen(mountp->mnt_point);

	return zffs_mkdir(mountp->fs_data, &fs_path[match_len]);
}

static int fs_zffs_opendir(struct fs_dir_t *dirp, const char *fs_path)
{
	int match_len = strlen(dirp->mp->mnt_point);
	int rc;

	if (k_mem_slab_alloc(&zffs_dir_pool, &dirp->dirp, K_NO_WAIT)) {
		dirp->dirp = NULL;
		return -EBUSY;
	}

	rc = zffs_opendir(dirp->mp->fs_data, dirp->dirp, &fs_path[match_len]);
	if (rc) {
		k_mem_slab_free(&zffs_dir_pool, &dirp->dirp);
		dirp->dirp = NULL;
	}

	return rc;
}

int fs_zffs_readdir(struct fs_dir_t *dirp, struct fs_dirent *entry)
{
	int rc;
	struct zffs_node_data node_data;

	node_data.name = entry->name;

	rc = zffs_readdir(dirp->mp->fs_data, dirp->dirp, &node_data);
	if (rc == -ENOENT) {
		entry->name[0] = '\0';
		return 0;
	}

	if (rc) {
		return rc;
	}

	switch (node_data.type) {
	case ZFFS_TYPE_DIR:
		entry->type = FS_DIR_ENTRY_DIR;
		entry->size = 0;
		break;
	case ZFFS_TYPE_FILE:
		entry->type = FS_DIR_ENTRY_FILE;
		entry->size = node_data.file.size;
		break;
	default:
		return -EIO;
	}

	return 0;
}

static int fs_zffs_closedir(struct fs_dir_t *dirp)
{
	int rc;

	rc = zffs_closedir(dirp->mp->fs_data, dirp->dirp);
	if (rc) {
		return rc;
	}

	k_mem_slab_free(&zffs_dir_pool, &dirp->dirp);
	dirp->dirp = NULL;

	return 0;
}

static int fs_zffs_open(struct fs_file_t *filp, const char *fs_path)
{
	int match_len = strlen(filp->mp->mnt_point);
	int rc;

	if (k_mem_slab_alloc(&zffs_file_pool, &filp->filep, K_NO_WAIT)) {
		filp->filep = NULL;
		return -EBUSY;
	}

	rc = zffs_open(filp->mp->fs_data, filp->filep, &fs_path[match_len]);
	if (rc) {
		k_mem_slab_free(&zffs_dir_pool, &filp->filep);
		filp->filep = NULL;
	}

	return rc;
}

static int fs_zffs_write(struct fs_file_t *filp, const void *src,
			 size_t nbytes)
{
	if (filp->filep == NULL) {
		return -ESPIPE;
	}

	return zffs_write(filp->mp->fs_data, filp->filep, src, nbytes);
}

static int fs_zffs_sync(struct fs_file_t *filp)
{
	if (filp->filep == NULL) {
		return -ESPIPE;
	}

	return zffs_sync(filp->mp->fs_data, filp->filep);
}

static int fs_zffs_close(struct fs_file_t *filp)
{
	int rc;

	if (filp->filep == NULL) {
		return -ESPIPE;
	}

	rc = zffs_close(filp->mp->fs_data, filp->filep);
	if (rc) {
		return rc;
	}

	k_mem_slab_free(&zffs_file_pool, &filp->filep);
	filp->filep = NULL;

	return 0;
}

static int fs_zffs_read(struct fs_file_t *filp, void *dest, size_t nbytes)
{
	if (filp->filep == NULL) {
		return -ESPIPE;
	}

	return zffs_read(filp->mp->fs_data, filp->filep, dest, nbytes);
}

static off_t fs_zffs_tell(struct fs_file_t *filp)
{
	if (filp->filep == NULL) {
		return 0;
	}

	return zffs_tell(filp->mp->fs_data, filp->filep);
}

static int fs_zffs_truncate(struct fs_file_t *filp, off_t length)
{
	if (filp->filep == NULL) {
		return -ESPIPE;
	}

	return zffs_truncate(filp->mp->fs_data, filp->filep, length);
}

static int fs_zffs_lseek(struct fs_file_t *filp, off_t off, int whence)
{
	if (filp->filep == NULL) {
		return -ESPIPE;
	}

	return zffs_lseek(filp->mp->fs_data, filp->filep, off, whence);
}

static int fs_zffs_rename(struct fs_mount_t *mountp, const char *from,
			  const char *to)
{
	int match_len = strlen(mountp->mnt_point);

	return zffs_rename(mountp->fs_data, &from[match_len], &to[match_len]);
}

static int fs_zffs_stat(struct fs_mount_t *mountp, const char *path,
			struct fs_dirent *entry)
{
	int match_len = strlen(mountp->mnt_point);
	struct zffs_node_data node_data;
	int rc;

	node_data.name = entry->name;

	rc = zffs_stat(mountp->fs_data, &path[match_len], &node_data);
	if (rc == -ENOENT) {
		entry->name[0] = '\0';
		return 0;
	}

	if (rc) {
		return rc;
	}

	switch (node_data.type) {
	case ZFFS_TYPE_DIR:
		entry->type = FS_DIR_ENTRY_DIR;
		entry->size = 0;
		break;
	case ZFFS_TYPE_FILE:
		entry->type = FS_DIR_ENTRY_FILE;
		entry->size = node_data.file.size;
		break;
	default:
		return -EIO;
	}

	return 0;
}

static int fs_zffs_unlink(struct fs_mount_t *mountp, const char *name)
{
	int match_len = strlen(mountp->mnt_point);

	return zffs_unlink(mountp->fs_data, &name[match_len]);
}

/* File system interface */
static struct fs_file_system_t fs_zffs = {
	.open = fs_zffs_open,
	.read = fs_zffs_read,
	.write = fs_zffs_write,
	.lseek = fs_zffs_lseek,
	.tell = fs_zffs_tell,
	.truncate = fs_zffs_truncate,
	.sync = fs_zffs_sync,
	.close = fs_zffs_close,
	.opendir = fs_zffs_opendir,
	.readdir = fs_zffs_readdir,
	.closedir = fs_zffs_closedir,
	.mount = fs_zffs_mount,
	.unmount = fs_zffs_unmount,
	.unlink = fs_zffs_unlink,
	.rename = fs_zffs_rename,
	.mkdir = fs_zffs_mkdir,
	.stat = fs_zffs_stat,
	.statvfs = fs_zffs_statvfs,
};

static int zffs_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return fs_register(FS_ZFFS, &fs_zffs);
}

SYS_INIT(zffs_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
