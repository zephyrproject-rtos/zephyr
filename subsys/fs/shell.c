/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>

#define STORAGE_PARTITION	storage_partition
#define STORAGE_PARTITION_ID	FIXED_PARTITION_ID(STORAGE_PARTITION)

/* FAT */
#ifdef CONFIG_FAT_FILESYSTEM_ELM
#include <ff.h>
#define FATFS_MNTP      "/RAM:"
/* FatFs work area */
FATFS fat_fs;
/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};
#endif
/* LITTLEFS */
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_data);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &lfs_data,
	.storage_dev = (void *)STORAGE_PARTITION_ID,
};
#endif

#define BUF_CNT 64

#define MAX_PATH_LEN 128
#define MAX_FILENAME_LEN 128
#define MAX_INPUT_LEN 20

#define SHELL_FS    "fs"

/* Maintenance guarantees this begins with '/' and is NUL-terminated. */
static char cwd[MAX_PATH_LEN] = "/";

static void create_abs_path(const char *name, char *path, size_t len)
{
	if (name[0] == '/') {
		strncpy(path, name, len);
		path[len - 1] = '\0';
	} else {
		if (cwd[1] == '\0') {
			__ASSERT_NO_MSG(len >= 2);
			*path++ = '/';
			--len;

			strncpy(path, name, len);
			path[len - 1] = '\0';
		} else {
			strncpy(path, cwd, len);
			path[len - 1] = '\0';

			size_t plen = strlen(path);

			if (plen < len) {
				path += plen;
				*path++ = '/';
				len -= plen + 1U;
				strncpy(path, name, len);
				path[len - 1] = '\0';
			}
		}
	}
}

static int cmd_cd(const struct shell *shell, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_dirent entry;
	int err;

	if (argc < 2) {
		strcpy(cwd, "/");
		return 0;
	}

	if (strcmp(argv[1], "..") == 0) {
		char *prev = strrchr(cwd, '/');

		if (!prev || prev == cwd) {
			strcpy(cwd, "/");
		} else {
			*prev = '\0';
		}

		/* No need to test that a parent exists */
		return 0;
	}

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_stat(path, &entry);
	if (err) {
		shell_error(shell, "%s doesn't exist", path);
		return -ENOEXEC;
	}

	if (entry.type != FS_DIR_ENTRY_DIR) {
		shell_error(shell, "%s is not a directory", path);
		return -ENOEXEC;
	}

	strncpy(cwd, path, sizeof(cwd));
	cwd[sizeof(cwd) - 1] = '\0';

	return 0;
}

static int cmd_ls(const struct shell *shell, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_dir_t dir;
	int err;

	if (argc < 2) {
		strncpy(path, cwd, sizeof(path));
		path[sizeof(path) - 1] = '\0';
	} else {
		create_abs_path(argv[1], path, sizeof(path));
	}

	fs_dir_t_init(&dir);

	err = fs_opendir(&dir, path);
	if (err) {
		shell_error(shell, "Unable to open %s (err %d)", path, err);
		return -ENOEXEC;
	}

	while (1) {
		struct fs_dirent entry;

		err = fs_readdir(&dir, &entry);
		if (err) {
			shell_error(shell, "Unable to read directory");
			break;
		}

		/* Check for end of directory listing */
		if (entry.name[0] == '\0') {
			break;
		}

		shell_print(shell, "%s%s", entry.name,
			      (entry.type == FS_DIR_ENTRY_DIR) ? "/" : "");
	}

	fs_closedir(&dir);

	return 0;
}

static int cmd_pwd(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "%s", cwd);

	return 0;
}

static int cmd_trunc(const struct shell *shell, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_file_t file;
	int length;
	int err;

	create_abs_path(argv[1], path, sizeof(path));

	if (argc > 2) {
		length = strtol(argv[2], NULL, 0);
	} else {
		length = 0;
	}

	fs_file_t_init(&file);
	err = fs_open(&file, path, FS_O_WRITE);
	if (err) {
		shell_error(shell, "Failed to open %s (%d)", path, err);
		return -ENOEXEC;;
	}

	err = fs_truncate(&file, length);
	if (err) {
		shell_error(shell, "Failed to truncate %s (%d)", path, err);
		err = -ENOEXEC;
	}

	fs_close(&file);

	return err;
}

static int cmd_mkdir(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	char path[MAX_PATH_LEN];

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_mkdir(path);
	if (err) {
		shell_error(shell, "Error creating dir[%d]", err);
		err = -ENOEXEC;
	}

	return err;
}

static int cmd_rm(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	char path[MAX_PATH_LEN];

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_unlink(path);
	if (err) {
		shell_error(shell, "Failed to remove %s (%d)", path, err);
		err = -ENOEXEC;
	}

	return err;
}

