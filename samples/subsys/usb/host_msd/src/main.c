/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbh_msd.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

/* Disk name registered by the MSD driver after device probe */
#define MSD_DISK_NAME        "USBDISK0"
/* FAT mount point - must match VolumeStr[0] with leading slash.
 * translate_path() strips the leading '/' so FatFS sees "USBDISK0:"
 */
#define MSD_MOUNT_POINT      "/USBDISK0:"
/* Test file written and read back to verify read/write path */
#define MSD_TEST_FILE        MSD_MOUNT_POINT "/ZEPHYR.TXT"
#define MSD_TEST_CONTENT     "Hello from Zephyr USB Host MSD!\n"
/* Sector offset from end of disk used for raw read/write test */
#define MSD_TEST_SECTOR_OFFSET  8U

static FATFS s_fat_fs;

static struct fs_mount_t s_mp = {
	.type      = FS_FATFS,
	.fs_data   = &s_fat_fs,
	.mnt_point = MSD_MOUNT_POINT,
};

static bool s_mounted;

/* Wait for USB MSD device to appear and report DISK_STATUS_OK */
static void wait_for_msd_connection(void)
{
	int status;

	while (true) {
		status = disk_access_status(MSD_DISK_NAME);
		if (status == DISK_STATUS_OK) {
			LOG_INF("USB MSD device connected!");
			return;
		}
		k_sleep(K_MSEC(10));
	}
}

/* Print device information retrieved via IOCTL */
static void log_msd_info(void)
{
	uint32_t sector_count = 0;
	uint32_t sector_size  = 0;
	uint64_t capacity_mb;

	disk_access_ioctl(MSD_DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	disk_access_ioctl(MSD_DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE,  &sector_size);

	capacity_mb = (uint64_t)sector_count * sector_size / (1024ULL * 1024ULL);

	LOG_INF("=== USB Mass Storage Device Information ===");
	LOG_INF("  Disk Name:    %s", MSD_DISK_NAME);
	LOG_INF("  Sector Size:  %u bytes", sector_size);
	LOG_INF("  Sector Count: %u", sector_count);
	LOG_INF("  Capacity:     %u MB", (uint32_t)capacity_mb);
}

/* List contents of the mount point root directory */
static void list_root_directory(void)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	int ret;

	fs_dir_t_init(&dir);

	ret = fs_opendir(&dir, MSD_MOUNT_POINT);
	if (ret) {
		LOG_ERR("fs_opendir failed: %d", ret);
		return;
	}

	LOG_INF("=== Directory Listing: %s ===", MSD_MOUNT_POINT);

	while (true) {
		ret = fs_readdir(&dir, &entry);
		if (ret || entry.name[0] == '\0') {
			break;
		}
		LOG_INF("  [%s] %-16s %u bytes",
			entry.type == FS_DIR_ENTRY_DIR ? "DIR " : "FILE",
			entry.name, (uint32_t)entry.size);
	}

	fs_closedir(&dir);
}

