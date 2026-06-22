/*
 * Copyright (c) 2025 Golioth, Inc.
 * Copyright (c) 2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>

#include "fs_impl.h"

#include <posix_native_task.h>
#include <nsi_errno.h>
#include <nsi_fcntl.h>
#include <nsi_host_trampolines.h>
#include <cmdline.h>

#include "native_mount_bottom.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(native_mount);

struct native_mount_point {
	struct fs_mount_t mount;
	const char *host;
	const char *zephyr;
	char *tokenized;
};

struct native_mount_dir {
	int fd;
};

struct native_mount_file {
	int fd;
	fs_mode_t zflags;
};

static int native_fs_flags_to_mid(int flags)
{
	int flags_mid = 0;

#define TO_MID(_flag, _flag_mid)					\
	if ((flags & (_flag)) == (_flag)) {				\
		flags &= ~(_flag);					\
		flags_mid |= NSI_FCNTL_MID_##_flag_mid;			\
	}

	TO_MID(FS_O_RDWR, O_RDWR);
	TO_MID(FS_O_READ, O_RDONLY);
	TO_MID(FS_O_WRITE, O_WRONLY);

	TO_MID(FS_O_CREATE, O_CREAT);
	TO_MID(FS_O_APPEND, O_APPEND);
	TO_MID(FS_O_TRUNC, O_TRUNC);

#undef TO_MID

	if (flags != 0) {
		return -NSI_ERRNO_MID_EINVAL;
	}

	return flags_mid;
}

static int native_mount_open(struct fs_file_t *fp, const char *path, fs_mode_t zflags)
{
	const char *host = fp->mp->fs_data;
	const char *path_rel = fs_impl_strip_prefix(path, fp->mp);
	struct native_mount_file *file;
	int fd;

	LOG_DBG("Opening '%s' '%s'", host, path_rel);

	file = nsi_host_malloc(sizeof(*file));
	if (!file) {
		return -ENOMEM;
	}

	fd = nsmb_open(host, path_rel, native_fs_flags_to_mid(zflags));
	if (fd < 0) {
		nsi_host_free(file);
		return -nsi_errno_from_mid(-fd);
	}

	file->fd = fd;
	file->zflags = zflags;
	fp->filep = file;

	return 0;
}

static int native_mount_close(struct fs_file_t *fp)
{
	struct native_mount_file *file = fp->filep;

	LOG_DBG("Closing '%d'", file->fd);

	nsi_host_close(file->fd);
	nsi_host_free(file);

	return 0;
}

static int native_mount_unlink(struct fs_mount_t *mountp, const char *path)
{
	const char *host = mountp->fs_data;
	const char *path_rel = fs_impl_strip_prefix(path, mountp);
	int ret;

	ret = nsmb_unlink(host, path_rel);
	if (ret < 0) {
		return -nsi_errno_from_mid(-ret);
	}

	return 0;
}

static int native_mount_rename(struct fs_mount_t *mountp, const char *from, const char *to)
{
	const char *host = mountp->fs_data;
	const char *from_rel = fs_impl_strip_prefix(from, mountp);
	const char *to_rel = fs_impl_strip_prefix(to, mountp);
	int ret;

	ret = nsmb_rename(host, from_rel, to_rel);
	if (ret < 0) {
		return -nsi_errno_from_mid(-ret);
	}

	return 0;
}

static ssize_t native_mount_read(struct fs_file_t *fp, void *ptr, size_t len)
{
	struct native_mount_file *file = fp->filep;
	long ret;

	if (!(file->zflags & FS_O_READ)) {
		return -EACCES;
	}

	ret = nsi_host_read(file->fd, ptr, len);
	if (ret < 0) {
		return -nsi_host_get_errno();
	}

	return ret;
}

static ssize_t native_mount_write(struct fs_file_t *fp, const void *ptr, size_t len)
{
	struct native_mount_file *file = fp->filep;
	long ret;

	if (!(file->zflags & FS_O_WRITE)) {
		return -EACCES;
	}

	ret = nsi_host_write(file->fd, ptr, len);
	if (ret < 0) {
		return -nsi_host_get_errno();
	}

	return ret;
}

static int native_mount_stat(struct fs_mount_t *mountp, const char *path, struct fs_dirent *dirent)
{
	const char *host = mountp->fs_data;
	const char *path_rel = fs_impl_strip_prefix(path, mountp);
	struct fs_mid_dirent dirent_mid;
	int err;

	LOG_DBG("Stat '%s' '%s'", host, path_rel);

	err = nsmb_stat(host, path_rel, &dirent_mid);
	if (err) {
		return -nsi_errno_from_mid(-err);
	}

	if (dirent_mid.type == FS_MID_DIR_ENTRY_FILE) {
		dirent->type = FS_DIR_ENTRY_FILE;
	} else {
		dirent->type = FS_DIR_ENTRY_DIR;
	}

	strncpy(dirent->name, dirent_mid.name, MAX_FILE_NAME);
	dirent->name[MAX_FILE_NAME] = '\0';

	dirent->size = dirent_mid.size;

	return 0;
}

static int native_mount_statvfs(struct fs_mount_t *mountp, const char *path,
				struct fs_statvfs *stat)
{
	const char *host = mountp->fs_data;
	const char *path_rel = fs_impl_strip_prefix(path, mountp);
	struct fs_mid_statvfs stat_mid;
	int ret;

	ret = nsmb_statvfs(host, path_rel, &stat_mid);
	if (ret < 0) {
		return -nsi_errno_from_mid(-ret);
	}

	stat->f_bsize = stat_mid.f_bsize;
	stat->f_frsize = stat_mid.f_frsize;
	stat->f_blocks = stat_mid.f_blocks;
	stat->f_bfree = stat_mid.f_bfree;

	return 0;
}

static int native_mount_mount(struct fs_mount_t *mountp)
{
	LOG_DBG("%s mounted (host %s)", mountp->mnt_point, (const char *)mountp->fs_data);

	return 0;
}

static int native_mount_unmount(struct fs_mount_t *mountp)
{
	LOG_DBG("%s unmounted", mountp->mnt_point);

	return 0;
}

static int native_mount_whence_to_mid(int whence)
{
	switch (whence) {
	case SEEK_SET:
		return NSMB_SEEK_MID_SET;
	case SEEK_CUR:
		return NSMB_SEEK_MID_CUR;
	case SEEK_END:
		return NSMB_SEEK_MID_END;
	default:
		return -NSI_ERRNO_MID_EINVAL;
	}
}

static off_t native_mount_tell(struct fs_file_t *fp)
{
	struct native_mount_file *file = fp->filep;
	long long ret;

	ret = nsmb_lseek(file->fd, 0, NSMB_SEEK_MID_CUR);
	if (ret < 0) {
		return -nsi_errno_from_mid(-ret);
	}

	return (off_t)ret;
}

static int native_mount_lseek(struct fs_file_t *fp, off_t off, int whence)
{
	struct native_mount_file *file = fp->filep;
	long long ret;

	ret = nsmb_lseek(file->fd, off, native_mount_whence_to_mid(whence));
	if (ret < 0) {
		return -nsi_errno_from_mid(-ret);
	}

	return 0;
}

static int native_mount_sync(struct fs_file_t *fp)
{
	struct native_mount_file *file = fp->filep;
	int ret;

	ret = nsi_host_fsync(file->fd);
	if (ret < 0) {
		return -nsi_host_get_errno();
	}

	return 0;
}

static int native_mount_truncate(struct fs_file_t *fp, off_t length)
{
	struct native_mount_file *file = fp->filep;
	int ret;

	ret = nsi_host_ftruncate(file->fd, length);
	if (ret < 0) {
		return -nsi_host_get_errno();
	}

	return ret;
}

static int native_mount_mkdir(struct fs_mount_t *mountp, const char *path)
{
	const char *host = mountp->fs_data;
	const char *path_rel = fs_impl_strip_prefix(path, mountp);
	int ret;

	ret = nsmb_mkdir(host, path_rel);
	if (ret < 0) {
		return -nsi_errno_from_mid(-ret);
	}

	return 0;
}

static int native_mount_opendir(struct fs_dir_t *dp, const char *path)
{
	const char *host = dp->mp->fs_data;
	struct fs_mid_dir *dir;
	const char *path_rel;

	path_rel = fs_impl_strip_prefix(path, dp->mp);

	dir = nsmb_opendir(host, path_rel);
	if (!dir) {
		return -nsi_host_get_errno();
	}

	dp->dirp = dir;

	return 0;
}

static int native_mount_readdir(struct fs_dir_t *dp, struct fs_dirent *entry)
{
	struct fs_mid_dirent entry_mid;
	int err;

	err = nsmb_readdir(dp->dirp, &entry_mid);
	if (err) {
		return -nsi_errno_from_mid(-err);
	}

	if (entry_mid.name[0] == '\0') {
		entry->name[0] = '\0';
		return 0;
	}

	switch (entry_mid.type) {
	case FS_MID_DIR_ENTRY_DIR:
		entry->type = FS_DIR_ENTRY_DIR;
		break;
	case FS_MID_DIR_ENTRY_FILE:
		entry->type = FS_DIR_ENTRY_FILE;
		break;
	default:
		return -ENOTSUP;
	}

	strncpy(entry->name, entry_mid.name, MAX_FILE_NAME);
	entry->name[MAX_FILE_NAME] = '\0';

	entry->size = entry_mid.size;

	return 0;
}

static int native_mount_closedir(struct fs_dir_t *dp)
{
	struct fs_mid_dir *dir = dp->dirp;
	int ret;

	ret = nsmb_closedir(dir);
	if (ret < 0) {
		return -nsi_host_get_errno();
	}

	return 0;
}

/* File system interface */
static const struct fs_file_system_t native_mount_fs = {
	.open = native_mount_open,
	.close = native_mount_close,
	.read = native_mount_read,
	.write = native_mount_write,
	.unlink = native_mount_unlink,
	.rename = native_mount_rename,
	.stat = native_mount_stat,
	.statvfs = native_mount_statvfs,
	.mount = native_mount_mount,
	.unmount = native_mount_unmount,

	.tell = native_mount_tell,
	.lseek = native_mount_lseek,
	.truncate = native_mount_truncate,
	.sync = native_mount_sync,

	.mkdir = native_mount_mkdir,
	.opendir = native_mount_opendir,
	.readdir = native_mount_readdir,
	.closedir = native_mount_closedir,
};

