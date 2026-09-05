/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic SCSI disk volume (disk_access backend for SCSI LUNs)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_DISK_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_DISK_H_

#include <stdint.h>

#include <zephyr/drivers/disk.h>
#include <zephyr/kernel.h>
#include <zephyr/scsi/scsi.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Runtime context for a SCSI-backed disk_access volume */
struct scsi_disk;

/** IOCTL hook return: handled (stop), pass-through, or error */
#define SCSI_DISK_IOCTL_HANDLED 1
#define SCSI_DISK_IOCTL_PASS    0

/**
 * @brief Optional transport-specific IOCTL hook
 *
 * Return @c SCSI_DISK_IOCTL_HANDLED when @a out_result is final.
 * Return @c SCSI_DISK_IOCTL_PASS to fall through to generic SCSI disk IOCTL.
 * Return negative errno on failure.
 */
typedef int (*scsi_disk_ioctl_hook_t)(struct scsi_disk *disk, uint8_t cmd, void *buff,
				      int *out_result);

/** Runtime context for a SCSI-backed disk_access volume */
struct scsi_disk {
	struct disk_info info;
	struct scsi_device *sdev;
	/** Added to disk_access sector index for SCSI LBA (partition offset) */
	uint64_t lba_offset;
	/**
	 * Visible sector count for disk_access; when zero @a sdev block_count
	 * is used.
	 */
	uint64_t sector_count;
	/** Optional lock held around read/write */
	struct k_mutex *io_lock;
	/** Optional lazy init before first I/O */
	int (*init_fn)(struct scsi_disk *disk);
	/** Optional transport-specific IOCTL extension */
	scsi_disk_ioctl_hook_t ioctl_hook;
#if IS_ENABLED(CONFIG_SCSI_DISK_BOUNCE_BUF)
	uint8_t *bounce_buf;
#endif
	bool registered;
};

/**
 * @brief Attach a SCSI device without registering disk_access
 *
 * Used when the volume name is registered at boot and @a sdev becomes
 * available later in @a init_fn.
 */
void scsi_disk_attach(struct scsi_disk *disk, struct scsi_device *sdev);

/**
 * @brief Register a disk_access volume backed by a SCSI device
 *
 * @a sdev must remain valid until @ref scsi_disk_unregister is called.
 * Optional @a lba_offset and @a sector_count may be preset before registration
 * to expose a partition rather than the full LUN.
 */
int scsi_disk_register(struct scsi_disk *disk, struct scsi_device *sdev, const char *name);

/** @brief Unregister a SCSI disk_access volume */
void scsi_disk_unregister(struct scsi_disk *disk);

/** @brief Shared disk_access operations for SCSI-backed volumes */
extern const struct disk_operations scsi_disk_operations;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISK_SCSI_DISK_H_ */
