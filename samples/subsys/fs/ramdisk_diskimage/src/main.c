/*
 * Copyright (c) 2025 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <stdio.h>
#include <string.h>

#define DISK_MOUNT_PT	"/RAM:"

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = DISK_MOUNT_PT
};

static void test_file_print(char *path);
static void test_file_write(char *path, char *buf, int len);
static void test_disk_stats(char *mountpt, bool full_stats);

int main(void)
{
	int res;

	res = fs_mount(&mp);
	if (res != 0) {
		printf("Failed to mount test disk image at %s (%d)\n", mp.mnt_point, res);
		return res;
	}

	printf("Test disk image mounted as a RAM disk at %s\n", mp.mnt_point);

	test_disk_stats(DISK_MOUNT_PT, true);

	printf("\nReading existing file 'test.txt':\n");
	test_file_print(DISK_MOUNT_PT "/test.txt");
	printf("\n");

	test_disk_stats(DISK_MOUNT_PT, false);

	printf("\nCreating a new file 'test2.txt'.\n");

	char *content = "This is a newly created test file.";

	test_file_write(DISK_MOUNT_PT "/test2.txt", content, strlen(content));

	printf("Reading the new file 'test2.txt':\n");
	test_file_print(DISK_MOUNT_PT "/test2.txt");

	printf("\n");

	test_disk_stats(DISK_MOUNT_PT, false);

	return 0;
}

static void test_file_print(char *path)
{
	struct fs_file_t file;
	ssize_t read;
	int res;

	fs_file_t_init(&file);

	res = fs_open(&file, path, FS_O_READ);
	if (res < 0) {
		printf("Failed to open %s (%d)\n", path, res);
		return;
	}

	while (true) {
		uint8_t buf[64];

		read = fs_read(&file, buf, sizeof(buf));
		if (read <= 0) {
			break;
		}

		for (int j = 0; j < read; j++) {
			printf("%c", buf[j]);
		}
	}
	printf("\n");

	if (read < 0) {
		printf("Failed to read from file %s (%zd)\n", path, read);
	}

	fs_close(&file);
}

static void test_file_write(char *path, char *buf, int len)
{
	struct fs_file_t file;
	ssize_t written;
	int res;

	fs_file_t_init(&file);

	res = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (res < 0) {
		printf("Failed to open %s (%d)\n", path, res);
		return;
	}

	written = fs_write(&file, buf, len);
	if (written < 0) {
		printf("Failed to write to file %s (%zd)\n", path, written);
	}

	fs_close(&file);
}

static void test_disk_stats(char *mountpt, bool full_stats)
{
	struct fs_statvfs stat;
	int res;

	res = fs_statvfs(DISK_MOUNT_PT, &stat);
	if (res != 0) {
		printf("Failed to statvfs %s (%d)\n", DISK_MOUNT_PT, res);
		return;
	}

	if (full_stats) {
		printf("Disk stats: bsize %lu, frsize %lu, blocks %lu, bfree %lu\n",
			stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);
	} else {
		printf("Disk space in use: %lu bytes\n",
			(stat.f_frsize * (stat.f_blocks - stat.f_bfree)));
	}
}
