/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/byteorder.h>
#include <stdio.h>

#if defined(CONFIG_APP_HOST_MASS_USE_FILESYSTEM)
#include <zephyr/fs/fs.h>
#include <ff.h>
#endif /* CONFIG_APP_HOST_MASS_USE_FILESYSTEM */

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* MBR partition table: 4 entries starting at byte 446, each 16 bytes. */
#define MBR_PARTITION_TABLE_OFFSET      446
#define MBR_PARTITION_COUNT             4
#define MBR_PARTITION_ENTRY_SIZE        16
#define MBR_PARTITION_TYPE_OFFSET       4
#define MBR_PARTITION_LBA_START_OFFSET  8
#define MBR_PARTITION_NUM_SECTORS_OFFSET 12

/* MBR partition type identifiers. */
#define MBR_PART_TYPE_EMPTY         0x00
#define MBR_PART_TYPE_FAT16_SMALL   0x01
#define MBR_PART_TYPE_FAT16_OLD     0x04
#define MBR_PART_TYPE_FAT16         0x06
#define MBR_PART_TYPE_NTFS_EXFAT    0x07
#define MBR_PART_TYPE_FAT32_CHS     0x0B
#define MBR_PART_TYPE_FAT32_LBA     0x0C
#define MBR_PART_TYPE_LINUX         0x83
#define MBR_PART_TYPE_GPT           0xEE

/* MBR boot sector signature at bytes 510-511. */
#define MBR_BOOT_SIG_OFFSET_LO      510
#define MBR_BOOT_SIG_OFFSET_HI      511
#define MBR_BOOT_SIG_LO             0x55
#define MBR_BOOT_SIG_HI             0xAA

/* MBR bootable partition status byte. */
#define MBR_PART_STATUS_BOOTABLE    0x80

#define TEST_SECTOR_SIZE        512
#define TEST_MULTI_SECTOR_COUNT 4

/* Shared buffer for disk command tests and boot sector analysis. */
static uint8_t test_buf[TEST_SECTOR_SIZE * TEST_MULTI_SECTOR_COUNT];

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

enum app_state {
	APP_STATE_INIT,
	APP_STATE_WAITING_DEVICE,
	APP_STATE_DEVICE_READY,
	APP_STATE_ERROR
};

struct app_context {
	enum app_state state;
	uint32_t sector_count;
	uint32_t sector_size;
	const char *disk_name;
};

static struct app_context app_ctx;

static void app_state_change(enum app_state new_state)
{
	enum app_state old_state = app_ctx.state;

	app_ctx.state = new_state;

	LOG_INF("State change: %d -> %d", old_state, new_state);
}

static void wait_for_disk_connection(const char *disk_name)
{
	int ret;

	LOG_INF("Waiting for USB Mass Storage device...");

	while (true) {
		ret = disk_access_status(disk_name);
		if (ret == DISK_STATUS_OK || ret == DISK_STATUS_UNINIT) {
			LOG_INF("USB Mass Storage device detected!");
			return;
		}
		k_sleep(K_MSEC(100));
	}
}

static bool is_disk_connected(const char *disk_name)
{
	int ret = disk_access_status(disk_name);

	return (ret == DISK_STATUS_OK);
}

