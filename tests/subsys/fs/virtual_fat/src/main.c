/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/virtual_fat.h>

#include <zephyr/ztest.h>
#include <zephyr/ztest_mock.h>

#include <zephyr/sys/byteorder.h>

#include <errno.h>

#define BYTES_PER_FILE_ENTRY 32
#define RESERVED_CLUSTERS    2

struct fat1x_dir {
	char DIR_Name[8 + 3];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	/* Component of the file creation time. Valid range: 0 - 199 */
	uint8_t DIR_CrtTimeTenth;
	/* Creation time. Granularity is 2 seconds. */
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	/* High word of first data cluster number for file/directory described by this entry */
	uint16_t DIR_FstClusHI;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate_day: 5;
	uint16_t DIR_WrtDate_month: 4;
	uint16_t DIR_WrtDate_year: 7;
	/* Low word of first data cluster number for file/directory described by this entry.*/
	uint16_t DIR_FstClusLO;
	/* 32-bit quantity containing size in bytes of file/directory described by this entry. */
	uint32_t DIR_FileSize;
} __packed;

static const uint16_t allowed_bytes_per_sector[] = {512, 1024, 2048, 4096};
static const uint16_t allowed_sector_per_cluster[] = {1, 2, 4, 8, 16, 32, 64, 128};

bool is_one_of(uint16_t val, const uint16_t *list, size_t list_size)
{
	bool match = false;

	for (size_t i = 0; i < list_size; i++) {
		if (val == list[i]) {
			if (match) {
				return false;
			}
			match = true;
		}
	}

	return match;
}

#define SECTOR_SIZE 512

#define FAT12_PARTITION_SIZE       0x1000
#define FAT16_PARTITION_SIZE       0x1000000
#define FAT16_LARGE_PARTITION_SIZE 0x2000000

#define FAT12_VOLUME_LABEL       "FAT12_VOL  "
#define FAT16_VOLUME_LABEL       "FAT16_VOL  "
#define FAT16_LARGE_VOLUME_LABEL "FAT16_BIG  "

VIRTUAL_FAT_DEFINE_PARTITION(fat12, FAT12_VOLUME_LABEL, SECTOR_SIZE, FAT12_PARTITION_SIZE);
VIRTUAL_FAT_DEFINE_PARTITION(fat16, FAT16_VOLUME_LABEL, SECTOR_SIZE, FAT16_PARTITION_SIZE);
VIRTUAL_FAT_DEFINE_PARTITION(fat16_large, FAT16_LARGE_VOLUME_LABEL, SECTOR_SIZE,
			     FAT16_LARGE_PARTITION_SIZE);

