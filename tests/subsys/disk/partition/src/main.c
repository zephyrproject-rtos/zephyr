/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/storage/disk_partition.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define SECTOR_SIZE  512U
#define DISK_SECTORS 12288U

struct fake_disk {
	uint8_t image[DISK_SECTORS * SECTOR_SIZE];
};

static struct fake_disk g_disk;

static int fake_read(void *ctx, uint64_t lba, void *buf, uint32_t bytes)
{
	struct fake_disk *disk = ctx;

	if (bytes != SECTOR_SIZE || lba >= DISK_SECTORS) {
		return -EINVAL;
	}

	memcpy(buf, &disk->image[lba * SECTOR_SIZE], bytes);

	return 0;
}

static void fake_mbr_primary(struct fake_disk *disk, uint8_t type, uint32_t start, uint32_t sectors)
{
	uint8_t *mbr = &disk->image[0];

	mbr[510] = 0x55U;
	mbr[511] = 0xAAU;
	mbr[0x1BE + 4] = type;
	sys_put_le32(start, &mbr[0x1BE + 8]);
	sys_put_le32(sectors, &mbr[0x1BE + 12]);
}

static void fake_fat32_bpb(uint8_t *sector)
{
	sector[0] = 0xEB;
	sector[2] = 0x90;
	memcpy(&sector[82], "FAT32   ", 8);
	sector[510] = 0x55U;
	sector[511] = 0xAAU;
}

static void fake_ext2_superblock(uint8_t *sector)
{
	sys_put_le16(0xEF53, &sector[56]);
}

ZTEST(disk_partition, test_mbr_primary_walk)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	memset(&g_disk, 0, sizeof(g_disk));
	fake_mbr_primary(&g_disk, 0x83, 2048U, 4096U);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_MBR, table.scheme);
	zassert_equal(1, table.entry_count);
	zassert_equal(2048U, table.entries[0].start_lba);
	zassert_equal(4096U, table.entries[0].nr_sectors);
	zassert_equal(0x83, table.entries[0].mbr_type);
}

ZTEST(disk_partition, test_find_fat_on_mbr_slice)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	uint64_t lba_off;
	uint64_t sec_count;
	int ret;

	memset(&g_disk, 0, sizeof(g_disk));
	fake_mbr_primary(&g_disk, 0x0C, 2048U, 8192U);
	fake_fat32_bpb(&g_disk.image[2048U * SECTOR_SIZE]);

	ret = disk_partition_find_fat_volume(&geo, fake_read, &g_disk, &lba_off, &sec_count);
	zassert_ok(ret);
	zassert_equal(2048U, lba_off);
	zassert_equal(8192U, sec_count);
}

ZTEST(disk_partition, test_whole_disk_and_ext2)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	uint64_t lba_off;
	uint64_t sec_count;
	int ret;

	memset(&g_disk, 0, sizeof(g_disk));
	fake_ext2_superblock(&g_disk.image[2U * SECTOR_SIZE]);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_NONE, table.scheme);
	zassert_equal(0, table.entry_count);

	ret = disk_partition_set_whole_disk(&geo, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_WHOLE_DISK, table.scheme);

	ret = disk_partition_find_ext2_volume(&geo, fake_read, &g_disk, &lba_off, &sec_count);
	zassert_ok(ret);
	zassert_equal(0U, lba_off);
}

ZTEST(disk_partition, test_superfloppy_fat)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	uint64_t lba_off;
	uint64_t sec_count;
	int ret;

	memset(&g_disk, 0, sizeof(g_disk));
	fake_fat32_bpb(&g_disk.image[0]);
	ret = disk_partition_find_fat_volume(&geo, fake_read, &g_disk, &lba_off, &sec_count);
	zassert_ok(ret);
	zassert_equal(0U, lba_off);
}

ZTEST_SUITE(disk_partition, NULL, NULL, NULL, NULL, NULL);
