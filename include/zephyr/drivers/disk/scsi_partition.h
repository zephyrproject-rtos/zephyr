/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI adapter for transport-neutral partition discovery
 *
 * Wraps @ref disk_partition_walk and filesystem volume helpers using
 * @ref scsi_io_read on the whole LUN before @ref scsi_disk_register.
 *
 * @since 4.5
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_PARTITION_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_PARTITION_H_

#include <stdint.h>

#include <zephyr/scsi/scsi.h>
#include <zephyr/storage/disk_partition.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse MBR/GPT partition tables on a SCSI LUN
 *
 * @param sdev Probed SCSI device (full LUN geometry)
 * @param table Output partition table
 *
 * @retval 0 Table parsed or no recognized layout
 * @retval -EINVAL Invalid argument
 * @retval Negative errno Read or parse failure
 */
int scsi_partition_walk(struct scsi_device *sdev, struct disk_partition_table *table);

/**
 * @brief Locate the first FAT/exFAT volume on a SCSI LUN
 *
 * @param sdev Probed SCSI device (full LUN geometry)
 * @param lba_offset Output: SCSI LBA of the volume boot sector
 * @param sector_count Output: visible sector count (0 = use remainder of LUN)
 *
 * @retval 0 Success
 * @retval -ENOENT No FAT/exFAT volume found
 * @retval Negative errno I/O or validation error
 */
int scsi_partition_find_fat_volume(struct scsi_device *sdev, uint64_t *lba_offset,
				   uint64_t *sector_count);

/**
 * @brief Locate the first ext2-compatible volume on a SCSI LUN
 *
 * @param sdev Probed SCSI device (full LUN geometry)
 * @param lba_offset Output: SCSI LBA of the volume superblock sector
 * @param sector_count Output: visible sector count (0 = use remainder of LUN)
 *
 * @retval 0 Success
 * @retval -ENOENT No ext2 volume found
 * @retval Negative errno I/O or validation error
 */
int scsi_partition_find_ext2_volume(struct scsi_device *sdev, uint64_t *lba_offset,
				    uint64_t *sector_count);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_PARTITION_H_ */
