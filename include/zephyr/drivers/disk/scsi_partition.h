/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI adapter for @ref disk_partition_walk (layout scan only)
 *
 * Requires ``CONFIG_SCSI_DISK_PARTITION``.
 *
 * Maps a probed @ref scsi_device to @ref disk_partition_geo and implements
 * the sector read callback with @ref scsi_io_read (one logical block per
 * read). Does not register @ref disk_access_interface volumes or choose partitions;
 * initiator/USB-host integration selects a @ref disk_partition_entry and
 * configures @ref scsi_disk before @ref scsi_disk_register.
 *
 * @since 4.6
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_PARTITION_H_ */