ZTEST(virtual_fat, test_readBootSector_fat12)
{
	struct fat1x_BS boot_sector;

	virtual_fat_read(&fat12, (void *)&boot_sector, sizeof(boot_sector), 0, 0);

	const uint16_t bytes_per_sector = sys_le16_to_cpu(boot_sector.BPB_BytsPerSec);
	const uint32_t fat12_size =
		sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) +
		boot_sector.BPB_NumFATs * sys_le16_to_cpu(boot_sector.BPB_FATSz16) +
		(sys_le16_to_cpu(boot_sector.BPB_RootEntCnt) * BYTES_PER_FILE_ENTRY /
		 bytes_per_sector);
	const uint16_t partition_size = sys_le16_to_cpu(boot_sector.BPB_TotSec16) - fat12_size;
	const uint16_t total_sectors =
		DIV_ROUND_UP(FAT12_PARTITION_SIZE, bytes_per_sector) + fat12_size;
	const uint16_t num_clusters = DIV_ROUND_UP(partition_size, boot_sector.BPB_SecPerClus);
	const uint16_t num_clusters_per_fat =
		DIV_ROUND_UP(DIV_ROUND_UP(num_clusters * 3, 2), bytes_per_sector);

	zassert_true(is_one_of(bytes_per_sector, allowed_bytes_per_sector,
			       ARRAY_SIZE(allowed_bytes_per_sector)),
		     "invalid bytes per sector");
	zassert_equal(bytes_per_sector, SECTOR_SIZE, "bytes per sec must match");
	zassert_true(is_one_of(boot_sector.BPB_SecPerClus, allowed_sector_per_cluster,
			       ARRAY_SIZE(allowed_sector_per_cluster)),
		     "invalid sectors per cluster %u", boot_sector.BPB_SecPerClus);
	zassert_true(boot_sector.BPB_SecPerClus * SECTOR_SIZE < FAT12_PARTITION_SIZE,
		     "too many sectors per cluster");
	zassert_true(sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) >= 1,
		     "At least the BS must be reserved");
	zassert_equal(
		(sys_le16_to_cpu(boot_sector.BPB_RootEntCnt) * BYTES_PER_FILE_ENTRY % SECTOR_SIZE),
		0, "BPB_RootEntCnt * 32 %% BPB_BytsPerSec has to be 0");
	zassert_equal(sys_le16_to_cpu(boot_sector.BPB_TotSec16), total_sectors,
		      "total sector count is wrong %u vs %u",
		      sys_le16_to_cpu(boot_sector.BPB_TotSec16), total_sectors);
	zassert_equal(sys_le16_to_cpu(boot_sector.BPB_FATSz16), num_clusters_per_fat,
		      "number of sectors per fat is wrong [%u vs %u]",
		      sys_le16_to_cpu(boot_sector.BPB_FATSz16), num_clusters_per_fat);
	zassert_mem_equal(boot_sector.BS_VolLab, FAT12_VOLUME_LABEL, sizeof(boot_sector.BS_VolLab),
			  "label not correct");
	zassert_mem_equal(boot_sector.BS_FilSysType, "FAT12   ", sizeof(boot_sector.BS_FilSysType),
			  "filesystem type string not correct");
}

ZTEST(virtual_fat, test_readBootSector_fat16)
{
	struct fat1x_BS boot_sector;

	virtual_fat_read(&fat16, (void *)&boot_sector, sizeof(boot_sector), 0, 0);

	const uint16_t bytes_per_sector = sys_le16_to_cpu(boot_sector.BPB_BytsPerSec);
	const uint32_t fat16_size =
		sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) +
		boot_sector.BPB_NumFATs * sys_le16_to_cpu(boot_sector.BPB_FATSz16) +
		(sys_le16_to_cpu(boot_sector.BPB_RootEntCnt) * BYTES_PER_FILE_ENTRY /
		 bytes_per_sector);
	const uint16_t partition_size = sys_le16_to_cpu(boot_sector.BPB_TotSec16) - fat16_size;
	const uint16_t total_sectors =
		DIV_ROUND_UP(FAT16_PARTITION_SIZE, bytes_per_sector) + fat16_size;
	const uint16_t num_clusters = DIV_ROUND_UP(partition_size, boot_sector.BPB_SecPerClus);
	const uint16_t num_clusters_per_fat = DIV_ROUND_UP(num_clusters * 2, bytes_per_sector);

	zassert_true(is_one_of(bytes_per_sector, allowed_bytes_per_sector,
			       ARRAY_SIZE(allowed_bytes_per_sector)),
		     "invalid bytes per sector");
	zassert_equal(bytes_per_sector, SECTOR_SIZE, "bytes per sec must match");
	zassert_true(is_one_of(boot_sector.BPB_SecPerClus, allowed_sector_per_cluster,
			       ARRAY_SIZE(allowed_sector_per_cluster)),
		     "invalid sectors per cluster %u", boot_sector.BPB_SecPerClus);
	zassert_true(boot_sector.BPB_SecPerClus * SECTOR_SIZE < FAT16_PARTITION_SIZE,
		     "too many sectors per cluster");
	zassert_true(sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) >= 1,
		     "At least the BS must be reserved");
	zassert_equal(
		(sys_le16_to_cpu(boot_sector.BPB_RootEntCnt) * BYTES_PER_FILE_ENTRY % SECTOR_SIZE),
		0, "BPB_RootEntCnt * 32 %% BPB_BytsPerSec has to be 0");
	zassert_equal(sys_le16_to_cpu(boot_sector.BPB_TotSec16), total_sectors,
		      "total sector count is wrong %u vs %u",
		      sys_le16_to_cpu(boot_sector.BPB_TotSec16), total_sectors);
	zassert_equal(sys_le16_to_cpu(boot_sector.BPB_FATSz16), num_clusters_per_fat,
		      "number of sectors per fat is wrong [%u vs %u]",
		      sys_le16_to_cpu(boot_sector.BPB_FATSz16), num_clusters_per_fat);
	zassert_mem_equal(boot_sector.BS_VolLab, FAT16_VOLUME_LABEL, sizeof(boot_sector.BS_VolLab),
			  "label not correct");
	zassert_mem_equal(boot_sector.BS_FilSysType, "FAT16   ", sizeof(boot_sector.BS_FilSysType),
			  "filesystem type string not correct");
}

