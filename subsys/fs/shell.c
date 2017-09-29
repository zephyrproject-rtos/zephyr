/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <misc/printk.h>
#include <shell/shell.h>
#include <init.h>
#include <fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>

#define BUF_CNT 64

#define MAX_PATH_LEN 128
#define MAX_FILENAME_LEN 128
#define MAX_INPUT_LEN 20

static char cwd[MAX_PATH_LEN] = "/";

static void create_abs_path(const char *name, char *path, size_t len)
{
	if (name[0] == '/') {
		strncpy(path, name, len - 1);
		path[len - 1] = '\0';
	} else {
		if (strcmp(cwd, "/") == 0) {
			snprintf(path, len, "/%s", name);
		} else {
			snprintf(path, len, "%s/%s", cwd, name);
		}
	}
}

static int cmd_mkdir(int argc, char *argv[])
{
	int res;
	char path[MAX_PATH_LEN];

	if (argc < 2) {
		printk("Missing argument\n");
		return 0;
	}

	create_abs_path(argv[1], path, sizeof(path));

	res = fs_mkdir(path);
	if (res) {
		printk("Error creating dir[%d]\n", res);
		return res;
	}

	return 0;
}

static int cmd_rm(int argc, char *argv[])
{
	int err;
	char path[MAX_PATH_LEN];

	if (argc < 2) {
		printk("Missing argument\n");
		return 0;
	}

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_unlink(path);
	if (err) {
		printk("Failed to remove %s (%d)\n", path, err);
		return err;
	}

	return 0;
}

static int cmd_read(int argc, char *argv[])
{
	char path[MAX_PATH_LEN];
	struct fs_dirent dirent;
	fs_file_t file;
	int count;
	off_t offset;
	int err;

	if (argc < 2) {
		printk("Missing argument\n");
		return 0;
	}

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
		printk("Failed to stat %s (%d)\n", path, err);
		return err;
	}

	if (dirent.type != FS_DIR_ENTRY_FILE) {
		return -EINVAL;
	}

	printk("File size: %zd\n", dirent.size);

	err = fs_open(&file, path);
	if (err) {
		printk("Failed to open %s (%d)\n", path, err);
		return err;
	}

	if (offset > 0) {
		fs_seek(&file, offset, FS_SEEK_SET);
	}

	while (count > 0) {
		ssize_t read;
		u8_t buf[16];
		int i;

		read = fs_read(&file, buf, min(count, sizeof(buf)));
		if (read <= 0) {
			break;
		}

		printk("%08X  ", (unsigned) offset);

		for (i = 0; i < read; i++) {
			printk("%02X ", buf[i]);
		}
		for (; i < sizeof(buf); i++) {
			printk("   ");
		}

		printk(" ");

		for (i = 0; i < read; i++) {
			printk("%c", buf[i] < 32 ||
			       buf[i] > 127 ? '.' : buf[i]);
		}

		printk("\n");

		offset += read;
		count -= read;
	}

	fs_close(&file);

	return 0;
}

static int cmd_write(int argc, char *argv[])
{
	char path[MAX_PATH_LEN];
	u8_t buf[BUF_CNT];
	u8_t buf_len;
	int arg_offset;
	fs_file_t file;
	off_t offset = -1;
	int err;

	if (argc < 3) {
		printk("Missing argument\n");
		return 0;
	}

	create_abs_path(argv[1], path, sizeof(path));

	if (!strcmp(argv[2], "-o")) {
		if (argc < 4) {
			printk("Missing argument\n");
			return 0;
		}

		offset = strtol(argv[3], NULL, 0);

		arg_offset = 4;
	} else {
		arg_offset = 2;
	}

	err = fs_open(&file, path);
	if (err) {
		printk("Failed to open %s (%d)\n", path, err);
		return err;
	}

	if (offset < 0) {
		err = fs_seek(&file, 0, FS_SEEK_END);
	} else {
		err = fs_seek(&file, offset, FS_SEEK_SET);
	}
	if (err) {
		printk("Failed to seek %s (%d)\n", path, err);
		fs_close(&file);
		return err;
	}

	buf_len = 0;
	while (arg_offset < argc) {
		buf[buf_len++] = strtol(argv[arg_offset++], NULL, 16);

		if ((buf_len == BUF_CNT) || (arg_offset == argc)) {
			err = fs_write(&file, buf, buf_len);
			if (err < 0) {
				printk("Failed to write %s (%d)\n", path, err);
				fs_close(&file);
				return err;
			}

			buf_len = 0;
		}
	}

	fs_close(&file);

	return 0;
}

static int cmd_ls(int argc, char *argv[])
{
	char path[MAX_PATH_LEN];
	fs_dir_t dir;
	int err;

	if (argc < 2) {
		strcpy(path, cwd);
	} else {
		create_abs_path(argv[1], path, sizeof(path));
	}

	err = fs_opendir(&dir, path);
	if (err) {
		printk("Unable to open %s (err %d)\n", path, err);
		return 0;
	}

	while (1) {
		struct fs_dirent entry;

		err = fs_readdir(&dir, &entry);
		if (err) {
			printk("Unable to read directory\n");
			break;
		}

		/* Check for end of directory listing */
		if (entry.name[0] == '\0') {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("%s/\n", entry.name);
		} else {
			printk("%s\n", entry.name);
		}
	}

	fs_closedir(&dir);

	return 0;
}

static int cmd_pwd(int argc, char *argv[])
{
	printk("%s\n", cwd);
	return 0;
}

static int cmd_cd(int argc, char *argv[])
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
		printk("%s doesn't exist\n", path);
		return 0;
	}

	if (entry.type != FS_DIR_ENTRY_DIR) {
		printk("%s is not a directory\n", path);
		return 0;
	}

	strcpy(cwd, path);

	return 0;
}

static int cmd_trunc(int argc, char *argv[])
{
	char path[MAX_PATH_LEN];
	fs_file_t file;
	int length;
	int err;

	if (argc < 2) {
		printk("Missing argument\n");
		return 0;
	}

	if (argv[1][0] == '/') {
		strncpy(path, argv[1], sizeof(path) - 1);
		path[MAX_PATH_LEN - 1] = '\0';
	} else {
		if (strcmp(cwd, "/") == 0) {
			snprintf(path, sizeof(path), "/%s", argv[1]);
		} else {
			snprintf(path, sizeof(path), "%s/%s", cwd, argv[1]);
		}
	}

	if (argc > 2) {
		length = strtol(argv[2], NULL, 0);
	} else {
		length = 0;
	}

	err = fs_open(&file, path);
	if (err) {
		printk("Failed to open %s (%d)\n", path, err);
		return err;
	}

	err = fs_truncate(&file, length);
	if (err) {
		printk("Failed to truncate %s (%d)\n", path, err);
		fs_close(&file);
		return err;
	}

	fs_close(&file);

	return 0;
}

struct shell_cmd fs_commands[] = {
	{ "ls",    cmd_ls,    "List files in current directory" },
	{ "cd",    cmd_cd,    "Change working directory" },
	{ "pwd",   cmd_pwd,   "Print current working directory" },
	{ "mkdir", cmd_mkdir, "Create directory" },
	{ "rm",    cmd_rm,    "Remove file"},
	{ "read",  cmd_read,  "Read from file" },
	{ "write", cmd_write, "Write to file" },
	{ "trunc", cmd_trunc, "Truncate file" },
	{ NULL, NULL }
};


SHELL_REGISTER("fs", fs_commands);
