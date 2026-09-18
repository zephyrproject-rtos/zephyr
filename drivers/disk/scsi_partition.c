/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/disk/scsi_partition.h>
#include <zephyr/logging/log.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(scsi_partition, CONFIG_SCSI_DISK_LOG_LEVEL);

#define SCSI_PART_MAX_SECTOR_SIZE 4096U

static bool scsi_part_boot_signature_ok(const uint8_t *buf, uint32_t len)
{
	return len >= 512U && buf[510] == 0x55U && buf[511] == 0xAAU;
}

static bool scsi_part_has_fat_bpb(const uint8_t *buf, uint32_t len)
{
	if (len < 90U) {
		return false;
	}

	if (memcmp(&buf[3], "EXFAT   ", 8) == 0) {
		return scsi_part_boot_signature_ok(buf, len);
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

	return scsi_part_boot_signature_ok(buf, len);
}

static int scsi_part_mbr_fat_partition(const uint8_t *mbr, uint32_t *base_lba,
				       uint32_t *part_sectors)
{
	if (!scsi_part_boot_signature_ok(mbr, 512U)) {
		return -ENOENT;
	}

	for (unsigned int i = 0U; i < 4U; i++) {
		const uint8_t *pe = &mbr[0x1BE + (i * 16U)];
		const uint8_t type = pe[4];

		switch (type) {
		case 0x01:
		case 0x04:
		case 0x06:
		case 0x0B:
		case 0x0C:
		case 0x0E:
			*base_lba = sys_get_le32(&pe[8]);
			*part_sectors = sys_get_le32(&pe[12]);
			return 0;
		case 0xEE:
			return -EAGAIN;
		default:
			break;
		}
	}

	return -ENOENT;
}

static uint64_t scsi_part_clamp_volume_sectors(uint64_t volume_base, uint64_t part_sectors,
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

static int scsi_part_gpt_basic_data_lba(struct scsi_device *sdev, uint32_t block_size,
					uint64_t disk_last_lba, uint32_t *base_lba,
					uint32_t *part_sectors)
{
	static const uint8_t basic_data_guid[16] = {
		0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
		0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7,
	};
	uint8_t hdr[512];
	uint32_t part_lba;
	uint32_t part_count;
	uint32_t entry_size;
	int ret;

	if (disk_last_lba < 2U) {
		return -EINVAL;
	}

	ret = scsi_io_read(sdev, 1U, 1U, hdr, block_size);
	if (ret != 0) {
		return ret;
	}

	if (memcmp(hdr, "EFI PART", 8) != 0) {
		return -ENOENT;
	}

	part_lba = sys_get_le32(&hdr[72]);
	part_count = sys_get_le32(&hdr[80]);
	entry_size = sys_get_le32(&hdr[84]);
	if (part_lba == 0U || part_count == 0U || entry_size < 128U || entry_size > 512U) {
		return -EINVAL;
	}

	for (uint32_t sec = 0U; sec < 4U; sec++) {
		uint8_t part_sec[512];
		const uint32_t entries_per_sec = block_size / entry_size;

		ret = scsi_io_read(sdev, part_lba + sec, 1U, part_sec, block_size);
		if (ret != 0) {
			return ret;
		}

		for (uint32_t i = 0U;
		     i < entries_per_sec && (sec * entries_per_sec + i) < part_count; i++) {
			const uint8_t *ent = &part_sec[i * entry_size];
			const uint32_t first = sys_get_le32(&ent[32]);
			const uint32_t last = sys_get_le32(&ent[40]);

			if (memcmp(ent, basic_data_guid, 16) != 0) {
				continue;
			}

			*base_lba = first;
			*part_sectors = (last >= first) ? (last - first + 1U) : 0U;
			return 0;
		}
	}

	return -ENOENT;
}

int scsi_partition_discover_fat(struct scsi_device *sdev, uint64_t *lba_offset,
				uint64_t *sector_count)
{
	uint8_t disk_lba0[SCSI_PART_MAX_SECTOR_SIZE];
	uint8_t vol_boot[SCSI_PART_MAX_SECTOR_SIZE];
	uint32_t base = 0U;
	uint32_t parts = 0U;
	uint32_t block_size;
	uint64_t disk_sectors;
	uint64_t disk_last_lba;
	int ret;

	if (sdev == NULL || lba_offset == NULL || sector_count == NULL) {
		return -EINVAL;
	}

	if (sdev->block_size == 0U || sdev->block_size > SCSI_PART_MAX_SECTOR_SIZE) {
		return -EINVAL;
	}

	block_size = sdev->block_size;
	disk_sectors = sdev->block_count;
	disk_last_lba = (disk_sectors == 0U) ? 0U : (disk_sectors - 1U);

	ret = scsi_io_read(sdev, 0U, 1U, disk_lba0, block_size);
	if (ret != 0) {
		return ret;
	}

	if (scsi_part_has_fat_bpb(disk_lba0, block_size)) {
		*lba_offset = 0U;
		*sector_count = scsi_part_clamp_volume_sectors(0U, 0U, disk_sectors);
		LOG_INF("scsi_part: FAT volume at LBA 0 (superfloppy / raw BPB)");
		return 0;
	}

	ret = scsi_part_mbr_fat_partition(disk_lba0, &base, &parts);
	if (ret == 0) {
		ret = scsi_io_read(sdev, base, 1U, vol_boot, block_size);
		if (ret != 0) {
			return ret;
		}
		if (!scsi_part_has_fat_bpb(vol_boot, block_size)) {
			return -EINVAL;
		}
		*lba_offset = base;
		*sector_count = scsi_part_clamp_volume_sectors(base, parts, disk_sectors);
		LOG_INF("scsi_part: FAT volume at LBA %u (%llu sectors)", base,
			(unsigned long long)*sector_count);
		return 0;
	}

	if (ret == -EAGAIN) {
		ret = scsi_part_gpt_basic_data_lba(sdev, block_size, disk_last_lba, &base, &parts);
		if (ret != 0) {
			return ret;
		}
		ret = scsi_io_read(sdev, base, 1U, vol_boot, block_size);
		if (ret != 0) {
			return ret;
		}
		if (!scsi_part_has_fat_bpb(vol_boot, block_size)) {
			return -EINVAL;
		}
		*lba_offset = base;
		*sector_count = scsi_part_clamp_volume_sectors(base, parts, disk_sectors);
		LOG_INF("scsi_part: GPT FAT volume at LBA %u (%llu sectors)", base,
			(unsigned long long)*sector_count);
		return 0;
	}

	return -ENOENT;
}
