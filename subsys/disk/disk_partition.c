/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_partition.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(disk_partition, CONFIG_DISK_LOG_LEVEL);

#define MBR_PARTITION_TYPE_GPT_PROTECTIVE 0xEEU
#define DISK_PART_MAX_EBR_CHAIN           64U
#define DISK_PART_GPT_ENTRY_SEC_MAX       32U

#define EXT2_SUPERBLOCK_OFFSET 1024U
#define EXT2_MAGIC_OFFSET      56U
#define EXT2_MAGIC             0xEF53U

static bool disk_part_mbr_type_is_extended(uint8_t type)
{
	return type == DISK_PARTITION_MBR_EXTENDED || type == DISK_PARTITION_MBR_EXTENDED_LBA ||
	       type == DISK_PARTITION_MBR_EXTENDED_W2K;
}

static bool disk_part_boot_signature_ok(const uint8_t *buf, uint32_t len)
{
	return len >= 512U && buf[510] == 0x55U && buf[511] == 0xAAU;
}

static bool disk_part_has_fat_bpb(const uint8_t *buf, uint32_t len)
{
	if (len < 90U) {
		return false;
	}

	if (memcmp(&buf[3], "EXFAT   ", 8) == 0) {
		return disk_part_boot_signature_ok(buf, len);
	}

	if (buf[0] != 0xEB && buf[0] != 0xE9) {
		return false;
	}

	if (memcmp(&buf[82], "FAT32   ", 8) == 0) {
		return true;
	}
	if (len >= 58U && memcmp(&buf[54], "FAT16   ", 8) == 0) {
		return true;
	}
	if (len >= 58U && memcmp(&buf[54], "FAT12   ", 8) == 0) {
		return true;
	}

	return disk_part_boot_signature_ok(buf, len);
}

static bool disk_part_has_ext2_superblock(const struct disk_partition_geo *geo,
					  disk_partition_read_sectors_t read, void *read_ctx,
					  uint64_t part_start_lba, uint8_t *sector)
{
	const uint32_t bs = geo->block_size;
	const uint64_t magic_lba =
		part_start_lba + ((EXT2_SUPERBLOCK_OFFSET + EXT2_MAGIC_OFFSET) / bs);
	const uint32_t magic_off = (EXT2_SUPERBLOCK_OFFSET + EXT2_MAGIC_OFFSET) % bs;
	uint16_t magic;
	int ret;

	if (sector == NULL || bs < 512U || magic_off + sizeof(magic) > bs) {
		return false;
	}

	ret = read(read_ctx, magic_lba, sector, bs);
	if (ret != 0) {
		return false;
	}

	magic = sys_get_le16(&sector[magic_off]);

	return magic == EXT2_MAGIC;
}

static uint64_t disk_part_clamp_volume_sectors(uint64_t volume_base, uint64_t part_sectors,
					       uint64_t disk_sectors)
{
	if (part_sectors == 0U) {
		if (volume_base >= disk_sectors) {
			return 0U;
		}
		return disk_sectors - volume_base;
	}

	if (volume_base + part_sectors < volume_base || volume_base + part_sectors > disk_sectors) {
		if (volume_base >= disk_sectors) {
			return 0U;
		}
		return disk_sectors - volume_base;
	}

	return part_sectors;
}

static void disk_part_table_clear(struct disk_partition_table *table)
{
	table->scheme = DISK_PARTITION_SCHEME_NONE;
	table->entry_count = 0U;
	(void)memset(table->entries, 0, sizeof(table->entries));
}

static int disk_part_table_add(struct disk_partition_table *table, uint64_t start_lba,
			       uint64_t nr_sectors, uint8_t mbr_type, const uint8_t type_guid[16])
{
	struct disk_partition_entry *ent;

	if (table->entry_count >= DISK_PARTITION_MAX_ENTRIES) {
		return -ENOSPC;
	}

	ent = &table->entries[table->entry_count];
	ent->start_lba = start_lba;
	ent->nr_sectors = nr_sectors;
	ent->mbr_type = mbr_type;
	if (type_guid != NULL) {
		(void)memcpy(ent->type_guid, type_guid, 16);
	} else {
		(void)memset(ent->type_guid, 0, 16);
	}
	table->entry_count++;

	return 0;
}