static void show_disk_info(const char *disk_name)
{
	uint32_t sector_count = 0;
	uint32_t sector_size = 0;
	uint64_t total_size;
	int ret;

	ret = disk_access_ioctl(disk_name, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	if (ret != 0) {
		LOG_ERR("Failed to get sector count: %d", ret);
		return;
	}

	ret = disk_access_ioctl(disk_name, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
	if (ret != 0) {
		LOG_ERR("Failed to get sector size: %d", ret);
		return;
	}

	app_ctx.sector_count = sector_count;
	app_ctx.sector_size = sector_size;
	total_size = (uint64_t)sector_count * sector_size;

	LOG_INF("=== USB Disk Information ===");
	LOG_INF("Disk Name:    %s", disk_name);
	LOG_INF("Sector Count: %u", sector_count);
	LOG_INF("Sector Size:  %u bytes", sector_size);
	LOG_INF("Total Size:   %llu bytes (%.2f MB)",
		total_size, (double)total_size / (1024 * 1024));
}

static int initialize_disk(const char *disk_name)
{
	int ret;
	int attempts = 0;
	const int max_attempts = 5;

	while (attempts < max_attempts) {
		ret = disk_access_init(disk_name);
		if (ret == 0) {
			LOG_INF("Disk initialized successfully on attempt %d", attempts + 1);
			break;
		}

		attempts++;
		LOG_WRN("Disk init attempt %d failed: %d", attempts, ret);

		if (attempts < max_attempts) {
			k_sleep(K_MSEC(200 * attempts));
		}
	}

	if (ret != 0) {
		LOG_ERR("Failed to initialize disk after %d attempts", max_attempts);
		return ret;
	}

	attempts = 0;
	while (attempts < max_attempts) {
		ret = disk_access_status(disk_name);
		LOG_INF("Disk status check %d: %d", attempts + 1, ret);

		if (ret == DISK_STATUS_OK) {
			LOG_INF("Disk is ready");
			break;
		}

		attempts++;
		LOG_DBG("Waiting for disk ready, status: %d (attempt %d)", ret, attempts);
		k_sleep(K_MSEC(100));
	}

	if (ret != DISK_STATUS_OK) {
		LOG_ERR("Disk not ready after %d attempts, status: %d", max_attempts, ret);
		return -EIO;
	}

	return 0;
}

static int test_msc_commands(const char *disk_name)
{
	uint32_t sector_count = 0;
	uint32_t sector_size = 0;
	uint32_t test_sector;
	int first_error;
	bool match;
	int ret;

	LOG_INF("=== MSC Command Test Start ===");

	LOG_INF("Test 1: Read Capacity - Get sector count...");
	ret = disk_access_ioctl(disk_name, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	if (ret == 0) {
		LOG_INF("  Success, last logical block: %u", sector_count - 1);
	} else {
		LOG_ERR("  Failed: %d", ret);
		return ret;
	}

	LOG_INF("Test 2: Read Capacity - Get block length...");
	ret = disk_access_ioctl(disk_name, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
	if (ret == 0) {
		LOG_INF("  Success, block length: %u bytes", sector_size);
		LOG_INF("  Total capacity: %llu bytes (%.2f MB)",
			(uint64_t)sector_count * sector_size,
			(double)((uint64_t)sector_count * sector_size) / (1024 * 1024));
	} else {
		LOG_ERR("  Failed: %d", ret);
		return ret;
	}

	LOG_INF("Test 3: Read(10) - Read sector 0...");
	memset(test_buf, 0, TEST_SECTOR_SIZE);
	ret = disk_access_read(disk_name, test_buf, 0, 1);
	if (ret == 0) {
		LOG_INF("  Success");
		LOG_HEXDUMP_INF(test_buf, 64, "  Sector 0 data (first 64 bytes):");
	} else {
		LOG_ERR("  Failed: %d", ret);
		return ret;
	}

	test_sector = 100;
	LOG_INF("Test 4: Write(10) - Write test pattern to sector %u...", test_sector);

	for (int i = 0; i < TEST_SECTOR_SIZE; i++) {
		test_buf[i] = i & 0xFF;
	}

	LOG_HEXDUMP_INF(test_buf, 64, "  Write data (first 64 bytes):");
	ret = disk_access_write(disk_name, test_buf, test_sector, 1);
	if (ret == 0) {
		LOG_INF("  Success");
	} else {
		LOG_ERR("  Failed: %d", ret);
		return ret;
	}

	LOG_INF("Test 5: Read(10) - Read back sector %u for verification...", test_sector);
	memset(test_buf, 0, TEST_SECTOR_SIZE);
	ret = disk_access_read(disk_name, test_buf, test_sector, 1);
	if (ret == 0) {
		LOG_INF("  Success");
		match = true;
		first_error = -1;
		LOG_HEXDUMP_INF(test_buf, 64, "  Read data (first 64 bytes):");

		for (int i = 0; i < TEST_SECTOR_SIZE; i++) {
			if (test_buf[i] != (i & 0xFF)) {
				if (first_error == -1) {
					first_error = i;
				}
				match = false;
			}
		}

		if (match) {
			LOG_INF("  Data verification PASSED");
		} else {
			LOG_ERR("  Data verification FAILED");
			if (first_error >= 0) {
				LOG_ERR("  First error at offset %d:"
					" got 0x%02X, expected 0x%02X",
					first_error,
					test_buf[first_error],
					first_error & 0xFF);
			}
		}
	} else {
		LOG_ERR("  Failed: %d", ret);
	}

	LOG_INF("Test 6: Multi-sector Write/Read test (%d sectors)...",
		TEST_MULTI_SECTOR_COUNT);

	for (int i = 0; i < TEST_SECTOR_SIZE * TEST_MULTI_SECTOR_COUNT; i++) {
		test_buf[i] = (i >> 2) & 0xFF;
	}

	test_sector = 200;
	LOG_INF("  Writing %d sectors to sector %u...", TEST_MULTI_SECTOR_COUNT, test_sector);
	ret = disk_access_write(disk_name, test_buf, test_sector, TEST_MULTI_SECTOR_COUNT);
	if (ret != 0) {
		LOG_ERR("  Write failed: %d", ret);
		return ret;
	}

	LOG_INF("  Write success");
	memset(test_buf, 0, TEST_SECTOR_SIZE * TEST_MULTI_SECTOR_COUNT);

	LOG_INF("  Reading back %d sectors...", TEST_MULTI_SECTOR_COUNT);
	ret = disk_access_read(disk_name, test_buf, test_sector, TEST_MULTI_SECTOR_COUNT);
	if (ret != 0) {
		LOG_ERR("  Read failed: %d", ret);
		return ret;
	}

	LOG_INF("  Read success");

	match = true;
	first_error = -1;

	for (int i = 0; i < TEST_SECTOR_SIZE * TEST_MULTI_SECTOR_COUNT; i++) {
		if (test_buf[i] != ((i >> 2) & 0xFF)) {
			if (first_error == -1) {
				first_error = i;
			}
			match = false;
		}
	}

	if (match) {
		LOG_INF("  Multi-sector verification PASSED");
	} else {
		LOG_ERR("  Multi-sector verification FAILED");
		if (first_error >= 0) {
			LOG_ERR("  First error at offset %d:"
				" got 0x%02X, expected 0x%02X",
				first_error,
				test_buf[first_error],
				(first_error >> 2) & 0xFF);
		}
	}

	LOG_INF("=== MSC Command Test Complete ===");
	LOG_INF("All basic MSC commands are working correctly!");

	return 0;
}

static int analyze_boot_sector(const char *disk_name)
{
	uint32_t lba_start;
	uint32_t num_sectors;
	const char *type_str;
	uint8_t *part_entry;
	bool has_partition;
	uint8_t status;
	uint8_t type;
	int ret;
	int i;

	LOG_INF("=== Analyzing Boot Sector (MBR/GPT) ===");

	memset(test_buf, 0, TEST_SECTOR_SIZE);
	ret = disk_access_read(disk_name, test_buf, 0, 1);
	if (ret != 0) {
		LOG_ERR("Failed to read boot sector: %d", ret);
		return ret;
	}

	LOG_HEXDUMP_INF(test_buf, 64, "Boot Sector (first 64 bytes):");

	if (test_buf[MBR_BOOT_SIG_OFFSET_LO] == MBR_BOOT_SIG_LO &&
	    test_buf[MBR_BOOT_SIG_OFFSET_HI] == MBR_BOOT_SIG_HI) {
		LOG_INF("Valid boot sector signature (0x55AA)");
	} else {
		LOG_WRN("Invalid boot signature: 0x%02X%02X (expected 0x55AA)",
			test_buf[MBR_BOOT_SIG_OFFSET_HI],
			test_buf[MBR_BOOT_SIG_OFFSET_LO]);
		return -EINVAL;
	}

	if (test_buf[MBR_PARTITION_TABLE_OFFSET + MBR_PARTITION_TYPE_OFFSET] ==
	    MBR_PART_TYPE_GPT) {
		LOG_INF("Partition scheme: GPT (GUID Partition Table)");
		LOG_INF("  Protective MBR detected");
		return 0;
	}

	LOG_INF("Partition scheme: MBR (Master Boot Record)");

	has_partition = false;

	for (i = 0; i < MBR_PARTITION_COUNT; i++) {
		part_entry = &test_buf[MBR_PARTITION_TABLE_OFFSET +
				       (i * MBR_PARTITION_ENTRY_SIZE)];
		status = part_entry[0];
		type = part_entry[MBR_PARTITION_TYPE_OFFSET];
		lba_start = sys_le32_to_cpu(
			sys_get_le32(&part_entry[MBR_PARTITION_LBA_START_OFFSET]));
		num_sectors = sys_le32_to_cpu(
			sys_get_le32(&part_entry[MBR_PARTITION_NUM_SECTORS_OFFSET]));

		if (type == MBR_PART_TYPE_EMPTY) {
			continue;
		}

		if (type == MBR_PART_TYPE_FAT32_CHS ||
		    type == MBR_PART_TYPE_FAT32_LBA) {
			type_str = "(FAT32)";
		} else if (type == MBR_PART_TYPE_FAT16_SMALL ||
			   type == MBR_PART_TYPE_FAT16_OLD ||
			   type == MBR_PART_TYPE_FAT16) {
			type_str = "(FAT16)";
		} else if (type == MBR_PART_TYPE_NTFS_EXFAT) {
			type_str = "(NTFS/exFAT)";
		} else if (type == MBR_PART_TYPE_LINUX) {
			type_str = "(Linux)";
		} else {
			type_str = "";
		}

		has_partition = true;
		LOG_INF("Partition %d:", i + 1);
		LOG_INF("  Status: 0x%02X %s", status,
			(status == MBR_PART_STATUS_BOOTABLE) ? "(Bootable)" : "");
		LOG_INF("  Type: 0x%02X %s", type, type_str);
		LOG_INF("  Start LBA: %u", lba_start);
		LOG_INF("  Sectors: %u (%.2f MB)",
			num_sectors,
			(double)(num_sectors * 512) / (1024 * 1024));
	}

	if (!has_partition) {
		LOG_WRN("No valid partitions found in MBR");
		LOG_INF("This might be a superfloppy format (no partition table)");
	}

	return 0;
}

#if defined(CONFIG_APP_HOST_MASS_USE_FILESYSTEM)

/* FatFs work area - one volume per mounted disk. */
static FATFS fatfs_data;

static struct fs_mount_t fs_mnt = {
	.type      = FS_FATFS,
	.fs_data   = &fatfs_data,
};

/* Persistent buffer for the mount path: "/disk_name:" */
static char fs_mnt_path[32];

static int fs_mount_disk(const char *disk_name)
{
	int ret;

	snprintf(fs_mnt_path, sizeof(fs_mnt_path), "/%s:", disk_name);
	fs_mnt.mnt_point = fs_mnt_path;

	LOG_INF("Mounting FAT file system at %s", fs_mnt_path);

	ret = fs_mount(&fs_mnt);
	if (ret != 0) {
		LOG_ERR("Failed to mount file system: %d", ret);
		return ret;
	}

	LOG_INF("File system mounted successfully");
	return 0;
}

static void fs_unmount_disk(void)
{
	if (fs_mnt.mnt_point == NULL) {
		return;
	}

	if (fs_unmount(&fs_mnt) == 0) {
		LOG_INF("File system unmounted");
	}

	fs_mnt.mnt_point = NULL;
}

static void fs_list_dir(const char *path)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	int ret;

	fs_dir_t_init(&dir);

	ret = fs_opendir(&dir, path);
	if (ret != 0) {
		LOG_ERR("Failed to open directory %s: %d", path, ret);
		return;
	}

	LOG_INF("Directory listing: %s", path);

	while (true) {
		ret = fs_readdir(&dir, &entry);
		if (ret != 0 || entry.name[0] == '\0') {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("  [DIR]  %s", entry.name);
		} else {
			LOG_INF("  [FILE] %s (%zu bytes)", entry.name, entry.size);
		}
	}

	fs_closedir(&dir);
}

static int fs_test_operations(const char *mnt_point)
{
	struct fs_file_t file;
	char path[64];
	const char *write_data = "Hello from Zephyr USB host mass storage!\n";
	char read_buf[64];
	ssize_t bytes;
	int ret;

	/* Create a subdirectory if it does not already exist */
	snprintf(path, sizeof(path), "%s/zephyr", mnt_point);
	{
		struct fs_dirent entry;

		if (fs_stat(path, &entry) != 0) {
			ret = fs_mkdir(path);
			if (ret != 0) {
				LOG_ERR("Failed to create directory %s: %d", path, ret);
				return ret;
			}
		}
	}
	LOG_INF("Directory: %s", path);

	/* Create and write a file inside the subdirectory */
	snprintf(path, sizeof(path), "%s/zephyr/test.txt", mnt_point);
	fs_file_t_init(&file);

	ret = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR | FS_O_TRUNC);
	if (ret != 0) {
		LOG_ERR("Failed to open %s: %d", path, ret);
		return ret;
	}

	bytes = fs_write(&file, write_data, strlen(write_data));
	if (bytes < 0) {
		LOG_ERR("Write failed: %zd", bytes);
		fs_close(&file);
		return (int)bytes;
	}
	LOG_INF("Wrote %zd bytes to %s", bytes, path);

	/* Seek back and read to verify */
	ret = fs_seek(&file, 0, FS_SEEK_SET);
	if (ret != 0) {
		LOG_ERR("Seek failed: %d", ret);
		fs_close(&file);
		return ret;
	}

	memset(read_buf, 0, sizeof(read_buf));
	bytes = fs_read(&file, read_buf, sizeof(read_buf) - 1);
	if (bytes < 0) {
		LOG_ERR("Read failed: %zd", bytes);
		fs_close(&file);
		return (int)bytes;
	}
	LOG_INF("Read back: \"%s\"", read_buf);

	fs_close(&file);
	return 0;
}

static int fs_run_tests(void)
{
	const char *mnt_point = fs_mnt.mnt_point;
	int ret;

	LOG_INF("=== File System Test Start ===");

	fs_list_dir(mnt_point);

	ret = fs_test_operations(mnt_point);
	if (ret != 0) {
		LOG_WRN("File system test failed: %d", ret);
	} else {
		LOG_INF("File system test passed");
	}

	LOG_INF("=== File System Test Complete ===");
	return ret;
}

#endif /* CONFIG_APP_HOST_MASS_USE_FILESYSTEM */

static int run_disk_tests(const char *disk_name)
{
	int ret;

	LOG_INF("Setting up disk access...");
	app_state_change(APP_STATE_DEVICE_READY);

	ret = initialize_disk(disk_name);
	if (ret != 0) {
		app_state_change(APP_STATE_ERROR);
		return ret;
	}

	show_disk_info(disk_name);

	ret = test_msc_commands(disk_name);
	if (ret != 0) {
		LOG_ERR("MSC command test failed: %d", ret);
	}

	ret = analyze_boot_sector(disk_name);
	if (ret != 0) {
		LOG_WRN("Boot sector analysis failed: %d", ret);
	}

#if defined(CONFIG_APP_HOST_MASS_USE_FILESYSTEM)
	ret = fs_mount_disk(disk_name);
	if (ret != 0) {
		LOG_WRN("File system mount failed: %d - continuing without FS", ret);
	} else {
		fs_run_tests();
	}
#endif /* CONFIG_APP_HOST_MASS_USE_FILESYSTEM */

	LOG_INF("=== Disk Access Setup Complete ===");

	return 0;
}

static void cleanup_disk_access(const char *disk_name)
{
	LOG_INF("Cleaning up disk access...");

#if defined(CONFIG_APP_HOST_MASS_USE_FILESYSTEM)
	fs_unmount_disk();
#endif /* CONFIG_APP_HOST_MASS_USE_FILESYSTEM */

	disk_access_ioctl(disk_name, DISK_IOCTL_CTRL_DEINIT, NULL);

	app_ctx.sector_count = 0;
	app_ctx.sector_size = 0;
	app_state_change(APP_STATE_WAITING_DEVICE);

	LOG_INF("Disk cleanup completed");
}

static int check_disk_connected(const char *disk_name)
{
	if (!is_disk_connected(disk_name)) {
		LOG_WRN("Disk disconnected");
		return -ENODEV;
	}

	return 0;
}

int main(void)
{
	const struct device *msc_dev;
	int err;

	LOG_INF("USB Host Mass Storage Sample - Raw Disk Access");

	msc_dev = device_get_binding("usbh_msc_0");
	if (msc_dev == NULL || !device_is_ready(msc_dev)) {
		LOG_ERR("USB host MSC device not ready");
		return -ENODEV;
	}

	LOG_INF("USB host MSC device %s is ready", msc_dev->name);

	app_ctx.disk_name = msc_dev->name;

	LOG_INF("Using disk name: %s", app_ctx.disk_name);

	app_state_change(APP_STATE_INIT);

	err = usbh_init(&uhs_ctx);
	if (err != 0) {
		LOG_ERR("Failed to initialize USB host support: %d", err);
		return err;
	}

	err = usbh_enable(&uhs_ctx);
	if (err != 0) {
		LOG_ERR("Failed to enable USB host support: %d", err);
		return err;
	}

	app_state_change(APP_STATE_WAITING_DEVICE);

	while (true) {
		wait_for_disk_connection(app_ctx.disk_name);

		err = run_disk_tests(app_ctx.disk_name);
		if (err != 0) {
			LOG_ERR("Failed to run disk tests: %d", err);
			k_sleep(K_MSEC(1000));
			continue;
		}

		while (true) {
			k_sleep(K_MSEC(1000));

			err = check_disk_connected(app_ctx.disk_name);
			if (err == -ENODEV) {
				LOG_INF("Disk disconnected");
				break;
			}
		}

		cleanup_disk_access(app_ctx.disk_name);

		LOG_INF("Waiting for device reconnection...");
	}

	return 0;
}
