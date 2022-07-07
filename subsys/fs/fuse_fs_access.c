/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <libgen.h>
#include <linux/limits.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/types.h>

#include <zephyr/zephyr.h>
#include <zephyr/fs/fs.h>

#include "cmdline.h"
#include "soc.h"

#define S_IRWX_DIR (0775)
#define S_IRW_FILE (0664)

#define NUMBER_OF_OPEN_FILES 128
#define INVALID_FILE_HANDLE (NUMBER_OF_OPEN_FILES + 1)

#define DIR_END '\0'

static struct fs_file_t files[NUMBER_OF_OPEN_FILES];
static uint8_t file_handles[NUMBER_OF_OPEN_FILES];

static pthread_t fuse_thread;

static const char default_fuse_mountpoint[] = "flash";

static const char *fuse_mountpoint;

static ssize_t get_new_file_handle(void)
{
	size_t idx;

	for (idx = 0; idx < ARRAY_SIZE(file_handles); ++idx) {
		if (file_handles[idx] == 0) {
			++file_handles[idx];
			return idx;
		}
	}

	return -ENOMEM;
}

static void release_file_handle(size_t handle)
{
	if (handle < ARRAY_SIZE(file_handles)) {
		--file_handles[handle];
	}
}

static bool is_mount_point(const char *path)
{
	char dir_path[PATH_MAX];

	sprintf(dir_path, "%s", path);
	return strcmp(dirname(dir_path), "/") == 0;
}

static int fuse_fs_access_getattr(const char *path, struct stat *stat)
{
	struct fs_dirent entry;
	int err;

	stat->st_dev = 0;
	stat->st_ino = 0;
	stat->st_nlink = 0;
	stat->st_uid = getuid();
	stat->st_gid = getgid();
	stat->st_rdev = 0;
	stat->st_blksize = 0;
	stat->st_blocks = 0;
	stat->st_atime = 0;
	stat->st_mtime = 0;
	stat->st_ctime = 0;

	if ((strcmp(path, "/") == 0) || is_mount_point(path)) {
		if (strstr(path, "/.") != NULL) {
			return -ENOENT;
		}
		stat->st_mode = S_IFDIR | S_IRWX_DIR;
		stat->st_size = 0;
		return 0;
	}

	err = fs_stat(path, &entry);
	if (err != 0) {
		return err;
	}

	if (entry.type == FS_DIR_ENTRY_DIR) {
		stat->st_mode = S_IFDIR | S_IRWX_DIR;
		stat->st_size = 0;
	} else {
		stat->st_mode = S_IFREG | S_IRW_FILE;
		stat->st_size = entry.size;
	}

	return 0;
}

static int fuse_fs_access_readmount(void *buf, fuse_fill_dir_t filler)
{
	int mnt_nbr = 0;
	const char *mnt_name;
	struct stat stat;
	int err;

	stat.st_dev = 0;
	stat.st_ino = 0;
	stat.st_nlink = 0;
	stat.st_uid = getuid();
	stat.st_gid = getgid();
	stat.st_rdev = 0;
	stat.st_atime = 0;
	stat.st_mtime = 0;
	stat.st_ctime = 0;
	stat.st_mode = S_IFDIR | S_IRWX_DIR;
	stat.st_size = 0;
	stat.st_blksize = 0;
	stat.st_blocks = 0;

	filler(buf, ".", &stat, 0);
	filler(buf, "..", NULL, 0);

	do {
		err = fs_readmount(&mnt_nbr, &mnt_name);
		if (err < 0) {
			break;
		}

		filler(buf, &mnt_name[1], &stat, 0);

	} while (true);

	if (err == -ENOENT) {
		err = 0;
	}

	return err;
}