/* Write a small test file to the file system */
static int write_test_file(void)
{
	struct fs_file_t fp;
	int ret;

	fs_file_t_init(&fp);

	LOG_INF("=== Writing Test File: %s ===", MSD_TEST_FILE);

	ret = fs_open(&fp, MSD_TEST_FILE, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (ret) {
		LOG_ERR("fs_open (write) failed: %d", ret);
		return ret;
	}

	ret = fs_write(&fp, MSD_TEST_CONTENT, strlen(MSD_TEST_CONTENT));
	fs_close(&fp);

	if (ret < 0) {
		LOG_ERR("fs_write failed: %d", ret);
		return ret;
	}

	LOG_INF("  Wrote %d bytes", ret);
	return 0;
}

/* Read back the test file and verify its content */
static int verify_test_file(void)
{
	struct fs_file_t fp;
	char buf[64];
	int ret;

	fs_file_t_init(&fp);

	LOG_INF("=== Verifying Test File: %s ===", MSD_TEST_FILE);

	ret = fs_open(&fp, MSD_TEST_FILE, FS_O_READ);
	if (ret) {
		LOG_ERR("fs_open (read) failed: %d", ret);
		return ret;
	}

	memset(buf, 0, sizeof(buf));
	ret = fs_read(&fp, buf, sizeof(buf) - 1);
	fs_close(&fp);

	if (ret < 0) {
		LOG_ERR("fs_read failed: %d", ret);
		return ret;
	}

	if (strcmp(buf, MSD_TEST_CONTENT) == 0) {
		LOG_INF("  Content verified OK");
		return 0;
	}

	LOG_ERR("  Content mismatch!");
	LOG_ERR("  Expected: '%s'", MSD_TEST_CONTENT);
	LOG_ERR("  Got:      '%s'", buf);
	return -EIO;
}

/* Perform a raw sector read/write round-trip test (bypasses FS layer) */
static int raw_sector_test(void)
{
	uint32_t sector_count = 0;
	uint32_t sector_size  = 0;
	uint32_t test_lba;
	uint8_t *wbuf;
	uint8_t *rbuf;
	uint32_t i;
	int ret;

	disk_access_ioctl(MSD_DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	disk_access_ioctl(MSD_DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE,  &sector_size);

	if (sector_size == 0 || sector_count < MSD_TEST_SECTOR_OFFSET + 1U) {
		LOG_WRN("Disk too small for raw sector test, skipping");
		return 0;
	}

	wbuf = k_malloc(sector_size);
	rbuf = k_malloc(sector_size);

	if (!wbuf || !rbuf) {
		LOG_ERR("k_malloc failed for sector buffers");
		k_free(wbuf);
		k_free(rbuf);
		return -ENOMEM;
	}

	/* Use a sector near the end to avoid overwriting the FS metadata */
	test_lba = sector_count - MSD_TEST_SECTOR_OFFSET;

	/* Fill write pattern */
	for (i = 0; i < sector_size; i++) {
		wbuf[i] = (uint8_t)(i & 0xFF);
	}

	LOG_INF("=== Raw Sector Read/Write Test (LBA %u) ===", test_lba);

	ret = disk_access_write(MSD_DISK_NAME, wbuf, test_lba, 1);
	if (ret) {
		LOG_ERR("  Sector write failed: %d", ret);
		goto done;
	}

	disk_access_ioctl(MSD_DISK_NAME, DISK_IOCTL_CTRL_SYNC, NULL);

	memset(rbuf, 0, sector_size);
	ret = disk_access_read(MSD_DISK_NAME, rbuf, test_lba, 1);
	if (ret) {
		LOG_ERR("  Sector read failed: %d", ret);
		goto done;
	}

	if (memcmp(wbuf, rbuf, sector_size) == 0) {
		LOG_INF("  Read/Write round-trip OK (%u bytes)", sector_size);
	} else {
		LOG_ERR("  Data mismatch after read-back!");
		ret = -EIO;
	}

done:
	k_free(wbuf);
	k_free(rbuf);
	return ret;
}

/* Mount the FAT file system and run FS-level tests */
static int mount_and_test(void)
{
	int ret;

	s_mp.storage_dev = (void *)MSD_DISK_NAME;

	ret = fs_mount(&s_mp);
	if (ret) {
		LOG_ERR("fs_mount failed: %d (is disk FAT formatted?)", ret);
		return ret;
	}

	s_mounted = true;
	LOG_INF("FAT file system mounted at %s", MSD_MOUNT_POINT);

	list_root_directory();

	ret = write_test_file();
	if (ret == 0) {
		verify_test_file();
	}

	return 0;
}

/* Monitor connection and return when device is removed */
static int monitor_connection(void)
{
	uint32_t elapsed_seconds = 0U;

	while (true) {
		k_sleep(K_SECONDS(1));
		elapsed_seconds++;

		if (disk_access_status(MSD_DISK_NAME) != DISK_STATUS_OK) {
			return -ENODEV;
		}

		if (elapsed_seconds % 10U == 0U) {
			LOG_INF("MSD connected: %u seconds elapsed",
				elapsed_seconds);
		}
	}
}

int main(void)
{
	int err;

	LOG_INF("===========================================");
	LOG_INF("USB Host Mass Storage Class Sample Application");
	LOG_INF("===========================================");

	err = usbh_init(&uhs_ctx);
	if (err) {
		LOG_ERR("Failed to initialize USB host support: %d", err);
		return err;
	}

	err = usbh_enable(&uhs_ctx);
	if (err) {
		LOG_ERR("Failed to enable USB host support: %d", err);
		return err;
	}

	err = uhc_sof_enable(uhs_ctx.dev);
	if (err) {
		LOG_ERR("Failed to start SoF");
		return err;
	}

	LOG_INF("USB host enabled, please connect a USB flash drive");

	while (true) {
		s_mounted = false;

		LOG_INF("Waiting for USB MSD device to connect...");
		wait_for_msd_connection();

		log_msd_info();

		err = raw_sector_test();
		if (err) {
			LOG_WRN("Raw sector test failed: %d", err);
		}

		err = mount_and_test();
		if (err) {
			LOG_WRN("FS mount/test failed: %d", err);
		}

		monitor_connection();

		if (s_mounted) {
			fs_unmount(&s_mp);
			s_mounted = false;
		}

		LOG_INF("Device disconnected, waiting for reconnection...");
		k_sleep(K_MSEC(200));
	}

	return 0;
}
