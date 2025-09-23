/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/virtiofs.h>
#include <zephyr/kernel.h>

#define VIRTIO_DEV DEVICE_DT_GET(DT_NODELABEL(virtio_pci))

#define MOUNT_POINT "/virtiofs"

struct virtiofs_fs_data fs_data;

static struct fs_mount_t mp = {
	.type = FS_VIRTIOFS,
	.fs_data = &fs_data,
	.flags = 0,
	.storage_dev = (void *)VIRTIO_DEV,
	.mnt_point = MOUNT_POINT,
};

void dirtree(const char *path, int indent)
{
	struct fs_dir_t dir;

	fs_dir_t_init(&dir);
	if (fs_opendir(&dir, path) == 0) {
		while (1) {
			struct fs_dirent entry;

			if (fs_readdir(&dir, &entry) == 0) {
				if (entry.name[0] == '\0') {
					break;
				}
				if (entry.type == FS_DIR_ENTRY_DIR) {
					printf("%*s- %s (type=dir)\n", indent * 2, "", entry.name);
					char *subdir_path = k_malloc(
						strlen(path) + strlen(entry.name) + 2
					);

					if (subdir_path == NULL) {
						printf("failed to allocate subdir path\n");
						continue;
					}
					strcpy(subdir_path, path);
					strcat(subdir_path, "/");
					strcat(subdir_path, entry.name);
					dirtree(subdir_path, indent + 1);
					k_free(subdir_path);
				} else {
					printf(
						"%*s- %s (type=file, size=%zu)\n",
						indent * 2, "", entry.name, entry.size
					);
				}
			} else {
				printf("failed to readdir %s\n", path);
				break;
			}
		};

		fs_closedir(&dir);
	} else {
		printf("failed to opendir %s\n", path);
	}
}

void create_file(const char *path, const char *content)
{
	struct fs_file_t file;

	fs_file_t_init(&file);
	if (fs_open(&file, path, FS_O_CREATE | FS_O_WRITE) == 0) {
		fs_write(&file, content, strlen(content) + 1);
	} else {
		printf("failed to create %s\n", path);
	}
	fs_close(&file);
}

void print_file(const char *path)
{
	struct fs_file_t file;

	fs_file_t_init(&file);
	if (fs_open(&file, path, FS_O_READ) == 0) {
		char buf[256] = "\0";
		int read_c = fs_read(&file, buf, sizeof(buf));

		if (read_c >= 0) {
			buf[read_c] = 0;

			printf(
				"%s content:\n"
				"%s\n",
				path, buf
			);
		} else {
			printf("failed to read from %s\n", path);
		}

		fs_close(&file);
	} else {
		printf("failed to open %s\n", path);
	}
}

int main(void)
{
	if (fs_mount(&mp) == 0) {
		printf("%s directory tree:\n", MOUNT_POINT);
		dirtree(MOUNT_POINT, 0);
		printf("\n");

		print_file(MOUNT_POINT"/file");

		create_file("/virtiofs/file_created_by_zephyr", "hello world\n");

		create_file("/virtiofs/second_file_created_by_zephyr", "lorem ipsum\n");

		fs_mkdir("/virtiofs/dir_created_by_zephyr");
	} else {
		printf("failed to mount %s\n", MOUNT_POINT);
	}

	return 0;
}
