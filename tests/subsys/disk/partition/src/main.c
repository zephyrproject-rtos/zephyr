/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/storage/disk_partition.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define SECTOR_SIZE  512U
#define DISK_SECTORS 12288U
#define EXT_BASE     2048U

/*
 * Microsoft Basic Data partition (EFI_BASIC_DATA):
 * EBD0A0A2-B9E5-4433-87C0-68B6B72699C7 — on-disk GUID field byte order.
 */
static const uint8_t gpt_type_basic_data[16] = {
	0xa2U, 0xa0U, 0xd0U, 0xebU, 0xe5U, 0xb9U, 0x33U, 0x44U,
	0x87U, 0xc0U, 0x68U, 0xb6U, 0xb7U, 0x26U, 0x99U, 0xc7U,
};

struct fake_disk {
	uint32_t block_size;
	uint8_t image[DISK_SECTORS * SECTOR_SIZE];
	/* fail_lba < 0 disables; otherwise fake_read returns fail_err at that LBA */
	int64_t fail_lba;
	int fail_err;
};

static struct fake_disk g_disk;

static int fake_read(void *ctx, uint64_t lba, void *buf, uint32_t bytes)
{
	struct fake_disk *disk = ctx;

	if (disk->fail_lba >= 0 && lba == (uint64_t)disk->fail_lba) {
		return disk->fail_err;
	}

	if (bytes != disk->block_size || lba >= DISK_SECTORS) {
		return -EINVAL;
	}

	memcpy(buf, &disk->image[lba * SECTOR_SIZE], bytes);

	return 0;
}

static void fake_mbr_slot(struct fake_disk *disk, unsigned int slot, uint8_t type, uint32_t start,
			  uint32_t sectors)
{
	uint8_t *mbr = &disk->image[0];
	const unsigned int off = 0x1BE + slot * 16U;

	mbr[510] = 0x55U;
	mbr[511] = 0xAAU;
	mbr[off + 4] = type;
	sys_put_le32(start, &mbr[off + 8]);
	sys_put_le32(sectors, &mbr[off + 12]);
}

static void fake_mbr_primary(struct fake_disk *disk, uint8_t type, uint32_t start, uint32_t sectors)
{
	fake_mbr_slot(disk, 0U, type, start, sectors);
}

static void fake_ebr_logical(struct fake_disk *disk, uint64_t ebr_lba, uint8_t type,
			     uint32_t rel_start, uint32_t sectors, uint32_t link_rel,
			     uint8_t link_type)
{
	uint8_t *ebr = &disk->image[ebr_lba * SECTOR_SIZE];

	ebr[510] = 0x55U;
	ebr[511] = 0xAAU;
	ebr[0x1BE + 4] = type;
	sys_put_le32(rel_start, &ebr[0x1BE + 8]);
	sys_put_le32(sectors, &ebr[0x1BE + 12]);
	ebr[0x1BE + 16U + 4] = link_type;
	sys_put_le32(link_rel, &ebr[0x1BE + 16U + 8]);
}

static void fake_gpt_protective_mbr(struct fake_disk *disk)
{
	uint8_t *mbr = &disk->image[0];

	mbr[510] = 0x55U;
	mbr[511] = 0xAAU;
	mbr[0x1BE + 4] = 0xEEU;
	sys_put_le32(1U, &mbr[0x1BE + 8]);
	sys_put_le32(DISK_SECTORS - 1U, &mbr[0x1BE + 12]);
}

static void fake_gpt_header(struct fake_disk *disk, uint32_t part_lba, uint32_t part_count,
			    uint32_t entry_size)
{
	uint8_t *hdr = &disk->image[1U * SECTOR_SIZE];

	memcpy(hdr, "EFI PART", 8);
	sys_put_le32(92U, &hdr[12]);
	sys_put_le64(part_lba, &hdr[72]);
	sys_put_le32(part_count, &hdr[80]);
	sys_put_le32(entry_size, &hdr[84]);
}