static struct native_mount_point *native_mount_points;
static size_t native_num_mountpoints;

static int native_mount_init(void)
{
	int err;

	err = fs_register(FS_NATIVE_MOUNT, &native_mount_fs);
	if (err) {
		return err;
	}

	for (struct native_mount_point *mp = native_mount_points;
	     mp < &native_mount_points[native_num_mountpoints]; mp++) {
		LOG_INF("Mounting %s under %s", mp->host, mp->zephyr);

		sys_dnode_init(&mp->mount.node);
		mp->mount.type = FS_NATIVE_MOUNT;
		mp->mount.mnt_point = mp->zephyr;
		mp->mount.fs_data = (void *)mp->host;
		mp->mount.storage_dev = NULL;

		err = fs_mount(&mp->mount);
		if (err) {
			LOG_ERR("Failed to mount: %d", err);
			return err;
		}
	}

	return 0;
}

SYS_INIT(native_mount_init, POST_KERNEL, CONFIG_FILE_SYSTEM_INIT_PRIORITY);

static int native_mount_volume_mount_tokenize(char *str, const char **host, const char **zephyr,
					      char **flags)
{
	*host = str;
	*zephyr = "";
	*flags = "";

	while (*str != '\0' && *str != ':') {
		str++;
	}

	if (*str != ':') {
		return -EINVAL;
	}

	*str = '\0';
	str++;

	*zephyr = str;

	while (*str != '\0' && *str != ':') {
		str++;
	}

	if (*str != ':') {
		return 0;
	}

	*str = '\0';
	str++;

	*flags = str;

	return 0;
}