static int cmd_read(const struct shell *shell, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_dirent dirent;
	struct fs_file_t file;
	int count;
	off_t offset;
	int err;

	create_abs_path(argv[1], path, sizeof(path));

	if (argc > 2) {
		count = strtol(argv[2], NULL, 0);
		if (count <= 0) {
			count = INT_MAX;
		}
	} else {
		count = INT_MAX;
	}

	if (argc > 3) {
		offset = strtol(argv[3], NULL, 0);
	} else {
		offset = 0;
	}

	err = fs_stat(path, &dirent);
	if (err) {
		shell_error(shell, "Failed to obtain file %s (err: %d)",
			    path, err);
		return -ENOEXEC;
	}

	if (dirent.type != FS_DIR_ENTRY_FILE) {
		shell_error(shell, "Not a file %s", path);
		return -ENOEXEC;
	}

	shell_print(shell, "File size: %zd", dirent.size);

	fs_file_t_init(&file);
	err = fs_open(&file, path, FS_O_READ);
	if (err) {
		shell_error(shell, "Failed to open %s (%d)", path, err);
		return -ENOEXEC;
	}

	if (offset > 0) {
		err = fs_seek(&file, offset, FS_SEEK_SET);
		if (err) {
			shell_error(shell, "Failed to seek %s (%d)",
				    path, err);
			fs_close(&file);
			return -ENOEXEC;
		}
	}

	while (count > 0) {
		ssize_t read;
		uint8_t buf[16];
		int i;

		read = fs_read(&file, buf, MIN(count, sizeof(buf)));
		if (read <= 0) {
			break;
		}

		shell_fprintf(shell, SHELL_NORMAL, "%08X  ", (uint32_t)offset);

		for (i = 0; i < read; i++) {
			shell_fprintf(shell, SHELL_NORMAL, "%02X ", buf[i]);
		}
		for (; i < sizeof(buf); i++) {
			shell_fprintf(shell, SHELL_NORMAL, "   ");
		}
		i = sizeof(buf) - i;
		shell_fprintf(shell, SHELL_NORMAL, "%*c", i*3, ' ');

		for (i = 0; i < read; i++) {
			shell_fprintf(shell, SHELL_NORMAL, "%c", buf[i] < 32 ||
				      buf[i] > 127 ? '.' : buf[i]);
		}

		shell_print(shell, "");

		offset += read;
		count -= read;
	}

	fs_close(&file);

	return 0;
}

static int cmd_cat(const struct shell *shell, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	uint8_t buf[BUF_CNT];
	struct fs_dirent dirent;
	struct fs_file_t file;
	int err;
	ssize_t read;

	fs_file_t_init(&file);

	for (size_t i = 1; i < argc; ++i) {
		create_abs_path(argv[i], path, sizeof(path));

		err = fs_stat(path, &dirent);
		if (err < 0) {
			shell_error(shell, "Failed to obtain file %s (err: %d)",
					path, err);
			continue;
		}

		if (dirent.type != FS_DIR_ENTRY_FILE) {
			shell_error(shell, "Not a file %s", path);
			continue;
		}

		err = fs_open(&file, path, FS_O_READ);
		if (err < 0) {
			shell_error(shell, "Failed to open %s (%d)", path, err);
			continue;
		}

		while (true) {
			read = fs_read(&file, buf, sizeof(buf));
			if (read <= 0) {
				break;
			}

			for (int j = 0; j < read; j++) {
				shell_fprintf(shell, SHELL_NORMAL, "%c", buf[j]);
			}
		}

		if (read < 0) {
			shell_error(shell, "Failed to read from file %s (err: %zd)",
				path, read);
		}

		fs_close(&file);
	}

	return 0;
}

static int cmd_statvfs(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	char path[MAX_PATH_LEN];
	struct fs_statvfs stat;

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_statvfs(path, &stat);
	if (err < 0) {
		shell_error(shell, "Failed to statvfs %s (%d)", path, err);
		return -ENOEXEC;
	}

	shell_fprintf(shell, SHELL_NORMAL,
		      "bsize %lu, frsize %lu, blocks %lu, bfree %lu\n",
		      stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);

	return 0;
}