static void fake_gpt_write_entry(struct fake_disk *disk, uint32_t part_lba, uint32_t entry_size,
				 uint32_t idx, uint64_t first_lba, uint64_t last_lba)
{
	const uint64_t byte_off = (uint64_t)idx * entry_size;
	const uint64_t sec = part_lba + byte_off / SECTOR_SIZE;
	const uint32_t off = (uint32_t)(byte_off % SECTOR_SIZE);
	uint8_t *base = &disk->image[sec * SECTOR_SIZE + off];
	uint8_t scratch[512];

	(void)memset(scratch, 0, entry_size);
	memcpy(scratch, gpt_type_basic_data, sizeof(gpt_type_basic_data));
	sys_put_le64(first_lba, &scratch[32]);
	sys_put_le64(last_lba, &scratch[40]);

	if (off + entry_size <= SECTOR_SIZE) {
		memcpy(base, scratch, entry_size);
		return;
	}

	memcpy(base, scratch, SECTOR_SIZE - off);
	memcpy(&disk->image[(sec + 1U) * SECTOR_SIZE], scratch + (SECTOR_SIZE - off),
	       entry_size - (SECTOR_SIZE - off));
}

static void fake_gpt_header_and_entry(struct fake_disk *disk, uint32_t part_lba, uint64_t first_lba,
				      uint64_t last_lba)
{
	fake_gpt_header(disk, part_lba, 128U, 128U);
	fake_gpt_write_entry(disk, part_lba, 128U, 0U, first_lba, last_lba);
}

static void fake_disk_reset(uint32_t block_size)
{
	memset(&g_disk, 0, sizeof(g_disk));
	g_disk.block_size = block_size;
	g_disk.fail_lba = -1;
	g_disk.fail_err = -EIO;
}

static void fake_gpt_write_partition(struct fake_disk *disk, uint32_t part_lba, uint32_t entry_size,
				     uint32_t idx, uint64_t first, uint64_t last)
{
	fake_gpt_write_entry(disk, part_lba, entry_size, idx, first, last);
}

ZTEST(disk_partition, test_mbr_primary_walk)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_mbr_primary(&g_disk, 0x83, 2048U, 4096U);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_MBR, table.scheme);
	zassert_equal(1, table.entry_count);
	zassert_equal(2048U, table.entries[0].start_lba);
	zassert_equal(4096U, table.entries[0].nr_sectors);
	zassert_equal(0x83, table.entries[0].mbr_type);
}

ZTEST(disk_partition, test_mbr_type_byte_preserved)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_mbr_primary(&g_disk, 0x0C, 2048U, 8192U);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(1, table.entry_count);
	zassert_equal(2048U, table.entries[0].start_lba);
	zassert_equal(8192U, table.entries[0].nr_sectors);
	zassert_equal(0x0CU, table.entries[0].mbr_type);
}

ZTEST(disk_partition, test_mbr_skips_zero_sector_entry)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_mbr_slot(&g_disk, 0U, 0x83, 2048U, 0U);
	fake_mbr_slot(&g_disk, 1U, 0x83, 4096U, 1024U);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(1, table.entry_count);
	zassert_equal(4096U, table.entries[0].start_lba);
	zassert_equal(1024U, table.entries[0].nr_sectors);
}

ZTEST(disk_partition, test_partitionless_whole_disk_helper)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_NONE, table.scheme);
	zassert_equal(0, table.entry_count);

	ret = disk_partition_set_whole_disk(&geo, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_WHOLE_DISK, table.scheme);
	zassert_equal(1, table.entry_count);
	zassert_equal(0U, table.entries[0].start_lba);
	zassert_equal(DISK_SECTORS, table.entries[0].nr_sectors);
}

ZTEST(disk_partition, test_superfloppy_no_partition_table)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	g_disk.image[0] = 0xEBU;

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_NONE, table.scheme);
	zassert_equal(0, table.entry_count);
}

ZTEST(disk_partition, test_mbr_three_partitions_select_p2)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	const struct disk_partition_entry *p2;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_mbr_slot(&g_disk, 0U, 0x0CU, 2048U, 1024U);
	fake_mbr_slot(&g_disk, 1U, 0x0CU, 4096U, 8192U);
	fake_mbr_slot(&g_disk, 2U, 0x83U, 8192U, 4096U);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_MBR, table.scheme);
	zassert_equal(3, table.entry_count);

	p2 = &table.entries[1];
	zassert_equal(4096U, p2->start_lba);
	zassert_equal(8192U, p2->nr_sectors);
}

ZTEST(disk_partition, test_mbr_skips_entry_beyond_capacity)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_mbr_slot(&g_disk, 0U, 0x0CU, 2048U, 4096U);
	fake_mbr_slot(&g_disk, 1U, 0x0CU, 20000000U, 8192U);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_MBR, table.scheme);
	zassert_equal(1, table.entry_count);
	zassert_equal(2048U, table.entries[0].start_lba);
}