static void native_mount_volume_found(char *argv, int offset)
{
	const char *option = &argv[offset];
	size_t option_len = strlen(option);
	char *flags;

	native_num_mountpoints++;
	native_mount_points = nsi_host_realloc(
		native_mount_points, native_num_mountpoints * sizeof(*native_mount_points));

	struct native_mount_point *mp = &native_mount_points[native_num_mountpoints - 1];
	char *tokenized = nsi_host_malloc(option_len + 1);

	memcpy(tokenized, option, option_len + 1);

	native_mount_volume_mount_tokenize(tokenized, &mp->host, &mp->zephyr, &flags);
	mp->tokenized = tokenized;
	mp->mount.flags = 0;

	if (strcmp(flags, "ro") == 0) {
		mp->mount.flags = FS_MOUNT_FLAG_READ_ONLY;
	}
}

static void native_mount_add_options(void)
{
	static struct args_struct_t native_mount_options[] = {
		{.option = "volume",
		 .name = "HOST-DIR:ZEPHYR-DIR[:ro]",
		 .type = 's',
		 .call_when_found = native_mount_volume_found,
		 .descript = "Mount HOST-DIR host directory under ZEPHYR-DIR in Zephyr "
			     "(can be provided multiple times)"},
		ARG_TABLE_ENDMARKER};

	native_add_command_line_opts(native_mount_options);
}

static void native_mount_cleanup(void)
{
	for (size_t i = 0; i < native_num_mountpoints; i++) {
		nsi_host_free(native_mount_points[i].tokenized);
	}
	nsi_host_free(native_mount_points);
}

NATIVE_TASK(native_mount_add_options, PRE_BOOT_1, 1);
NATIVE_TASK(native_mount_cleanup, ON_EXIT, 1);