ZTEST(virtual_fat, test_readBootSector_signature_at_offset_510)
{
	uint8_t signature[2] = {0};

	virtual_fat_read(&fat12, signature, sizeof(signature), 0, 510);

	zassert_equal(sys_get_le16(signature), 0xaa55,
		      "boot sector signature does not match 0xaa55 (is 0x%04x)",
		      sys_get_le16(signature));
}

ZTEST(virtual_fat, test_readBootSector_totSec_greater_than_0xffff_count_is_in_totSec32)
{
	struct fat1x_BS boot_sector;

	virtual_fat_read(&fat16_large, (void *)&boot_sector, sizeof(boot_sector), 0, 0);

	const uint32_t total_sectors = FAT_BS_TOT_SEC(SECTOR_SIZE, FAT16_LARGE_PARTITION_SIZE);

	zassert_true(total_sectors > 0xffff, "test setup must exceed 0xffff sectors");
	zassert_equal(sys_le16_to_cpu(boot_sector.BPB_TotSec16), 0,
		      "BPB_TotSec16 must be 0 when total sectors exceed 0xffff");
	zassert_equal(sys_le32_to_cpu(boot_sector.BPB_TotSec32), total_sectors,
		      "BPB_TotSec32 must hold full total sector count");
}

static bool all_zero(uint8_t *buf, size_t size)
{
	for (size_t i = 0; i < size; ++i) {
		if (buf[i] != 0) {
			return false;
		}
	}
	return true;
}

static inline size_t fat12_cluster_to_index(uint16_t cluster)
{
	return cluster + cluster / 2;
}

static uint16_t fat12_entry_from_cluster(uint8_t *fat, uint16_t cluster)
{
	uint16_t index = fat12_cluster_to_index(cluster);
	uint16_t data = fat[index] | (fat[index + 1] << 8);

	if (cluster & 0x0001) {
		return data >> 4;
	}

	return data & 0x0fff;
}

static void check_fat_fat12(uint16_t fat_start, uint16_t fat_size, const struct virtual_file *files,
			    size_t num_files, uint16_t bytes_per_sector,
			    uint16_t sectors_per_cluster, uint8_t media_type)
{
	uint16_t cluster = RESERVED_CLUSTERS;
	uint32_t bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
	uint8_t buf[64];

	virtual_fat_read(&fat12, buf, sizeof(buf), fat_start, 0);

	zassert_equal(buf[0], media_type, "First fat entry byte must be the media type");
	zassert_equal(buf[1] | (buf[2] << 8), 0xffff, "Reserced cluster must be 0xffff");

	for (size_t i = 0; i < num_files; ++i) {
		const uint16_t next_file_cluster =
			cluster + DIV_ROUND_UP(files[i].file_size, bytes_per_cluster);
		for (; cluster < next_file_cluster; ++cluster) {
			if (cluster == next_file_cluster - 1) {
				zassert_equal(fat12_entry_from_cluster(buf, cluster), 0xfff,
					      "End cluster must be 0xfff");
			} else {
				zassert_equal(fat12_entry_from_cluster(buf, cluster), cluster + 1,
					      "successive cluster number expected");
			}
		}
	}

	uint16_t zeroes_offset = fat_start * bytes_per_sector + fat12_cluster_to_index(cluster);
	size_t zeroes_len = fat_size - fat12_cluster_to_index(cluster);

	while (zeroes_len) {
		const uint32_t lba = zeroes_offset / bytes_per_sector;
		const uint32_t lba_offs = zeroes_offset % bytes_per_sector;
		const size_t len = MIN(sizeof(buf), zeroes_len);

		virtual_fat_read(&fat12, buf, len, lba, lba_offs);
		zassert_true(all_zero(buf, len), "FAT must be all zero lba %u offs %u", lba,
			     lba_offs);
		zeroes_len -= len;
		zeroes_offset += len;
	}
}