static int disk_part_parse_mbr_primaries(const uint8_t *mbr, struct disk_partition_table *table,
					 bool *gpt_protective, uint32_t *extended_base)
{
	bool protective = false;
	uint32_t ext_base = 0U;
	int ret;

	if (!disk_part_boot_signature_ok(mbr, 512U)) {
		return -ENOENT;
	}

	for (unsigned int i = 0U; i < 4U; i++) {
		const uint8_t *pe = &mbr[0x1BE + (i * 16U)];
		const uint8_t type = pe[4];
		const uint32_t start = sys_get_le32(&pe[8]);
		const uint32_t sectors = sys_get_le32(&pe[12]);

		if (type == 0U) {
			continue;
		}

		if (type == MBR_PARTITION_TYPE_GPT_PROTECTIVE) {
			protective = true;
			continue;
		}

		if (disk_part_mbr_type_is_extended(type)) {
			ext_base = start;
			continue;
		}

		ret = disk_part_table_add(table, start, sectors, type, NULL);
		if (ret != 0) {
			return ret;
		}
	}

	*gpt_protective = protective;
	*extended_base = ext_base;

	if (table->entry_count > 0U || ext_base != 0U) {
		return 0;
	}

	if (protective) {
		return -EAGAIN;
	}

	return -ENOENT;
}

static int disk_part_parse_mbr_extended(const struct disk_partition_geo *geo,
					disk_partition_read_sectors_t read, void *read_ctx,
					uint32_t extended_base, uint8_t *sector,
					struct disk_partition_table *table)
{
	uint32_t next_ebr = 0U;
	int ret;

	for (unsigned int link = 0U; link < DISK_PART_MAX_EBR_CHAIN; link++) {
		const uint64_t ebr_lba = extended_base + next_ebr;
		const uint8_t *pe0;
		const uint8_t *pe1;
		uint8_t type0;
		uint8_t type1;
		uint32_t start0;
		uint32_t sectors0;
		uint32_t link_rel;

		ret = read(read_ctx, ebr_lba, sector, geo->block_size);
		if (ret != 0) {
			return ret;
		}

		if (!disk_part_boot_signature_ok(sector, geo->block_size)) {
			return -EINVAL;
		}

		pe0 = &sector[0x1BE];
		pe1 = &sector[0x1BE + 16U];
		type0 = pe0[4];
		type1 = pe1[4];
		start0 = sys_get_le32(&pe0[8]);
		sectors0 = sys_get_le32(&pe0[12]);
		link_rel = sys_get_le32(&pe1[8]);

		if (type0 != 0U && !disk_part_mbr_type_is_extended(type0)) {
			const uint64_t abs_start = extended_base + start0;

			ret = disk_part_table_add(table, abs_start, sectors0, type0, NULL);
			if (ret != 0) {
				return ret;
			}
		}

		if (type1 != DISK_PARTITION_MBR_EXTENDED &&
		    type1 != DISK_PARTITION_MBR_EXTENDED_LBA) {
			break;
		}

		if (link_rel == 0U) {
			break;
		}

		next_ebr = link_rel;
	}

	return 0;
}

static int disk_part_parse_gpt(const struct disk_partition_geo *geo,
			       disk_partition_read_sectors_t read, void *read_ctx, uint8_t *sector,
			       struct disk_partition_table *table)
{
	uint64_t disk_last_lba;
	uint32_t part_lba;
	uint32_t part_count;
	uint32_t entry_size;
	uint32_t entry_secs;
	uint32_t sec;
	int ret;

	disk_last_lba = (geo->sector_count == 0U) ? 0U : (geo->sector_count - 1U);
	if (disk_last_lba < 2U) {
		return -EINVAL;
	}

	ret = read(read_ctx, 1U, sector, geo->block_size);
	if (ret != 0) {
		return ret;
	}

	if (memcmp(sector, "EFI PART", 8) != 0) {
		return -ENOENT;
	}

	part_lba = sys_get_le32(&sector[72]);
	part_count = sys_get_le32(&sector[80]);
	entry_size = sys_get_le32(&sector[84]);
	if (part_lba == 0U || part_count == 0U || entry_size < 128U || entry_size > 512U) {
		return -EINVAL;
	}