ZTEST(disk_partition, test_mbr_extended_two_ebr_logical_lbas)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_mbr_slot(&g_disk, 0U, DISK_PARTITION_MBR_EXTENDED_LBA, EXT_BASE, 8192U);
	fake_ebr_logical(&g_disk, EXT_BASE, 0x83, 2048U, 1024U, 512U,
			 DISK_PARTITION_MBR_EXTENDED_LBA);
	fake_ebr_logical(&g_disk, EXT_BASE + 512U, 0x83, 2048U, 2048U, 0U, 0U);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_MBR, table.scheme);
	zassert_equal(2, table.entry_count);
	zassert_equal(EXT_BASE + 2048U, table.entries[0].start_lba);
	zassert_equal(1024U, table.entries[0].nr_sectors);
	zassert_equal(EXT_BASE + 512U + 2048U, table.entries[1].start_lba);
	zassert_equal(2048U, table.entries[1].nr_sectors);
}

ZTEST(disk_partition, test_mbr_extended_skips_wrapping_logical_offset)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_mbr_slot(&g_disk, 0U, DISK_PARTITION_MBR_EXTENDED_LBA, EXT_BASE, 4096U);
	fake_ebr_logical(&g_disk, EXT_BASE, 0x83, UINT32_MAX, 1024U, 0U, 0U);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_NONE, table.scheme);
	zassert_equal(0, table.entry_count);
}

ZTEST(disk_partition, test_mbr_extended_stops_on_link_beyond_geometry)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_mbr_slot(&g_disk, 0U, DISK_PARTITION_MBR_EXTENDED_LBA, EXT_BASE, 4096U);
	fake_ebr_logical(&g_disk, EXT_BASE, 0x83, 2048U, 1024U, DISK_SECTORS,
			 DISK_PARTITION_MBR_EXTENDED_LBA);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(1, table.entry_count);
	zassert_equal(EXT_BASE + 2048U, table.entries[0].start_lba);
}

ZTEST(disk_partition, test_block_size_boundaries)
{
	struct disk_partition_table table;
	struct disk_partition_geo geo = {.sector_count = DISK_SECTORS};
	int ret;

	static const uint32_t invalid[] = {0U, 256U, 511U, 4097U};
	static const uint32_t valid[] = {512U, 4096U};

	for (size_t i = 0U; i < ARRAY_SIZE(invalid); i++) {
		fake_disk_reset(invalid[i]);
		geo.block_size = invalid[i];
		ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
		zassert_equal(-EINVAL, ret, "block_size %u", invalid[i]);
	}

	for (size_t i = 0U; i < ARRAY_SIZE(valid); i++) {
		fake_disk_reset(valid[i]);
		geo.block_size = valid[i];
		fake_mbr_primary(&g_disk, 0x83, 2048U, 4096U);
		ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
		zassert_ok(ret, "block_size %u", valid[i]);
		zassert_equal(1, table.entry_count);
	}
}

ZTEST(disk_partition, test_gpt_skips_zero_type_guid)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	uint8_t *ent1 = &g_disk.image[34U * SECTOR_SIZE + 128U];
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_gpt_protective_mbr(&g_disk);
	fake_gpt_header_and_entry(&g_disk, 34U, 2048U, 2048U + 4096U - 1U);
	/* Second slot: non-zero LBAs, all-zero type GUID (unused GPT entry). */
	memset(ent1, 0, 128U);
	sys_put_le64(8192U, &ent1[32]);
	sys_put_le64(12287U, &ent1[40]);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_GPT, table.scheme);
	zassert_equal(1, table.entry_count);
}

ZTEST(disk_partition, test_gpt_entry_byte_offset_misaligned)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	const uint64_t first = 4096U;
	const uint64_t last = first + 1024U - 1U;
	const uint32_t entry_size = 384U;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_gpt_protective_mbr(&g_disk);
	fake_gpt_header(&g_disk, 34U, 2U, entry_size);
	/* Second array slot (byte 384): entry_size does not divide block_size. */
	fake_gpt_write_entry(&g_disk, 34U, entry_size, 1U, first, last);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_GPT, table.scheme);
	zassert_equal(1, table.entry_count);
	zassert_equal(first, table.entries[0].start_lba);
	zassert_equal(1024U, table.entries[0].nr_sectors);
}

