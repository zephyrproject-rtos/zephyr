/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Transport-neutral MBR/GPT layout parser. Invoked only via the sector read
 * callback; no SCSI, USB, or filesystem dependencies.
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

static bool disk_part_mbr_type_is_extended(uint8_t type)
{
	return type == DISK_PARTITION_MBR_EXTENDED || type == DISK_PARTITION_MBR_EXTENDED_LBA ||
	       type == DISK_PARTITION_MBR_EXTENDED_W2K;
}

static bool disk_part_boot_signature_ok(const uint8_t *buf, uint32_t len)
{
	return len >= 512U && buf[510] == 0x55U && buf[511] == 0xAAU;
}

static bool disk_part_u64_add(uint64_t a, uint64_t b, uint64_t *sum)
{
	if (a > (UINT64_MAX - b)) {
		return false;
	}

	*sum = a + b;
	return true;
}

static void disk_part_table_clear(struct disk_partition_table *table)
{
	table->scheme = DISK_PARTITION_SCHEME_NONE;
	table->entry_count = 0U;
	(void)memset(table->entries, 0, sizeof(table->entries));
}

static bool disk_part_slice_within_geo(const struct disk_partition_geo *geo, uint64_t start_lba,
				       uint64_t nr_sectors)
{
	if (geo == NULL || nr_sectors == 0U) {
		return false;
	}

	if (geo->sector_count == 0U) {
		return true;
	}

	if (start_lba >= geo->sector_count) {
		return false;
	}

	return nr_sectors <= geo->sector_count - start_lba;
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

static int disk_part_parse_mbr_primaries(const struct disk_partition_geo *geo, const uint8_t *mbr,
					 struct disk_partition_table *table, bool *gpt_protective,
					 uint32_t *extended_base)
{
	bool protective = false;
	uint32_t ext_base = 0U;
	int ret;

	if (!disk_part_boot_signature_ok(mbr, geo->block_size)) {
		return -ENOENT;
	}

	for (unsigned int i = 0U; i < 4U; i++) {
		const uint8_t *pe = &mbr[0x1BE + (i * 16U)];
		const uint8_t type = pe[4];
		const uint32_t start = sys_get_le32(&pe[8]);
		const uint32_t sectors = sys_get_le32(&pe[12]);

		if (type == 0U || sectors == 0U) {
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

		if (!disk_part_slice_within_geo(geo, start, sectors)) {
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
		uint64_t ebr_lba;
		const uint8_t *pe0;
		const uint8_t *pe1;
		uint8_t type0;
		uint8_t type1;
		uint32_t start0;
		uint32_t sectors0;
		uint32_t link_rel;

		if (!disk_part_u64_add(extended_base, next_ebr, &ebr_lba)) {
			break;
		}

		if (geo->sector_count != 0U && ebr_lba >= geo->sector_count) {
			break;
		}

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

		if (type0 != 0U && !disk_part_mbr_type_is_extended(type0) && sectors0 != 0U) {
			uint64_t abs_start;

			if (disk_part_u64_add(ebr_lba, start0, &abs_start) &&
			    disk_part_slice_within_geo(geo, abs_start, sectors0)) {
				ret = disk_part_table_add(table, abs_start, sectors0, type0, NULL);
				if (ret != 0) {
					return ret;
				}
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
	uint64_t part_lba;
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

	{
		const uint32_t hdr_size = sys_get_le32(&sector[12]);

		if (hdr_size < 92U || hdr_size > geo->block_size) {
			return -EINVAL;
		}
	}

	part_lba = sys_get_le64(&sector[72]);
	part_count = sys_get_le32(&sector[80]);
	entry_size = sys_get_le32(&sector[84]);
	if (part_lba == 0U || part_count == 0U || entry_size < 128U || entry_size > 512U) {
		return -EINVAL;
	}

	if (part_count > (UINT32_MAX / entry_size)) {
		return -EINVAL;
	}

	{
		const uint64_t array_bytes = (uint64_t)part_count * entry_size;
		const uint64_t entry_secs64 =
			(array_bytes + geo->block_size - 1U) / geo->block_size;

		if (entry_secs64 > UINT32_MAX) {
			return -EINVAL;
		}

		entry_secs = (uint32_t)entry_secs64;

		if (geo->sector_count != 0U && (part_lba >= geo->sector_count ||
						entry_secs64 > geo->sector_count - part_lba)) {
			return -EINVAL;
		}
	}

	disk_part_table_clear(table);
	table->scheme = DISK_PARTITION_SCHEME_GPT;

	entry_secs = MIN(entry_secs, DISK_PART_GPT_ENTRY_SEC_MAX);

	for (sec = 0U; sec < entry_secs; sec++) {
		const uint32_t entries_per_sec = geo->block_size / entry_size;
		const uint32_t base_index = sec * entries_per_sec;
		uint64_t sec_lba;

		if (!disk_part_u64_add(part_lba, sec, &sec_lba)) {
			return -EINVAL;
		}

		if (geo->sector_count != 0U && sec_lba >= geo->sector_count) {
			return -EINVAL;
		}

		ret = read(read_ctx, sec_lba, sector, geo->block_size);
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

			if (geo->sector_count != 0U && last >= geo->sector_count) {
				continue;
			}

			nr_sectors = last - first + 1U;
			if (nr_sectors == 0U) {
				continue;
			}

			if (!disk_part_slice_within_geo(geo, first, nr_sectors)) {
				continue;
			}

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

	if (geo->block_size < 512U || geo->block_size > DISK_PARTITION_MAX_BLOCK_SIZE) {
		return -EINVAL;
	}

	disk_part_table_clear(table);

	ret = read(read_ctx, 0U, sector, geo->block_size);
	if (ret != 0) {
		return ret;
	}

	ret = disk_part_parse_mbr_primaries(geo, sector, table, &gpt_protective, &extended_base);
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
	uint8_t sector[DISK_PARTITION_MAX_BLOCK_SIZE];

	if (geo == NULL || table == NULL) {
		return -EINVAL;
	}

	return disk_partition_walk_with_sector(geo, read, read_ctx, table, sector);
}