static int fuse_fs_access_readdir(const char *path, void *buf,
			      fuse_fill_dir_t filler, off_t off,
			      struct fuse_file_info *fi)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	int err;
	struct stat stat;

	ARG_UNUSED(off);
	ARG_UNUSED(fi);

	if (strcmp(path, "/") == 0) {
		return fuse_fs_access_readmount(buf, filler);
	}

	fs_dir_t_init(&dir);

	if (is_mount_point(path)) {
		/* File system API expects trailing slash for a mount point
		 * directory but FUSE strips the trailing slashes from
		 * directory names so add it back.
		 */
		char mount_path[PATH_MAX];

		sprintf(mount_path, "%s/", path);
		err = fs_opendir(&dir, mount_path);
	} else {
		err = fs_opendir(&dir, path);
	}

	if (err) {
		return -ENOEXEC;
	}

	stat.st_dev = 0;
	stat.st_ino = 0;
	stat.st_nlink = 0;
	stat.st_uid = getuid();
	stat.st_gid = getgid();
	stat.st_rdev = 0;
	stat.st_atime = 0;
	stat.st_mtime = 0;
	stat.st_ctime = 0;
	stat.st_mode = S_IFDIR | S_IRWX_DIR;
	stat.st_size = 0;
	stat.st_blksize = 0;
	stat.st_blocks = 0;

	filler(buf, ".", &stat, 0);
	filler(buf, "..", &stat, 0);

	do {
		err = fs_readdir(&dir, &entry);
		if (err) {
			break;
		}

		if (entry.name[0] == DIR_END) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			stat.st_mode = S_IFDIR | S_IRWX_DIR;
			stat.st_size = 0;
		} else {
			stat.st_mode = S_IFREG | S_IRW_FILE;
			stat.st_size = entry.size;
		}

		if (filler(buf, entry.name, &stat, 0)) {
			break;
		}

	} while (1);

	fs_closedir(&dir);

	return err;

}

static int fuse_fs_access_create(const char *path, mode_t mode,
			     struct fuse_file_info *fi)
{
	int err;
	ssize_t handle;

	ARG_UNUSED(mode);

	if (is_mount_point(path)) {
		return -ENOENT;
	}

	handle = get_new_file_handle();
	if (handle < 0) {
		return handle;
	}

	fi->fh = handle;

	err = fs_open(&files[handle], path, FS_O_CREATE | FS_O_WRITE);
	if (err != 0) {
		release_file_handle(handle);
		fi->fh = INVALID_FILE_HANDLE;
		return err;
	}

	return 0;
}

static int fuse_fs_access_open(const char *path, struct fuse_file_info *fi)
{
	return fuse_fs_access_create(path, 0, fi);
}

static int fuse_fs_access_release(const char *path, struct fuse_file_info *fi)
{
	ARG_UNUSED(path);

	if (fi->fh == INVALID_FILE_HANDLE) {
		return -EINVAL;
	}

	fs_close(&files[fi->fh]);

	release_file_handle(fi->fh);

	return 0;
}

static int fuse_fs_access_read(const char *path, char *buf, size_t size,
		off_t off, struct fuse_file_info *fi)
{
	int err;

	ARG_UNUSED(path);

	if (fi->fh == INVALID_FILE_HANDLE) {
		return -EINVAL;
	}

	err = fs_seek(&files[fi->fh], off, FS_SEEK_SET);
	if (err != 0) {
		return err;
	}

	err = fs_read(&files[fi->fh], buf, size);

	return err;
}

static int fuse_fs_access_write(const char *path, const char *buf, size_t size,
		off_t off, struct fuse_file_info *fi)
{
	int err;

	ARG_UNUSED(path);

	if (fi->fh == INVALID_FILE_HANDLE) {
		return -EINVAL;
	}

	err = fs_seek(&files[fi->fh], off, FS_SEEK_SET);
	if (err != 0) {
		return err;
	}

	err = fs_write(&files[fi->fh], buf, size);

	return err;
}

static int fuse_fs_access_ftruncate(const char *path, off_t size,
				struct fuse_file_info *fi)
{
	int err;

	ARG_UNUSED(path);

	if (fi->fh == INVALID_FILE_HANDLE) {
		return -EINVAL;
	}

	err = fs_truncate(&files[fi->fh], size);

	return err;
}

static int fuse_fs_access_truncate(const char *path, off_t size)
{
	int err;
	static struct fs_file_t file;

	err = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (err != 0) {
		return err;
	}

	err = fs_truncate(&file, size);
	if (err != 0) {
		fs_close(&file);
		return err;
	}

	err = fs_close(&file);

	return err;
}

static int fuse_fs_access_mkdir(const char *path, mode_t mode)
{
	ARG_UNUSED(mode);

	return fs_mkdir(path);
}

static int fuse_fs_access_rmdir(const char *path)
{
	return fs_unlink(path);
}

static int fuse_fs_access_unlink(const char *path)
{
	return fs_unlink(path);
}

static int fuse_fs_access_statfs(const char *path, struct statvfs *buf)
{
	ARG_UNUSED(path);
	ARG_UNUSED(buf);
	return 0;
}

static int fuse_fs_access_utimens(const char *path, const struct timespec tv[2])
{
	/* dummy */
	ARG_UNUSED(path);
	ARG_UNUSED(tv);

	return 0;
}