ZTEST(virtual_fat, test_readFat_fat12)
{
	struct fat1x_BS boot_sector;

	virtual_fat_read(&fat12, (void *)&boot_sector, sizeof(boot_sector), 0, 0);

	const uint16_t bytes_per_sector = sys_le16_to_cpu(boot_sector.BPB_BytsPerSec);
	const uint32_t fat_start = sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt);
	const uint32_t fat_size = sys_le16_to_cpu(boot_sector.BPB_FATSz16) * bytes_per_sector;

	const struct virtual_file files[] = {
		{.name = "TEST1",
		 .ext = "TXT",
		 .file_size = bytes_per_sector * boot_sector.BPB_SecPerClus},
		{.name = "TEST2",
		 .ext = "TXT",
		 .file_size = bytes_per_sector * boot_sector.BPB_SecPerClus * 2},
		{.name = "TEST3",
		 .ext = "TXT",
		 .file_size = bytes_per_sector * boot_sector.BPB_SecPerClus},
		{.name = "TEST4",
		 .ext = "TXT",
		 .file_size = bytes_per_sector * boot_sector.BPB_SecPerClus * 5}};

	register_virtual_files(&fat12, files, ARRAY_SIZE(files));

	check_fat_fat12(fat_start, fat_size, files, ARRAY_SIZE(files), bytes_per_sector,
			boot_sector.BPB_SecPerClus, boot_sector.BPB_Media);
	check_fat_fat12(fat_start + sys_le16_to_cpu(boot_sector.BPB_FATSz16), fat_size, files,
			ARRAY_SIZE(files), bytes_per_sector, boot_sector.BPB_SecPerClus,
			boot_sector.BPB_Media);
}

static inline size_t fat16_cluster_to_index(uint16_t cluster)
{
	return cluster * 2;
}

static uint16_t fat16_entry_from_cluster(const uint8_t *fat, uint16_t cluster)
{
	const uint16_t index = fat16_cluster_to_index(cluster);

	return sys_get_le16(&fat[index]);
}

static void check_fat_fat16(uint16_t fat_start, uint16_t fat_size, const struct virtual_file *files,
			    size_t num_files, uint16_t bytes_per_sector,
			    uint16_t sectors_per_cluster, uint8_t media_type)
{
	uint16_t cluster = RESERVED_CLUSTERS;
	uint32_t bytes_per_cluster = bytes_per_sector * sectors_per_cluster;
	uint8_t buf[64];

	virtual_fat_read(&fat16, buf, sizeof(buf), fat_start, 0);

	zassert_equal(buf[0], media_type, "First fat entry byte must be the media type");
	zassert_equal(buf[1] | (buf[2] << 8), 0xffff, "Reserced cluster must be 0xffff");

	for (size_t i = 0; i < num_files; ++i) {
		const uint16_t next_file_cluster =
			cluster + DIV_ROUND_UP(files[i].file_size, bytes_per_cluster);
		for (; cluster < next_file_cluster; ++cluster) {
			if (cluster == next_file_cluster - 1) {
				zassert_equal(fat16_entry_from_cluster(buf, cluster), 0xffff,
					      "End cluster must be 0xffff (is 0x%04x)",
					      fat16_entry_from_cluster(buf, cluster));
			} else {
				zassert_equal(fat16_entry_from_cluster(buf, cluster), cluster + 1,
					      "successive cluster number expected");
			}
		}
	}

	uint16_t zeroes_offset = fat_start * bytes_per_sector + fat16_cluster_to_index(cluster);
	size_t zeroes_len = fat_size - fat16_cluster_to_index(cluster);

	while (zeroes_len) {
		const uint32_t lba = zeroes_offset / bytes_per_sector;
		const uint32_t lba_offs = zeroes_offset % bytes_per_sector;
		const size_t len = MIN(sizeof(buf), zeroes_len);

		virtual_fat_read(&fat16, buf, len, lba, lba_offs);
		zassert_true(all_zero(buf, len), "FAT must be all zero lba %u offs %u", lba,
			     lba_offs);
		zeroes_len -= len;
		zeroes_offset += len;
	}
}