static int cmd_write(const struct shell *shell, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	uint8_t buf[BUF_CNT];
	uint8_t buf_len;
	int arg_offset;
	struct fs_file_t file;
	off_t offset = -1;
	int err;

	create_abs_path(argv[1], path, sizeof(path));

	if (!strcmp(argv[2], "-o")) {
		if (argc < 4) {
			shell_error(shell, "Missing argument");
			return -ENOEXEC;
		}

		offset = strtol(argv[3], NULL, 0);

		arg_offset = 4;
	} else {
		arg_offset = 2;
	}

	fs_file_t_init(&file);
	err = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (err) {
		shell_error(shell, "Failed to open %s (%d)", path, err);
		return -ENOEXEC;
	}

	if (offset < 0) {
		err = fs_seek(&file, 0, FS_SEEK_END);
	} else {
		err = fs_seek(&file, offset, FS_SEEK_SET);
	}
	if (err) {
		shell_error(shell, "Failed to seek %s (%d)", path, err);
		fs_close(&file);
		return -ENOEXEC;
	}

	buf_len = 0U;
	while (arg_offset < argc) {
		buf[buf_len++] = strtol(argv[arg_offset++], NULL, 16);

		if ((buf_len == BUF_CNT) || (arg_offset == argc)) {
			err = fs_write(&file, buf, buf_len);
			if (err < 0) {
				shell_error(shell, "Failed to write %s (%d)",
					      path, err);
				fs_close(&file);
				return -ENOEXEC;
			}

			buf_len = 0U;
		}
	}

	fs_close(&file);

	return 0;
}

#if defined(CONFIG_FAT_FILESYSTEM_ELM)		\
	|| defined(CONFIG_FILE_SYSTEM_LITTLEFS)
static char *mntpt_prepare(char *mntpt)
{
	char *cpy_mntpt;

	cpy_mntpt = k_malloc(strlen(mntpt) + 1);
	if (cpy_mntpt) {
		strcpy(cpy_mntpt, mntpt);
	}
	return cpy_mntpt;
}
#endif

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
static int cmd_mount_fat(const struct shell *shell, size_t argc, char **argv)
{
	char *mntpt;
	int res;

	mntpt = mntpt_prepare(argv[1]);
	if (!mntpt) {
		shell_error(shell,
			    "Failed to allocate  buffer for mount point");
		return -ENOEXEC;
	}

	fatfs_mnt.mnt_point = (const char *)mntpt;
	res = fs_mount(&fatfs_mnt);
	if (res != 0) {
		shell_error(shell,
			"Error mounting FAT fs. Error Code [%d]", res);
		return -ENOEXEC;
	}

	shell_print(shell, "Successfully mounted fat fs:%s",
			fatfs_mnt.mnt_point);

	return 0;
}
#endif

#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)

static int cmd_mount_littlefs(const struct shell *shell, size_t argc, char **argv)
{
	if (littlefs_mnt.mnt_point != NULL) {
		return -EBUSY;
	}

	char *mntpt = mntpt_prepare(argv[1]);

	if (!mntpt) {
		shell_error(shell, "Failed to allocate mount point");
		return -ENOEXEC; /* ?!? */
	}

	littlefs_mnt.mnt_point = mntpt;

	int rc = fs_mount(&littlefs_mnt);

	if (rc != 0) {
		shell_error(shell, "Error mounting as littlefs: %d", rc);
		return -ENOEXEC;
	}

	return rc;
}
#endif

#if defined(CONFIG_FAT_FILESYSTEM_ELM)		\
	|| defined(CONFIG_FILE_SYSTEM_LITTLEFS)
SHELL_STATIC_SUBCMD_SET_CREATE(sub_fs_mount,
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
	SHELL_CMD_ARG(fat, NULL,
		      "Mount fatfs. fs mount fat <mount-point>",
		      cmd_mount_fat, 2, 0),
#endif

#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)
	SHELL_CMD_ARG(littlefs, NULL,
		      "Mount littlefs. fs mount littlefs <mount-point>",
		      cmd_mount_littlefs, 2, 0),
#endif

	SHELL_SUBCMD_SET_END
);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_fs,
	SHELL_CMD(cd, NULL, "Change working directory", cmd_cd),
	SHELL_CMD(ls, NULL, "List files in current directory", cmd_ls),
	SHELL_CMD_ARG(mkdir, NULL, "Create directory", cmd_mkdir, 2, 0),
#if defined(CONFIG_FAT_FILESYSTEM_ELM)		\
	|| defined(CONFIG_FILE_SYSTEM_LITTLEFS)
	SHELL_CMD(mount, &sub_fs_mount,
		  "<Mount fs, syntax:- fs mount <fs type> <mount-point>", NULL),
#endif
	SHELL_CMD(pwd, NULL, "Print current working directory", cmd_pwd),
	SHELL_CMD_ARG(read, NULL, "Read from file", cmd_read, 2, 255),
	SHELL_CMD_ARG(cat, NULL,
		"Concatenate files and print on the standard output",
		cmd_cat, 2, 255),
	SHELL_CMD_ARG(rm, NULL, "Remove file", cmd_rm, 2, 0),
	SHELL_CMD_ARG(statvfs, NULL, "Show file system state", cmd_statvfs, 2, 0),
	SHELL_CMD_ARG(trunc, NULL, "Truncate file", cmd_trunc, 2, 255),
	SHELL_CMD_ARG(write, NULL, "Write file", cmd_write, 3, 255),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(fs, &sub_fs, "File system commands", NULL);
