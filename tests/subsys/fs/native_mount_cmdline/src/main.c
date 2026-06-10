/*
 * Copyright (c) 2026 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <string.h>

/* Zephyr-side mount points, matching the -volume= command-line arguments */
#define FILE_A "/dir_a/hello.txt"
#define FILE_B "/dir_b/hello.txt"

#define CONTENT_A "hello_from_dir_a"
#define CONTENT_B "hello_from_dir_b"

static int write_file(const char *path, const char *content)
{
	struct fs_file_t file;
	ssize_t ret;

	fs_file_t_init(&file);

	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (ret < 0) {
		printk("Failed to open %s: %d\n", path, (int)ret);
		return (int)ret;
	}

	ret = fs_write(&file, content, strlen(content));
	fs_close(&file);

	if (ret < 0) {
		printk("Failed to write %s: %d\n", path, (int)ret);
		return (int)ret;
	}

	return 0;
}

int main(void)
{
	int ret;

	ret = write_file(FILE_A, CONTENT_A);
	if (ret) {
		return ret;
	}

	ret = write_file(FILE_B, CONTENT_B);
	if (ret) {
		return ret;
	}

	printk("native_mount_cmdline: PASS\n");
	return 0;
}
