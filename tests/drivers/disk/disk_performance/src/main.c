/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/device.h>
#include <zephyr/timing/timing.h>
#include <zephyr/random/random.h>

#if defined(CONFIG_DISK_DRIVER_SDMMC)
#define DISK_NAME CONFIG_SDMMC_VOLUME_NAME
#elif defined(CONFIG_DISK_DRIVER_MMC)
#define DISK_NAME CONFIG_MMC_VOLUME_NAME
#elif defined(CONFIG_NVME)
#define DISK_NAME "nvme0n0"
#else
#error "No disk device defined, is your board supported?"
#endif

/* Assume the largest sector we will encounter is 512 bytes */
#define SECTOR_SIZE 512
#if CONFIG_SRAM_SIZE >= 512
/* Cap buffer size at 128 KiB */
#define SEQ_BLOCK_COUNT 256
#elif CONFIG_SOC_POSIX
/* Posix does not define SRAM size */
#define SEQ_BLOCK_COUNT 256
#else
/* Two buffers with 512 byte blocks will use half of all SRAM */
#define SEQ_BLOCK_COUNT (CONFIG_SRAM_SIZE / 2)
#endif
#define BUF_SIZE (SECTOR_SIZE * SEQ_BLOCK_COUNT)
/* Number of sequential reads to get an average speed */
#define SEQ_ITERATIONS 10
/* Number of random reads to get an IOPS calculation */
#define RANDOM_ITERATIONS SEQ_BLOCK_COUNT

static uint32_t chosen_sectors[RANDOM_ITERATIONS];


static const char *disk_pdrv = DISK_NAME;
static uint32_t disk_sector_count;
static uint32_t disk_sector_size;

static uint8_t test_buf[BUF_SIZE] __aligned(32);
static uint8_t backup_buf[BUF_SIZE] __aligned(32);

static bool disk_init_done;

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

	/* Assume sector size is 512 bytes, it will speed up calculations later */
	zassert_true(cmd_buf == SECTOR_SIZE,
		"Test will fail, SECTOR_SIZE definition must be changed");

	disk_init_done = true;
}

/* Helper function to time multiple sequential reads. Returns average time. */
static uint64_t read_helper(uint32_t num_blocks)
{
	int rc;
	timing_t start_time, end_time;
	uint64_t cycles, total_ns;

	/* Start the timing system */
	timing_init();
	timing_start();

	total_ns = 0;
	for (int i = 0; i < SEQ_ITERATIONS; i++) {
		start_time = timing_counter_get();

		/* Read from start of disk */
		rc = disk_access_read(disk_pdrv, test_buf, 0, num_blocks);

		end_time = timing_counter_get();

		zassert_equal(rc, 0, "disk read failed");

		cycles = timing_cycles_get(&start_time, &end_time);
		total_ns += timing_cycles_to_ns(cycles);
	}
	/* Stop timing system */
	timing_stop();
	/* Return average time */
	return (total_ns / SEQ_ITERATIONS);
}

ZTEST(disk_performance, test_sequential_read)
{
	uint64_t time_ns;

	if (!disk_init_done) {
		zassert_unreachable("Disk is not initialized");
	}

	/* Start with single sector read */
	time_ns = read_helper(1);

	TC_PRINT("Average read speed over one sector: %"PRIu64" KiB/s\n",
		((SECTOR_SIZE * (NSEC_PER_SEC / time_ns))) / 1024);

	/* Now time long sequential read */
	time_ns = read_helper(SEQ_BLOCK_COUNT);

	TC_PRINT("Average read speed over %d sectors: %"PRIu64" KiB/s\n",
		SEQ_BLOCK_COUNT,
		((BUF_SIZE) * (NSEC_PER_SEC / time_ns)) / 1024);
}

/* Helper function to time multiple sequential writes. Returns average time. */
static uint64_t write_helper(uint32_t num_blocks)
{
	int rc;
	timing_t start_time, end_time;
	uint64_t cycles, total_ns;

	/* Start the timing system */
	timing_init();
	timing_start();

	/* Read block we will overwrite, to back it up. */
	rc = disk_access_read(disk_pdrv, backup_buf, 0, num_blocks);
	zassert_equal(rc, 0, "disk read failed");

	/* Initialize write buffer with data */
	sys_rand_get(test_buf, num_blocks * SECTOR_SIZE);

	total_ns = 0;
	for (int i = 0; i < SEQ_ITERATIONS; i++) {
		start_time = timing_counter_get();

		rc = disk_access_write(disk_pdrv, test_buf, 0, num_blocks);

		end_time = timing_counter_get();

		zassert_equal(rc, 0, "disk write failed");

		cycles = timing_cycles_get(&start_time, &end_time);
		total_ns += timing_cycles_to_ns(cycles);
	}
	/* Stop timing system */
	timing_stop();

	/* Replace block with backup */
	rc = disk_access_write(disk_pdrv, backup_buf, 0, num_blocks);
	zassert_equal(rc, 0, "disk write failed");
	/* Return average time */
	return (total_ns / SEQ_ITERATIONS);
}

