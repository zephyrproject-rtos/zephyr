/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Transport-neutral MBR/GPT FAT partition discovery for SCSI disks
 *
 * Partition tables on the whole LUN are parsed here to locate a FAT/exFAT
 * volume and expose it via @ref scsi_disk sector offset / count.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_PARTITION_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_PARTITION_H_

#include <stdint.h>

#include <zephyr/scsi/scsi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Locate a FAT/exFAT volume on a probed SCSI disk
 *
 * Reads LBA 0 and optional MBR/GPT structures via @a sdev. On success
 * @a lba_offset and @a sector_count describe the FAT volume within the LUN.
 *
 * @param sdev Probed SCSI device (full LUN geometry)
 * @param lba_offset Output: SCSI LBA of the FAT boot sector
 * @param sector_count Output: visible sector count for the volume (0 = use full LUN)
 *
 * @return 0 on success, negative errno if no FAT volume is found
 */
int scsi_partition_discover_fat(struct scsi_device *sdev, uint64_t *lba_offset,
				uint64_t *sector_count);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_PARTITION_H_ */
