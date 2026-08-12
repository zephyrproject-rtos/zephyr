/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Write a large repeating text file to a FAT volume on USB MSC.
 */

#include "usb_host_dump.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(msc_large_file, LOG_LEVEL_INF);

#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
#define MSC_FAT_DISK_NAME CONFIG_USBH_MSC_DISK_NAME
#else
#define MSC_FAT_DISK_NAME CONFIG_USB_HOST_MSC_SAMPLE_DISK_NAME
#endif

#define MSC_LARGE_FILE_SIZE_BYTES                                                                  \
	((uint32_t)CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_SIZE_MB * 1024U * 1024U)

#define MSC_LARGE_FILE_PROGRESS_BYTES (1024U * 1024U)

static int msc_large_file_fill_chunk(uint8_t *buf, size_t buflen, uint32_t byte_offset)
{
	size_t pos = 0U;

	while (pos < buflen) {
		int n = snprintf((char *)&buf[pos], buflen - pos, "USB MSC large file offset=%u\n",
				 byte_offset + (uint32_t)pos);

		if (n <= 0) {
			return -EIO;
		}

		if ((size_t)n >= buflen - pos) {
			/* Line truncated: pad remainder with a fixed pattern. */
			memset(&buf[pos], 'X', buflen - pos);
			pos = buflen;
			break;
		}

		pos += (size_t)n;
	}

	return 0;
}

int usb_host_msc_sample_write_large_text_file(void)
{
	struct fs_file_t file;
	char path[80];
	uint8_t chunk[CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_CHUNK_SIZE];
	uint32_t total = MSC_LARGE_FILE_SIZE_BYTES;
	uint32_t written_total = 0U;
	uint32_t next_progress = MSC_LARGE_FILE_PROGRESS_BYTES;
	int64_t start_ms;
	int ret;

	if (total == 0U) {
		return -EINVAL;
	}

	ret = usb_host_msc_sample_fat_mount_volume();
	if (ret != 0) {
		return ret;
	}

	ret = snprintf(path, sizeof(path), "%s/%s", usb_host_msc_sample_fat_mount_point(),
		       CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_NAME);
	if (ret < 0 || ret >= (int)sizeof(path)) {
		ret = -ENOMEM;
		goto unmount;
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (ret != 0) {
		LOG_ERR("msc_large: fs_open %s failed: %d", path, ret);
		goto unmount;
	}

	LOG_INF("msc_large: writing %u bytes (%u MiB) to %s", total,
		(unsigned int)CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_SIZE_MB, path);

	start_ms = k_uptime_get();

	while (written_total < total) {
		uint32_t chunk_len = MIN((uint32_t)sizeof(chunk), total - written_total);
		ssize_t nwrite;

		ret = msc_large_file_fill_chunk(chunk, chunk_len, written_total);
		if (ret != 0) {
			goto close_file;
		}

		nwrite = fs_write(&file, chunk, chunk_len);
		if (nwrite < 0) {
			LOG_ERR("msc_large: fs_write failed at %u: %zd", written_total, nwrite);
			ret = (int)nwrite;
			goto close_file;
		}

		written_total += (uint32_t)nwrite;

		if (written_total >= next_progress) {
			LOG_INF("msc_large: %u / %u bytes written", written_total, total);
			next_progress += MSC_LARGE_FILE_PROGRESS_BYTES;
		}
	}

	ret = fs_sync(&file);
	if (ret != 0) {
		LOG_ERR("msc_large: fs_sync failed: %d", ret);
		goto close_file;
	}

	ret = fs_close(&file);
	if (ret != 0) {
		LOG_ERR("msc_large: fs_close failed: %d", ret);
		goto unmount;
	}

	{
		struct fs_dirent ent;

		ret = fs_stat(path, &ent);
		if (ret != 0) {
			LOG_ERR("msc_large: fs_stat failed: %d", ret);
			goto unmount;
		}

		if (ent.type != FS_DIR_ENTRY_FILE || ent.size != total) {
			LOG_ERR("msc_large: size mismatch (got %zu, want %u)", ent.size, total);
			ret = -EIO;
			goto unmount;
		}
	}

	(void)disk_access_ioctl(MSC_FAT_DISK_NAME, DISK_IOCTL_CTRL_SYNC, NULL);

	LOG_INF("msc_large: done — %u bytes in %lld ms", written_total,
		(long long)(k_uptime_get() - start_ms));

	ret = usb_host_msc_sample_fat_unmount_volume();
	if (ret != 0) {
		LOG_WRN("msc_large: fs_unmount failed: %d", ret);
	}

	return 0;

close_file:
	(void)fs_close(&file);
unmount:
	(void)usb_host_msc_sample_fat_unmount_volume();
	return ret;
}