static struct fuse_operations fuse_fs_access_oper = {
	.getattr = fuse_fs_access_getattr,
	.readlink = NULL,
	.getdir = NULL,
	.mknod = NULL,
	.mkdir = fuse_fs_access_mkdir,
	.unlink = fuse_fs_access_unlink,
	.rmdir = fuse_fs_access_rmdir,
	.symlink = NULL,
	.rename = NULL,
	.link = NULL,
	.chmod = NULL,
	.chown = NULL,
	.truncate = fuse_fs_access_truncate,
	.utime = NULL,
	.open = fuse_fs_access_open,
	.read = fuse_fs_access_read,
	.write = fuse_fs_access_write,
	.statfs = fuse_fs_access_statfs,
	.flush = NULL,
	.release = fuse_fs_access_release,
	.fsync = NULL,
	.setxattr = NULL,
	.getxattr = NULL,
	.listxattr = NULL,
	.removexattr = NULL,
	.opendir  = NULL,
	.readdir = fuse_fs_access_readdir,
	.releasedir = NULL,
	.fsyncdir = NULL,
	.init = NULL,
	.destroy = NULL,
	.access = NULL,
	.create = fuse_fs_access_create,
	.ftruncate = fuse_fs_access_ftruncate,
	.fgetattr = NULL,
	.lock = NULL,
	.utimens = fuse_fs_access_utimens,
	.bmap = NULL,
	.flag_nullpath_ok = 0,
	.flag_nopath = 0,
	.flag_utime_omit_ok = 0,
	.flag_reserved = 0,
	.ioctl = NULL,
	.poll = NULL,
	.write_buf = NULL,
	.read_buf = NULL,
	.flock = NULL,
	.fallocate = NULL,
};

static void *fuse_fs_access_main(void *arg)
{
	ARG_UNUSED(arg);

	char *argv[] = {
		"",
		"-f",
		"-s",
		(char *) fuse_mountpoint
	};
	int argc = ARRAY_SIZE(argv);

	posix_print_trace("Mounting flash at %s/\n", fuse_mountpoint);
	fuse_main(argc, argv, &fuse_fs_access_oper, NULL);

	pthread_exit(0);
}

static void fuse_fs_access_init(void)
{
	int err;
	struct stat st;
	size_t i = 0;

	while (i < ARRAY_SIZE(files)) {
		fs_file_t_init(&files[i]);
		++i;
	}

	if (fuse_mountpoint == NULL) {
		fuse_mountpoint = default_fuse_mountpoint;
	}

	if (stat(fuse_mountpoint, &st) < 0) {
		if (mkdir(fuse_mountpoint, 0700) < 0) {
			posix_print_error_and_exit("Failed to create"
				" directory for flash mount point (%s): %s\n",
				fuse_mountpoint, strerror(errno));
		}
	} else if (!S_ISDIR(st.st_mode)) {
		posix_print_error_and_exit("%s is not a directory\n",
					   fuse_mountpoint);

	}

	err = pthread_create(&fuse_thread, NULL, fuse_fs_access_main, NULL);
	if (err < 0) {
		posix_print_error_and_exit(
					"Failed to create thread for "
					"fuse_fs_access_main\n");
	}
}

static void fuse_fs_access_exit(void)
{
	char *full_cmd;
	const char cmd[] = "fusermount -uz ";

	if (fuse_mountpoint == NULL) {
		return;
	}

	full_cmd = malloc(strlen(cmd) + strlen(fuse_mountpoint) + 1);

	sprintf(full_cmd, "%s%s", cmd, fuse_mountpoint);
	if (system(full_cmd) < -1) {
		printf("Failed to unmount fuse mount point\n");
	}
	free(full_cmd);

	pthread_join(fuse_thread, NULL);
}

static void fuse_fs_access_options(void)
{
	static struct args_struct_t fuse_fs_access_options[] = {
		{ .manual = false,
		  .is_mandatory = false,
		  .is_switch = false,
		  .option = "flash-mount",
		  .name = "path",
		  .type = 's',
		  .dest = (void *)&fuse_mountpoint,
		  .call_when_found = NULL,
		  .descript = "Path to the directory where to mount flash" },
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(fuse_fs_access_options);
}

NATIVE_TASK(fuse_fs_access_options, PRE_BOOT_1, 1);
NATIVE_TASK(fuse_fs_access_init, PRE_BOOT_2, 1);
NATIVE_TASK(fuse_fs_access_exit, ON_EXIT, 1);
