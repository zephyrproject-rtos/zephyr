/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * WARNING: This test will overwrite data on any disk utilized. Do not run
 * this test with an disk that has useful data
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/device.h>

#ifdef CONFIG_DISK_DRIVER_LOOPBACK
#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/drivers/loopback_disk.h>
#endif

#if defined(CONFIG_DISK_DRIVER_SDMMC)
#define DISK_NAME_PHYS "SD"
#elif defined(CONFIG_DISK_DRIVER_MMC)
#define DISK_NAME_PHYS "SD2"
#elif defined(CONFIG_DISK_DRIVER_FLASH)
#define DISK_NAME_PHYS "NAND"
#elif defined(CONFIG_NVME)
#define DISK_NAME_PHYS "nvme0n0"
#elif defined(CONFIG_DISK_DRIVER_RAM)
/* Since ramdisk is enabled by default on e.g. qemu boards, it needs to be checked last to not
 * override other backends.
 */
#define DISK_NAME_PHYS "RAM"
#else
#error "No disk device defined, is your board supported?"
#endif

#ifdef CONFIG_DISK_DRIVER_LOOPBACK
#define DISK_NAME "loopback0"
#else
#define DISK_NAME DISK_NAME_PHYS
#endif

/* Assume the largest sector we will encounter is 512 bytes */
#define SECTOR_SIZE 512

/* Sector counts to read */
#define SECTOR_COUNT1 8
#define SECTOR_COUNT2 1
#define SECTOR_COUNT3 29
#define SECTOR_COUNT4 31

#define OVERFLOW_CANARY 0xDE

static const char *disk_pdrv = DISK_NAME;
static uint32_t disk_sector_count;
static uint32_t disk_sector_size;

/* + 4 to make sure the second buffer is dword-aligned for NVME */
static uint8_t scratch_buf[2][SECTOR_COUNT4 * SECTOR_SIZE + 4];

#ifdef CONFIG_DISK_DRIVER_LOOPBACK
#define BACKING_PATH "/"DISK_NAME_PHYS":"

static struct loopback_disk_access lo_access;
static FATFS fat_fs;
static struct fs_mount_t backing_mount = {
	.type = FS_FATFS,
	.mnt_point = BACKING_PATH,
	.fs_data = &fat_fs,
};
static const uint8_t zero_kb[1024] = {};
static void setup_loopback_backing(void)
{
	int rc;

	rc = fs_mkfs(FS_FATFS, (uintptr_t)&BACKING_PATH[1], NULL, 0);
	zassert_equal(rc, 0, "Failed to format backing file system");

	rc = fs_mount(&backing_mount);
	zassert_equal(rc, 0, "Failed to mount backing file system");

	struct fs_file_t f;

	fs_file_t_init(&f);
	rc = fs_open(&f, BACKING_PATH "/loopback.img", FS_O_WRITE | FS_O_CREATE);
	zassert_equal(rc, 0, "Failed to create backing file");
	for (int i = 0; i < 64; i++) {
		rc = fs_write(&f, zero_kb, sizeof(zero_kb));
		zassert_equal(rc, sizeof(zero_kb), "Failed to enlarge backing file");
	}
	rc = fs_close(&f);
	zassert_equal(rc, 0, "Failed to close backing file");

	rc = loopback_disk_access_register(&lo_access, BACKING_PATH "/loopback.img", DISK_NAME);
	zassert_equal(rc, 0, "Loopback disk access initialization failed");
}
#endif

/* Sets up test by initializing disk */
static void test_setup(void)
{
	int rc;
	uint32_t cmd_buf;

	rc = disk_access_init(disk_pdrv);
	zassert_equal(rc, 0, "Disk access initialization failed");

	rc = disk_access_status(disk_pdrv);
	zassert_equal(rc, DISK_STATUS_OK, "Disk status is not OK");

	rc = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &cmd_buf);
	zassert_equal(rc, 0, "Disk ioctl get sector count failed");

	TC_PRINT("Disk reports %u sectors\n", cmd_buf);
	disk_sector_count = cmd_buf;

	rc = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &cmd_buf);
	zassert_equal(rc, 0, "Disk ioctl get sector size failed");
	TC_PRINT("Disk reports sector size %u\n", cmd_buf);
	disk_sector_size = cmd_buf;

	/* We could allocate memory once we know the sector size, but instead
	 * just verify our assumed maximum size
	 */
	zassert_true(cmd_buf <= SECTOR_SIZE,
		"Test will fail, SECTOR_SIZE definition must be increased");
}

/* Reads sectors, verifying overflow does not occur */
static int read_sector(uint8_t *buf, uint32_t start, uint32_t num_sectors)
{
	int rc;

	/* Set up overflow canary */
	buf[num_sectors * disk_sector_size] = OVERFLOW_CANARY;
	rc = disk_access_read(disk_pdrv, buf, start, num_sectors);
	/* Check canary */
	zassert_equal(buf[num_sectors * disk_sector_size], OVERFLOW_CANARY,
		"Read overflowed requested length");
	return rc; /* Let calling function check return code */
}

