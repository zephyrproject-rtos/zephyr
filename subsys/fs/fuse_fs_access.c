/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <nsi_errno.h>

#include "cmdline.h"
#include "soc.h"

#include "fuse_fs_access_bottom.h"

#define NUMBER_OF_OPEN_FILES 128

static struct fs_file_t files[NUMBER_OF_OPEN_FILES];
static uint8_t file_handles[NUMBER_OF_OPEN_FILES];

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

static int ffa_stat_top(const char *path, struct ffa_dirent *entry_bottom)
{
	struct fs_dirent entry;
	int err;

	err = fs_stat(path, &entry);

	if (err != 0) {
		return nsi_errno_to_mid(-err);
	}

	entry_bottom->size = entry.size;

	if (entry.type == FS_DIR_ENTRY_DIR) {
		entry_bottom->is_directory = true;
	} else {
		entry_bottom->is_directory = false;
	}

	return 0;
}

static int ffa_readmount_top(int *mnt_nbr, const char **mnt_name)
{
	int err = fs_readmount(mnt_nbr, mnt_name);

	return nsi_errno_to_mid(-err);
}

/* Status shared between readdir_* calls */
static struct {
	struct fs_dir_t dir;
	struct fs_dirent entry;
} readdir_status;

static int ffa_readdir_start(const char *path)
{
	int err;

	fs_dir_t_init(&readdir_status.dir);
	err = fs_opendir(&readdir_status.dir, path);

	return nsi_errno_to_mid(-err);
}

static int ffa_readdir_read_next(struct ffa_dirent *entry_bottom)
{
	int err;

	err = fs_readdir(&readdir_status.dir, &readdir_status.entry);

	if (err) {
		return nsi_errno_to_mid(-err);
	}
	entry_bottom->name = readdir_status.entry.name;
	entry_bottom->size = readdir_status.entry.size;

	if (readdir_status.entry.type == FS_DIR_ENTRY_DIR) {
		entry_bottom->is_directory = true;
	} else {
		entry_bottom->is_directory = false;
	}

	return 0;
}

static void ffa_readdir_end(void)
{
	(void)fs_closedir(&readdir_status.dir);
}

static int ffa_create_top(const char *path, uint64_t *fh)
{
	int err;
	ssize_t handle;

	handle = get_new_file_handle();
	if (handle < 0) {
		return nsi_errno_to_mid(-handle);
	}

	*fh = handle;

	err = fs_open(&files[handle], path, FS_O_CREATE | FS_O_WRITE);
	if (err != 0) {
		release_file_handle(handle);
		*fh = INVALID_FILE_HANDLE;
		return nsi_errno_to_mid(-err);
	}

	return 0;
}

static int ffa_release_top(uint64_t fh)
{
	fs_close(&files[fh]);

	release_file_handle(fh);

	return 0;
}

static int ffa_read_top(uint64_t fh, char *buf, size_t size, off_t off)
{
	int err;

	err = fs_seek(&files[fh], off, FS_SEEK_SET);

	if (err == 0) {
		err = fs_read(&files[fh], buf, size);
	}

	return nsi_errno_to_mid(-err);
}

static int ffa_write_top(uint64_t fh, const char *buf, size_t size, off_t off)
{
	int err;

	err = fs_seek(&files[fh], off, FS_SEEK_SET);

	if (err == 0) {
		err = fs_write(&files[fh], buf, size);
	}

	return nsi_errno_to_mid(-err);
}

static int ffa_ftruncate_top(uint64_t fh, off_t size)
{
	int err = fs_truncate(&files[fh], size);

	return nsi_errno_to_mid(-err);
}

static int ffa_truncate_top(const char *path, off_t size)
{
	int err;
	static struct fs_file_t file;

	err = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (err != 0) {
		return nsi_errno_to_mid(-err);
	}

	err = fs_truncate(&file, size);
	if (err != 0) {
		fs_close(&file);
	} else {
		err = fs_close(&file);
	}

	return nsi_errno_to_mid(-err);
}

static int ffa_mkdir_top(const char *path)
{
	int err = fs_mkdir(path);

	return nsi_errno_to_mid(-err);
}

static int ffa_unlink_top(const char *path)
{
	int err = fs_unlink(path);

	return nsi_errno_to_mid(-err);
}

struct ffa_op_callbacks op_callbacks = {
	.readdir_start = ffa_readdir_start,
	.readdir_read_next = ffa_readdir_read_next,
	.readdir_end = ffa_readdir_end,
	.stat = ffa_stat_top,
	.readmount = ffa_readmount_top,
	.mkdir = ffa_mkdir_top,
	.create = ffa_create_top,
	.release = ffa_release_top,
	.read = ffa_read_top,
	.write = ffa_write_top,
	.ftruncate = ffa_ftruncate_top,
	.truncate = ffa_truncate_top,
	.unlink = ffa_unlink_top,
	.rmdir = ffa_unlink_top,
};

static void fuse_top_dispath_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

#define COOLDOWN_TIME 10
	int cooldown_count = 0;

	while (true) {
		if (ffa_is_op_pended()) {
			ffa_run_pending_op();
			cooldown_count = COOLDOWN_TIME;
		} else {
			if (cooldown_count > 0) {
				k_sleep(K_MSEC(1));
				cooldown_count--;
			} else {
				k_sleep(K_MSEC(20));
			}
		}
	}
}

K_THREAD_DEFINE(fuse_op_handler, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
		fuse_top_dispath_thread, NULL, NULL, NULL, 100, 0, 0);

static void fuse_fs_access_init(void)
{
	size_t i = 0;

	while (i < ARRAY_SIZE(files)) {
		fs_file_t_init(&files[i]);
		++i;
	}

	if (fuse_mountpoint == NULL) {
		fuse_mountpoint = default_fuse_mountpoint;
	}

	ffsa_init_bottom(fuse_mountpoint, &op_callbacks);
}

static void fuse_fs_access_cleanup(void)
{
	if (fuse_mountpoint == NULL) {
		return;
	}

	ffsa_cleanup_bottom(fuse_mountpoint);
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
NATIVE_TASK(fuse_fs_access_cleanup, ON_EXIT, 1);
