/*
 * Copyright (c) 2025 Golioth, Inc.
 * Copyright (c) 2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <unistd.h>

#include "nsi_errno.h"
#include "nsi_fcntl.h"
#include "nsi_tracing.h"
#include "native_mount_bottom.h"

struct fs_mid_dir {
	DIR *dir;
	char *path;
};

int nsmb_open(const char *prefix, const char *path, int flags_mid)
{
	char *full_path;
	int ret;

	ret = asprintf(&full_path, "%s%s", prefix, path);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	ret = open(full_path, nsi_fcntl_from_mid(flags_mid), 0640);

	if (ret < 0) {
		nsi_print_warning("%s: Cannot open '%s': %s\n", __func__, full_path,
				  strerror(errno));
		free(full_path);
		return -nsi_errno_to_mid(errno);
	}

	free(full_path);

	return ret;
}

static const char *file_name_from_path(const char *path)
{
	const char *p = path;

	if (p[0] == '/' && p[1] == '\0') {
		return "/";
	}

	while (p[0] != '\0') {
		if (p[0] == '/' && p[1] != '\0') {
			path = p + 1;
		}

		p++;
	}

	return path;
}

int nsmb_stat(const char *prefix, const char *path, struct fs_mid_dirent *dirent_mid)
{
	struct stat st;
	size_t name_len;
	char *full_path;
	int ret;

	ret = asprintf(&full_path, "%s%s", prefix, path);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	ret = stat(full_path, &st);

	free(full_path);

	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	switch (st.st_mode & S_IFMT) {
	case S_IFDIR:
		dirent_mid->type = FS_MID_DIR_ENTRY_DIR;
		break;
	case S_IFREG:
		dirent_mid->type = FS_MID_DIR_ENTRY_FILE;
		break;
	default:
		return -NSI_ERRNO_MID_ENOTSUP;
	}

	strncpy(dirent_mid->name, file_name_from_path(path), NATIVE_MOUNT_MAX_FILE_NAME_LEN);
	dirent_mid->name[NATIVE_MOUNT_MAX_FILE_NAME_LEN] = '\0';
	name_len = strlen(dirent_mid->name);

	/* Remove trailing slash */
	if (dirent_mid->name[name_len - 1] == '/') {
		dirent_mid->name[name_len - 1] = '\0';
	}

	dirent_mid->size = st.st_size;

	return ret;
}

int nsmb_statvfs(const char *prefix, const char *path, struct fs_mid_statvfs *stat_mid)
{
	struct statvfs stat;
	char *full_path;
	int ret;

	ret = asprintf(&full_path, "%s%s", prefix, path);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	ret = statvfs(full_path, &stat);

	free(full_path);

	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	stat_mid->f_bsize = stat.f_bsize;
	stat_mid->f_frsize = stat.f_frsize;
	stat_mid->f_blocks = stat.f_blocks;
	stat_mid->f_bfree = stat.f_bfree;

	return 0;
}

static int nsmb_whence_from_mid(int whence_mid)
{
	switch (whence_mid) {
	case NSMB_SEEK_MID_SET:
		return SEEK_SET;
	case NSMB_SEEK_MID_CUR:
		return SEEK_CUR;
	case NSMB_SEEK_MID_END:
		return SEEK_END;
	default:
		return -NSI_ERRNO_MID_EINVAL;
	}
}

long long nsmb_lseek(int fd, long long offset, int whence_mid)
{
	long long ret;
	int whence;

	whence = nsmb_whence_from_mid(whence_mid);
	if (whence < 0) {
		return whence;
	}

	ret = lseek(fd, offset, whence);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	return ret;
}

int nsmb_mkdir(const char *prefix, const char *path)
{
	char *full_path;
	int ret;

	ret = asprintf(&full_path, "%s%s", prefix, path);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	ret = mkdir(full_path, 0750);

	free(full_path);

	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	return ret;
}

struct fs_mid_dir *nsmb_opendir(const char *prefix, const char *path)
{
	struct fs_mid_dir *dir;
	char *full_path;
	int ret;

	ret = asprintf(&full_path, "%s%s", prefix, path);
	if (ret < 0) {
		return NULL;
	}

	dir = malloc(sizeof(*dir));
	if (!dir) {
		free(full_path);
		return NULL;
	}

	dir->path = full_path;
	dir->dir = opendir(full_path);
	if (!dir->dir) {
		nsi_print_warning("%s: Cannot open host directory '%s': %s\n", __func__, full_path,
				  strerror(errno));
		free(full_path);
		free(dir);
		return NULL;
	}

	return dir;
}

int nsmb_readdir(struct fs_mid_dir *dir, struct fs_mid_dirent *dirent_mid)
{
	struct stat st;
	struct dirent *dirent_host;
	char *full_path;
	size_t name_len;
	int ret;

	errno = 0;

	dirent_host = readdir(dir->dir);
	if (!dirent_host) {
		dirent_mid->name[0] = '\0';
		return -nsi_errno_to_mid(errno);
	}

	switch (dirent_host->d_type) {
	case DT_DIR:
		dirent_mid->type = FS_MID_DIR_ENTRY_DIR;
		break;
	case DT_REG:
		dirent_mid->type = FS_MID_DIR_ENTRY_FILE;
		break;
	default:
		return -NSI_ERRNO_MID_ENOTSUP;
	}

	strncpy(dirent_mid->name, dirent_host->d_name, NATIVE_MOUNT_MAX_FILE_NAME_LEN);
	dirent_mid->name[NATIVE_MOUNT_MAX_FILE_NAME_LEN] = '\0';
	name_len = strlen(dirent_mid->name);

	/* Remove trailing slash */
	if (dirent_mid->name[name_len - 1] == '/') {
		dirent_mid->name[name_len - 1] = '\0';
	}

	if (dirent_host->d_type == DT_DIR) {
		dirent_mid->size = 0;
		return 0;
	}

	ret = asprintf(&full_path, "%s/%s", dir->path, dirent_mid->name);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	ret = stat(full_path, &st);

	free(full_path);

	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	dirent_mid->size = st.st_size;

	return 0;
}

int nsmb_closedir(struct fs_mid_dir *dir)
{
	int ret;

	ret = closedir(dir->dir);
	free(dir->path);
	free(dir);

	return ret;
}

int nsmb_unlink(const char *prefix, const char *path)
{
	struct stat st;
	char *full_path;
	int ret;

	ret = asprintf(&full_path, "%s%s", prefix, path);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	ret = stat(full_path, &st);
	if (ret < 0) {
		nsi_print_warning("%s: Cannot stat '%s': %s\n", __func__, full_path,
				  strerror(errno));
		goto free_full_path;
	}

	if ((st.st_mode & S_IFMT) == S_IFDIR) {
		ret = rmdir(full_path);
	} else {
		ret = unlink(full_path);
	}

free_full_path:
	free(full_path);

	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	return ret;
}

int nsmb_rename(const char *prefix, const char *from, const char *to)
{
	char *from_full;
	char *to_full;
	int ret;

	ret = asprintf(&from_full, "%s%s", prefix, from);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	ret = asprintf(&to_full, "%s%s", prefix, to);
	if (ret < 0) {
		free(from_full);
		return -nsi_errno_to_mid(errno);
	}

	ret = rename(from_full, to_full);

	free(to_full);
	free(from_full);

	if (ret < 0) {
		nsi_print_warning("%s: Cannot rename '%s%s' to '%s%s': %s\n", __func__, prefix,
				  from, prefix, to, strerror(errno));
		return -nsi_errno_to_mid(errno);
	}

	return ret;
}