ZTEST(virtual_fat, test_readFat_fat16)
{
	struct fat1x_BS boot_sector;

	virtual_fat_read(&fat16, (void *)&boot_sector, sizeof(boot_sector), 0, 0);

	const uint16_t bytes_per_sector = sys_le16_to_cpu(boot_sector.BPB_BytsPerSec);
	const uint32_t fat_start = sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt);
	const uint32_t fat_size = sys_le16_to_cpu(boot_sector.BPB_FATSz16) * bytes_per_sector;

	const struct virtual_file files[] = {
		{.name = "TEST1",
		 .ext = "TXT",
		 .file_size = bytes_per_sector * boot_sector.BPB_SecPerClus},
		{.name = "TEST2",
		 .ext = "TXT",
		 .file_size = bytes_per_sector * boot_sector.BPB_SecPerClus * 2},
		{.name = "TEST3",
		 .ext = "TXT",
		 .file_size = bytes_per_sector * boot_sector.BPB_SecPerClus},
		{.name = "TEST4",
		 .ext = "TXT",
		 .file_size = bytes_per_sector * boot_sector.BPB_SecPerClus * 5}};

	register_virtual_files(&fat16, files, ARRAY_SIZE(files));

	check_fat_fat16(fat_start, fat_size, files, ARRAY_SIZE(files), bytes_per_sector,
			boot_sector.BPB_SecPerClus, boot_sector.BPB_Media);
	check_fat_fat16(fat_start + sys_le16_to_cpu(boot_sector.BPB_FATSz16), fat_size, files,
			ARRAY_SIZE(files), bytes_per_sector, boot_sector.BPB_SecPerClus,
			boot_sector.BPB_Media);
}

ZTEST(virtual_fat, test_readVolLabel_fat12)
{
	struct fat1x_BS boot_sector;
	struct fat1x_dir vol_label;
	const char *vol_label_str = FAT12_VOLUME_LABEL;

	set_volume_label(&fat12, vol_label_str);

	virtual_fat_read(&fat12, (void *)&boot_sector, sizeof(boot_sector), 0, 0);
	uint32_t root_dir_start =
		sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) +
		sys_le16_to_cpu(boot_sector.BPB_FATSz16) * boot_sector.BPB_NumFATs;
	virtual_fat_read(&fat12, (void *)&vol_label, sizeof(vol_label), root_dir_start, 0);

	zassert_mem_equal(vol_label.DIR_Name, vol_label_str, sizeof(vol_label.DIR_Name),
			  "label not correct %s vs %s", vol_label.DIR_Name, vol_label_str);
	zassert_equal(vol_label.DIR_Attr, 0x08, "Label needs attribute 0x08");
	zassert_equal(sys_le32_to_cpu(vol_label.DIR_FileSize), 0, "Vol label size must be 0");
}

ZTEST(virtual_fat, test_readLongVolLabel_fat12)
{
	struct fat1x_BS boot_sector;
	struct fat1x_dir vol_label;
	const char *vol_label_str = "A_LONG_LBL";

	set_volume_label(&fat12, vol_label_str);

	virtual_fat_read(&fat12, (void *)&boot_sector, sizeof(boot_sector), 0, 0);
	uint32_t root_dir_start =
		sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) +
		sys_le16_to_cpu(boot_sector.BPB_FATSz16) * boot_sector.BPB_NumFATs;
	virtual_fat_read(&fat12, (void *)&vol_label, sizeof(vol_label), root_dir_start, 0);

	zassert_mem_equal(vol_label.DIR_Name, vol_label_str, sizeof(vol_label.DIR_Name),
			  "label not correct");
	zassert_equal(vol_label.DIR_Attr, 0x08, "Label needs attribute 0x08");
	zassert_equal(sys_le32_to_cpu(vol_label.DIR_FileSize), 0, "Vol label size must be 0");
}