/* Tests reading from a variety of sectors */
static void test_sector_read(uint8_t *buf, uint32_t num_sectors)
{
	int rc, sector;

	TC_PRINT("Testing reads of %u sectors\n", num_sectors);
	/* Read from disk sector 0*/
	rc = read_sector(buf, 0, num_sectors);
	zassert_equal(rc, 0, "Failed to read from sector zero");
	/* Read from a sector in the "middle" of the disk */
	if (disk_sector_count / 2 > num_sectors) {
		sector = disk_sector_count / 2 - num_sectors;
	} else {
		sector = 0;
	}

	rc = read_sector(buf, sector, num_sectors);
	zassert_equal(rc, 0, "Failed to read from mid disk sector");
	/* Read from the last sector */
	rc = read_sector(buf, disk_sector_count - 1, num_sectors);
	if (num_sectors == 1) {
		zassert_equal(rc, 0, "Failed to read from last sector");
	} else {
		zassert_not_equal(rc, 0, "Disk should fail to read out of sector bounds");
	}
}

/* Write sector of disk, and check the data to ensure it is valid
 * WARNING: this test is destructive- it will overwrite data on the disk!
 */
static int write_sector_checked(uint8_t *wbuf, uint8_t *rbuf,
			uint32_t start, uint32_t num_sectors)
{
	int rc, i;

	/* First, fill the write buffer with data */
	for (i = 0; i < num_sectors * disk_sector_size; i++) {
		wbuf[i] = (i & (~num_sectors));
	}
	/* Now write data to the sector */
	rc = disk_access_write(disk_pdrv, wbuf, start, num_sectors);
	if (rc) {
		return rc; /* Let calling function handle disk error */
	}
	/* Read back the written data into another buffer */
	memset(rbuf, 0, num_sectors * disk_sector_size);
	rc = read_sector(rbuf, start, num_sectors);
	if (rc) {
		return rc;
	}
	/* Check the read data versus the written data */
	zassert_mem_equal(wbuf, rbuf, num_sectors * disk_sector_size,
		"Read data did not match data written to disk");
	return rc;
}

/* Tests writing to a variety of sectors
 * WARNING: this test is destructive- it will overwrite data on the disk!
 */
static void test_sector_write(uint8_t *wbuf, uint8_t *rbuf, uint32_t num_sectors)
{
	int rc, sector;

	TC_PRINT("Testing writes of %u sectors\n", num_sectors);
	/* Write to disk sector zero */
	rc = write_sector_checked(wbuf, rbuf, 0, num_sectors);
	zassert_equal(rc, 0, "Failed to write to sector zero");
	/* Write to a sector in the "middle" of the disk */
	if (disk_sector_count / 2 > num_sectors) {
		sector = disk_sector_count / 2 - num_sectors;
	} else {
		sector = 0;
	}

	rc = write_sector_checked(wbuf, rbuf, sector, num_sectors);
	zassert_equal(rc, 0, "Failed to write to mid disk sector");
	/* Write to the last sector */
	rc = write_sector_checked(wbuf, rbuf, disk_sector_count - 1, num_sectors);
	if (num_sectors == 1) {
		zassert_equal(rc, 0, "Failed to write to last sector");
	} else {
		zassert_not_equal(rc, 0, "Disk should fail to write out of sector bounds");
	}
}

/* Test multiple reads in series, and reading from a variety of blocks */
ZTEST(disk_driver, test_read)
{
	int rc, i;

	/* Verify all 4 read sizes work */
	test_sector_read(scratch_buf[0], SECTOR_COUNT1);
	test_sector_read(scratch_buf[0], SECTOR_COUNT2);
	test_sector_read(scratch_buf[0], SECTOR_COUNT3);
	test_sector_read(scratch_buf[0], SECTOR_COUNT4);

	/* Verify that reading from the same location returns to same data */
	memset(scratch_buf[0], 0, SECTOR_COUNT1 * disk_sector_size);
	rc = read_sector(scratch_buf[0], 0, SECTOR_COUNT1);
	zassert_equal(rc, 0, "Failed to read from disk");
	for (i = 0; i < 10; i++) {
		/* Read from sector, and compare it to the first read */
		memset(scratch_buf[1], 0xff, SECTOR_COUNT1 * disk_sector_size);
		rc = read_sector(scratch_buf[1], 0, SECTOR_COUNT1);
		zassert_equal(rc, 0, "Failed to read from disk at same sector location");
		zassert_mem_equal(scratch_buf[1], scratch_buf[0],
				SECTOR_COUNT1 * disk_sector_size,
				"Multiple reads mismatch");
	}
}

/* test writing data, and then verifying it was written correctly.
 * WARNING: this test is destructive- it will overwrite data on the disk!
 */
ZTEST(disk_driver, test_write)
{
	int rc, i;

	/* Verify all 4 sector write sizes work */
	test_sector_write(scratch_buf[0], scratch_buf[1], SECTOR_COUNT1);
	test_sector_write(scratch_buf[0], scratch_buf[1], SECTOR_COUNT2);
	test_sector_write(scratch_buf[0], scratch_buf[1], SECTOR_COUNT3);
	test_sector_write(scratch_buf[0], scratch_buf[1], SECTOR_COUNT4);

	/* Verify that multiple writes to the same location work */
	for (i = 0; i < 10; i++) {
		/* Write to sector- helper function verifies written data is correct */
		rc = write_sector_checked(scratch_buf[0], scratch_buf[1], 0, SECTOR_COUNT1);
		zassert_equal(rc, 0, "Failed to write to disk at same sector location");
	}
}

static void *disk_driver_setup(void)
{
#ifdef CONFIG_DISK_DRIVER_LOOPBACK
	setup_loopback_backing();
#endif
	test_setup();

	return NULL;
}

ZTEST_SUITE(disk_driver, NULL, disk_driver_setup, NULL, NULL, NULL);