ZTEST(disk_partition, test_gpt_entry_beyond_old_32_sector_scan)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	const uint32_t part_lba = 34U;
	const uint32_t entry_size = 128U;
	const uint32_t part_count = 129U;
	const uint32_t target_idx = 128U;
	const uint64_t first = 8192U;
	const uint64_t last = first + 4096U - 1U;
	unsigned int i;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_gpt_protective_mbr(&g_disk);
	fake_gpt_header(&g_disk, part_lba, part_count, entry_size);

	for (i = 0U; i < target_idx; i++) {
		fake_gpt_write_entry(&g_disk, part_lba, entry_size, i, 0U, 0U);
	}

	fake_gpt_write_partition(&g_disk, part_lba, entry_size, target_idx, first, last);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_GPT, table.scheme);
	zassert_equal(1, table.entry_count);
	zassert_equal(first, table.entries[0].start_lba);
	zassert_equal(4096U, table.entries[0].nr_sectors);
}

ZTEST(disk_partition, test_gpt_stops_at_max_entries)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	const uint32_t part_lba = 34U;
	const uint32_t entry_size = 128U;
	const uint32_t want = DISK_PARTITION_MAX_ENTRIES + 4U;
	unsigned int i;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_gpt_protective_mbr(&g_disk);
	fake_gpt_header(&g_disk, part_lba, want, entry_size);

	for (i = 0U; i < want; i++) {
		const uint64_t first = 2048U + (uint64_t)i * 128U;
		const uint64_t last = first + 64U - 1U;

		fake_gpt_write_partition(&g_disk, part_lba, entry_size, i, first, last);
	}

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_GPT, table.scheme);
	zassert_equal(DISK_PARTITION_MAX_ENTRIES, table.entry_count);

	for (i = 0U; i < DISK_PARTITION_MAX_ENTRIES; i++) {
		const uint64_t first = 2048U + (uint64_t)i * 128U;

		zassert_equal(first, table.entries[i].start_lba);
		zassert_equal(64U, table.entries[i].nr_sectors);
	}
}

ZTEST(disk_partition, test_gpt_malformed_header_returns_einval)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	uint8_t *hdr = &g_disk.image[SECTOR_SIZE];
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_gpt_protective_mbr(&g_disk);
	memcpy(hdr, "EFI PART", 8);
	sys_put_le32(92U, &hdr[12]);
	sys_put_le64(34U, &hdr[72]);
	sys_put_le32(1U, &hdr[80]);
	sys_put_le32(64U, &hdr[84]); /* < 128 minimum entry size */

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_equal(-EINVAL, ret);
	zassert_equal(0, table.entry_count);
	zassert_equal(DISK_PARTITION_SCHEME_NONE, table.scheme);
}

ZTEST(disk_partition, test_gpt_array_past_disk_geometry_returns_einval)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = 64U};
	struct disk_partition_table table;
	uint8_t *hdr = &g_disk.image[SECTOR_SIZE];
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_gpt_protective_mbr(&g_disk);
	memcpy(hdr, "EFI PART", 8);
	sys_put_le32(92U, &hdr[12]);
	sys_put_le64(2U, &hdr[72]);
	sys_put_le32(512U, &hdr[80]);
	sys_put_le32(128U, &hdr[84]);

	g_disk.fail_lba = -2; /* die if any entry-array sector is read */
	g_disk.fail_err = -EIO;

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_equal(-EINVAL, ret);
	zassert_equal(0, table.entry_count);
}

ZTEST(disk_partition, test_gpt_read_error_propagates)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	const uint32_t part_lba = 34U;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_gpt_protective_mbr(&g_disk);
	fake_gpt_header_and_entry(&g_disk, part_lba, 2048U, 2048U + 4096U - 1U);
	g_disk.fail_lba = part_lba;
	g_disk.fail_err = -EIO;

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_equal(-EIO, ret);
	zassert_equal(0, table.entry_count);
}

ZTEST(disk_partition, test_gpt_protective_and_entry)
{
	struct disk_partition_geo geo = {.block_size = SECTOR_SIZE, .sector_count = DISK_SECTORS};
	struct disk_partition_table table;
	const uint64_t first = 2048U;
	const uint64_t last = first + 4096U - 1U;
	int ret;

	fake_disk_reset(SECTOR_SIZE);
	fake_gpt_protective_mbr(&g_disk);
	fake_gpt_header_and_entry(&g_disk, 34U, first, last);

	ret = disk_partition_walk(&geo, fake_read, &g_disk, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_GPT, table.scheme);
	zassert_equal(1, table.entry_count);
	zassert_equal(first, table.entries[0].start_lba);
	zassert_equal(4096U, table.entries[0].nr_sectors);
}

ZTEST_SUITE(disk_partition, NULL, NULL, NULL, NULL, NULL);