ZTEST(virtual_fat, test_readVolLabel_fat16)
{
	struct fat1x_BS boot_sector;
	struct fat1x_dir vol_label;
	const char *vol_label_str = FAT16_VOLUME_LABEL;

	set_volume_label(&fat16, vol_label_str);

	virtual_fat_read(&fat16, (void *)&boot_sector, sizeof(boot_sector), 0, 0);
	uint32_t root_dir_start =
		sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) +
		sys_le16_to_cpu(boot_sector.BPB_FATSz16) * boot_sector.BPB_NumFATs;
	virtual_fat_read(&fat16, (void *)&vol_label, sizeof(vol_label), root_dir_start, 0);

	zassert_mem_equal(vol_label.DIR_Name, vol_label_str, sizeof(vol_label.DIR_Name),
			  "label not correct %s vs %s", vol_label.DIR_Name, vol_label_str);
	zassert_equal(vol_label.DIR_Attr, FAT_ATTR_VOLUME_ID,
		      "Label needs attribute FAT_ATTR_VOLUME_ID");
	zassert_equal(sys_le32_to_cpu(vol_label.DIR_FileSize), 0, "Vol label size must be 0");
}

void check_dir(const struct virtual_file *file, struct fat1x_dir *entry)
{
	char file_name[sizeof(entry->DIR_Name)];

	memset(file_name, ' ', sizeof(file_name));
	memcpy(file_name, file->name, strlen(file->name));
	memcpy(file_name + sizeof(file_name) - sizeof(file->ext), file->ext, sizeof(file->ext));
	zassert_mem_equal(entry->DIR_Name, file_name, sizeof(entry->DIR_Name),
			  "Filename not correct");
	zassert_equal(sys_le32_to_cpu(entry->DIR_FileSize), file->file_size, "File-Size mismatch");
	zassert_equal(entry->DIR_Attr, file->attributes, "attribute mismatch");
}

ZTEST(virtual_fat, test_readRootDir)
{
	const struct virtual_file virtual_files[] = {
		{.name = "TEST1", .ext = "TXT", .file_size = 1024},
		{.name = "TEST2", .ext = "TXT", .attributes = FAT_ATTR_READ_ONLY, .file_size = 10}};
	struct fat1x_BS boot_sector;
	struct fat1x_dir files[ARRAY_SIZE(virtual_files)];

	virtual_fat_read(&fat12, (void *)&boot_sector, sizeof(boot_sector), 0, 0);

	uint32_t root_dir_start =
		sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) +
		sys_le16_to_cpu(boot_sector.BPB_FATSz16) * boot_sector.BPB_NumFATs;

	register_virtual_files(&fat12, virtual_files, ARRAY_SIZE(files));

	virtual_fat_read(&fat12, (void *)&files, sizeof(files), root_dir_start,
			 sizeof(struct fat1x_dir));
	for (size_t i = 0; i < ARRAY_SIZE(files); ++i) {
		check_dir(&virtual_files[i], &files[i]);
	}
}

static size_t get_file_chunk(off_t offset, uint8_t *buf, size_t len, void *user_data)
{
	ARG_UNUSED(buf);
	ztest_check_expected_value(offset);
	ztest_check_expected_value(len);
	ztest_check_expected_value(user_data);
	return len;
}

static void check_file_read(const struct virtual_file *file, off_t start_lba,
			    uint16_t bytes_per_sector)
{
	uint8_t buf[64];

	for (off_t offset = 0; offset < file->file_size; offset += sizeof(buf)) {
		size_t len = MIN(sizeof(buf), file->file_size - offset);
		const uint32_t lba = start_lba + offset / bytes_per_sector;
		const uint32_t lba_offs = offset % bytes_per_sector;

		ztest_expect_value(get_file_chunk, offset, offset);
		ztest_expect_value(get_file_chunk, len, len);
		ztest_expect_value(get_file_chunk, user_data, file->user_data);

		virtual_fat_read(&fat16, buf, len, lba, lba_offs);
	}
}