	disk_part_table_clear(table);
	table->scheme = DISK_PARTITION_SCHEME_GPT;

	entry_secs = (part_count * entry_size + geo->block_size - 1U) / geo->block_size;
	entry_secs = MIN(entry_secs, DISK_PART_GPT_ENTRY_SEC_MAX);

	for (sec = 0U; sec < entry_secs; sec++) {
		const uint32_t entries_per_sec = geo->block_size / entry_size;
		const uint32_t base_index = sec * entries_per_sec;

		ret = read(read_ctx, part_lba + sec, sector, geo->block_size);
		if (ret != 0) {
			return ret;
		}

		for (uint32_t i = 0U; i < entries_per_sec && (base_index + i) < part_count; i++) {
			const uint8_t *ent = &sector[i * entry_size];
			const uint64_t first = sys_get_le64(&ent[32]);
			const uint64_t last = sys_get_le64(&ent[40]);
			uint64_t nr_sectors;

			if (first == 0U && last == 0U) {
				continue;
			}

			if (last < first) {
				continue;
			}

			nr_sectors = last - first + 1U;
			ret = disk_part_table_add(table, first, nr_sectors, 0U, ent);
			if (ret != 0) {
				return ret;
			}
		}
	}

	if (table->entry_count == 0U) {
		table->scheme = DISK_PARTITION_SCHEME_NONE;
		return -ENOENT;
	}

	return 0;
}

int disk_partition_set_whole_disk(const struct disk_partition_geo *geo,
				  struct disk_partition_table *table)
{
	int ret;

	if (geo == NULL || table == NULL || geo->sector_count == 0U) {
		return -EINVAL;
	}

	disk_part_table_clear(table);
	ret = disk_part_table_add(table, 0U, geo->sector_count, 0U, NULL);
	if (ret != 0) {
		return ret;
	}

	table->scheme = DISK_PARTITION_SCHEME_WHOLE_DISK;
	return 0;
}

static int disk_partition_walk_with_sector(const struct disk_partition_geo *geo,
					   disk_partition_read_sectors_t read, void *read_ctx,
					   struct disk_partition_table *table, uint8_t *sector)
{
	bool gpt_protective = false;
	uint32_t extended_base = 0U;
	int ret;

	if (geo == NULL || read == NULL || table == NULL || sector == NULL) {
		return -EINVAL;
	}

	if (geo->block_size == 0U || geo->block_size > DISK_PARTITION_MAX_BLOCK_SIZE) {
		return -EINVAL;
	}

	disk_part_table_clear(table);

	ret = read(read_ctx, 0U, sector, geo->block_size);
	if (ret != 0) {
		return ret;
	}

	ret = disk_part_parse_mbr_primaries(sector, table, &gpt_protective, &extended_base);
	if (ret == 0) {
		if (extended_base != 0U) {
			ret = disk_part_parse_mbr_extended(geo, read, read_ctx, extended_base,
							   sector, table);
			if (ret != 0) {
				return ret;
			}
		}
		if (table->entry_count > 0U) {
			table->scheme = DISK_PARTITION_SCHEME_MBR;
			LOG_DBG("disk_part: MBR with %u slot(s)", table->entry_count);
			return 0;
		}
		if (gpt_protective) {
			ret = -EAGAIN;
		}
	}

	if (ret == -EAGAIN || gpt_protective) {
		ret = disk_part_parse_gpt(geo, read, read_ctx, sector, table);
		if (ret == 0) {
			LOG_DBG("disk_part: GPT with %u slot(s)", table->entry_count);
			return 0;
		}
		if (ret != -ENOENT && ret != -EINVAL) {
			return ret;
		}
	}

	if (ret == -ENOENT) {
		ret = disk_part_parse_gpt(geo, read, read_ctx, sector, table);
		if (ret == 0) {
			LOG_DBG("disk_part: GPT with %u slot(s)", table->entry_count);
			return 0;
		}
	}

	disk_part_table_clear(table);
	return 0;
}