ZTEST(disk_performance, test_sequential_write)
{
	uint64_t time_ns;

	if (!disk_init_done) {
		zassert_unreachable("Disk is not initialized");
	}

	/* Start with single sector write */
	time_ns = write_helper(1);

	TC_PRINT("Average write speed over one sector: %"PRIu64" KiB/s\n",
		((SECTOR_SIZE * (NSEC_PER_SEC / time_ns))) / 1024);

	/* Now time long sequential write */
	time_ns = write_helper(SEQ_BLOCK_COUNT);

	TC_PRINT("Average write speed over %d sectors: %"PRIu64" KiB/s\n",
		SEQ_BLOCK_COUNT,
		((BUF_SIZE) * (NSEC_PER_SEC / time_ns)) / 1024);
}

ZTEST(disk_performance, test_random_read)
{
	timing_t start_time, end_time;
	uint64_t cycles, total_ns;
	uint32_t sector;
	int rc;

	if (!disk_init_done) {
		zassert_unreachable("Disk is not initialized");
	}

	/* Build list of sectors to read from. */
	for (int i = 0; i < RANDOM_ITERATIONS; i++) {
		/* Get random num until we select a value within sector count */
		sector = sys_rand32_get() / ((UINT32_MAX / disk_sector_count) + 1);
		chosen_sectors[i] = sector;
	}

	/* Start the timing system */
	timing_init();
	timing_start();

	start_time = timing_counter_get();
	for (int i = 0; i < RANDOM_ITERATIONS; i++) {
		/*
		 * Note: we don't check return code here,
		 * we want to do I/O as fast as possible
		 */
		rc = disk_access_read(disk_pdrv, test_buf, chosen_sectors[i], 1);
	}
	end_time = timing_counter_get();
	zassert_equal(rc, 0, "Random read failed");
	cycles = timing_cycles_get(&start_time, &end_time);
	total_ns = timing_cycles_to_ns(cycles);
	/* Stop timing system */
	timing_stop();

	TC_PRINT("512 Byte IOPS over %d random reads: %"PRIu64" IOPS\n",
		RANDOM_ITERATIONS,
		((uint64_t)(((uint64_t)RANDOM_ITERATIONS)*
		((uint64_t)NSEC_PER_SEC)))
		/ total_ns);
}

ZTEST(disk_performance, test_random_write)
{
	timing_t start_time, end_time;
	uint64_t cycles, total_ns;
	uint32_t sector;
	int rc;

	if (!disk_init_done) {
		zassert_unreachable("Disk is not initialized");
	}

	/* Build list of sectors to read from. */
	for (int i = 0; i < RANDOM_ITERATIONS; i++) {
		/* Get random num until we select a value within sector count */
		sector = sys_rand32_get() / ((UINT32_MAX / disk_sector_count) + 1);
		chosen_sectors[i] = sector;
		/* Backup this sector */
		rc = disk_access_read(disk_pdrv, &backup_buf[i * SECTOR_SIZE],
			sector, 1);
		zassert_equal(rc, 0, "disk read failed for random write backup");
	}

	/* Initialize write buffer with data */
	sys_rand_get(test_buf, BUF_SIZE);

	/* Start the timing system */
	timing_init();
	timing_start();

	start_time = timing_counter_get();
	for (int i = 0; i < RANDOM_ITERATIONS; i++) {
		/*
		 * Note: we don't check return code here,
		 * we want to do I/O as fast as possible
		 */
		rc = disk_access_write(disk_pdrv, &test_buf[i * SECTOR_SIZE],
			chosen_sectors[i], 1);
	}
	end_time = timing_counter_get();
	zassert_equal(rc, 0, "Random write failed");
	cycles = timing_cycles_get(&start_time, &end_time);
	total_ns = timing_cycles_to_ns(cycles);
	/* Stop timing system */
	timing_stop();

	TC_PRINT("512 Byte IOPS over %d random writes: %"PRIu64" IOPS\n",
		RANDOM_ITERATIONS,
		((uint64_t)(((uint64_t)RANDOM_ITERATIONS)*
		((uint64_t)NSEC_PER_SEC)))
		/ total_ns);
	/* Restore backed up sectors */
	for (int i = 0; i < RANDOM_ITERATIONS; i++) {
		disk_access_write(disk_pdrv, &backup_buf[i * SECTOR_SIZE],
			chosen_sectors[i], 1);
		zassert_equal(rc, 0, "failed to write backup sector to disk");
	}
}

static void *disk_setup(void)
{
	test_setup();

	return NULL;
}

ZTEST_SUITE(disk_performance, NULL, disk_setup, NULL, NULL, NULL);