ZTEST(virtual_fat, test_readFile)
{
	const struct virtual_file files[] = {{.name = "TEST1",
					      .ext = "TXT",
					      .file_size = 1024,
					      .get_chunk = get_file_chunk,
					      .user_data = (void *)123},
					     {.name = "TEST2",
					      .ext = "TXT",
					      .attributes = FAT_ATTR_READ_ONLY,
					      .file_size = 10,
					      .get_chunk = get_file_chunk},
					     {.name = "TEST3",
					      .ext = "TXT",
					      .attributes = 0,
					      .file_size = 32,
					      .get_chunk = get_file_chunk}};
	struct fat1x_BS boot_sector;

	virtual_fat_read(&fat16, (void *)&boot_sector, sizeof(boot_sector), 0, 0);

	const uint32_t root_dir_start =
		sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) +
		sys_le16_to_cpu(boot_sector.BPB_FATSz16) * boot_sector.BPB_NumFATs;
	const uint16_t bytes_per_sector = sys_le16_to_cpu(boot_sector.BPB_BytsPerSec);
	uint32_t file_start = root_dir_start +
			      sys_le16_to_cpu(boot_sector.BPB_RootEntCnt) * 32 / bytes_per_sector;

	zassert_ok(register_virtual_files(&fat16, files, ARRAY_SIZE(files)));

	for (size_t i = 0; i < ARRAY_SIZE(files); ++i) {
		check_file_read(&files[i], file_start, bytes_per_sector);
		file_start += DIV_ROUND_UP(files[i].file_size,
					   bytes_per_sector * boot_sector.BPB_SecPerClus) *
			      boot_sector.BPB_SecPerClus;
	}
}

static void put_new_file_chunk(off_t offset, const uint8_t *buf, size_t len, void *user_data)
{
	ztest_check_expected_value(offset);
	ztest_check_expected_value(len);
	ztest_check_expected_data(buf, len);
	ztest_check_expected_value(user_data);
}

ZTEST(virtual_fat, test_writeNewFile)
{
	const char *data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do e";
	const size_t data_len = strlen(data);
	const struct virtual_file files[] = {
		{.name = "TEST1", .ext = "TXT", .file_size = 1024, .user_data = (void *)1}};
	struct fat1x_BS boot_sector;

	virtual_fat_read(&fat12, (void *)&boot_sector, sizeof(boot_sector), 0, 0);

	const uint32_t root_dir_start =
		sys_le16_to_cpu(boot_sector.BPB_RsvdSecCnt) +
		sys_le16_to_cpu(boot_sector.BPB_FATSz16) * boot_sector.BPB_NumFATs;
	const uint16_t bytes_per_sector = sys_le16_to_cpu(boot_sector.BPB_BytsPerSec);
	const uint32_t file_start = root_dir_start + sys_le16_to_cpu(boot_sector.BPB_RootEntCnt) *
							     32 / bytes_per_sector;
	const uint32_t new_file_start = file_start + boot_sector.BPB_SecPerClus;

	zassert_ok(register_virtual_files(&fat12, files, ARRAY_SIZE(files)));
	register_new_file_write_cb(&fat12, put_new_file_chunk, (void *)1);

	ztest_expect_value(put_new_file_chunk, offset, 0);
	ztest_expect_value(put_new_file_chunk, len, data_len);
	ztest_expect_value(put_new_file_chunk, user_data, 1);
	ztest_expect_data(put_new_file_chunk, buf, data);

	zassert_ok(virtual_fat_write(&fat12, data, data_len, new_file_start, 0),
		   "from here, there should be new data");

	const int res =
		virtual_fat_write(&fat12, NULL, 0, new_file_start + boot_sector.BPB_SecPerClus, 0);
	zassert_equal(res, -EINVAL, "This sector should be over the limit");
}

ZTEST_SUITE(virtual_fat, NULL, NULL, NULL, NULL, NULL);