int disk_partition_walk(const struct disk_partition_geo *geo, disk_partition_read_sectors_t read,
			void *read_ctx, struct disk_partition_table *table)
{
	if (geo == NULL || table == NULL) {
		return -EINVAL;
	}

	if (geo->block_size <= 512U) {
		uint8_t sector[512];

		return disk_partition_walk_with_sector(geo, read, read_ctx, table, sector);
	}

	{
		uint8_t sector[DISK_PARTITION_MAX_BLOCK_SIZE];

		return disk_partition_walk_with_sector(geo, read, read_ctx, table, sector);
	}
}

static int disk_part_find_volume_fs_with_sector(const struct disk_partition_geo *geo,
						disk_partition_read_sectors_t read, void *read_ctx,
						uint64_t *lba_offset, uint64_t *sector_count,
						bool fat, uint8_t *sector)
{
	struct disk_partition_table table;
	uint64_t disk_sectors;
	int ret;

	disk_sectors = geo->sector_count;

	if (fat) {
		ret = read(read_ctx, 0U, sector, geo->block_size);
		if (ret != 0) {
			return ret;
		}

		if (disk_part_has_fat_bpb(sector, geo->block_size)) {
			*lba_offset = 0U;
			*sector_count = disk_part_clamp_volume_sectors(0U, 0U, disk_sectors);
			LOG_INF("disk_part: FAT volume at LBA 0 (superfloppy / raw BPB)");
			return 0;
		}
	}

	ret = disk_partition_walk_with_sector(geo, read, read_ctx, &table, sector);
	if (ret != 0) {
		return ret;
	}

	if (table.entry_count == 0U) {
		ret = disk_partition_set_whole_disk(geo, &table);
		if (ret != 0) {
			return ret;
		}
	}

	for (uint8_t i = 0U; i < table.entry_count; i++) {
		const struct disk_partition_entry *ent = &table.entries[i];
		bool match;

		if (fat) {
			ret = read(read_ctx, ent->start_lba, sector, geo->block_size);
			if (ret != 0) {
				return ret;
			}
			match = disk_part_has_fat_bpb(sector, geo->block_size);
		} else {
			match = disk_part_has_ext2_superblock(geo, read, read_ctx, ent->start_lba,
							      sector);
		}

		if (!match) {
			continue;
		}

		*lba_offset = ent->start_lba;
		*sector_count = disk_part_clamp_volume_sectors(ent->start_lba, ent->nr_sectors,
							       disk_sectors);
		return 0;
	}

	return -ENOENT;
}

static int disk_part_find_volume_fs(const struct disk_partition_geo *geo,
				    disk_partition_read_sectors_t read, void *read_ctx,
				    uint64_t *lba_offset, uint64_t *sector_count, bool fat)
{
	if (geo->block_size <= 512U) {
		uint8_t sector[512];

		return disk_part_find_volume_fs_with_sector(geo, read, read_ctx, lba_offset,
							    sector_count, fat, sector);
	}

	{
		uint8_t sector[DISK_PARTITION_MAX_BLOCK_SIZE];

		return disk_part_find_volume_fs_with_sector(geo, read, read_ctx, lba_offset,
							    sector_count, fat, sector);
	}
}

int disk_partition_find_fat_volume(const struct disk_partition_geo *geo,
				   disk_partition_read_sectors_t read, void *read_ctx,
				   uint64_t *lba_offset, uint64_t *sector_count)
{
	if (geo == NULL || read == NULL || lba_offset == NULL || sector_count == NULL) {
		return -EINVAL;
	}

	if (geo->block_size == 0U || geo->block_size > DISK_PARTITION_MAX_BLOCK_SIZE) {
		return -EINVAL;
	}

	return disk_part_find_volume_fs(geo, read, read_ctx, lba_offset, sector_count, true);
}

int disk_partition_find_ext2_volume(const struct disk_partition_geo *geo,
				    disk_partition_read_sectors_t read, void *read_ctx,
				    uint64_t *lba_offset, uint64_t *sector_count)
{
	if (geo == NULL || read == NULL || lba_offset == NULL || sector_count == NULL) {
		return -EINVAL;
	}

	if (geo->block_size == 0U || geo->block_size > DISK_PARTITION_MAX_BLOCK_SIZE) {
		return -EINVAL;
	}

	return disk_part_find_volume_fs(geo, read, read_ctx, lba_offset, sector_count, false);
}
