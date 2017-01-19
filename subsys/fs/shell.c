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

#define MAX_PATH_LEN 128

static char cwd[MAX_PATH_LEN] = "/";

static int cmd_mkdir(int argc, char *argv[])
{
	int res;
	char path[MAX_PATH_LEN];

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
	res = fs_mkdir(path);
	if (res) {
		printk("Error creating dir[%d]\n", res);
		return res;
	}

	return 0;
}

static int cmd_ls(int argc, char *argv[])
{
	char path[MAX_PATH_LEN];
	fs_dir_t dir;
	int err;

	if (argc < 2) {
		strcpy(path, cwd);
	} else if (argv[1][0] == '/') {
		strncpy(path, argv[1], sizeof(path) - 1);
		path[MAX_PATH_LEN - 1] = '\0';
	} else {
		if (strcmp(cwd, "/") == 0) {
			snprintf(path, sizeof(path), "/%s", argv[1]);
		} else {
			snprintf(path, sizeof(path), "%s/%s", cwd, argv[1]);
		}
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

struct shell_cmd fs_commands[] = {
	{ "ls",    cmd_ls,    "List files in current directory" },
	{ "cd",    cmd_cd,    "Change working directory" },
	{ "pwd",   cmd_pwd,   "Print current working directory" },
	{ "mkdir", cmd_mkdir, "Create directory" },
	{ NULL, NULL }
};


SHELL_REGISTER("fs", fs_commands);
