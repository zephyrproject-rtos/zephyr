/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI LUN exposed as a disk_access volume (block slice)
 *
 * Translates disk_access sector indices to SCSI LBAs using @a lba_offset.
 * Does not parse MBR/GPT; partition scan (@ref scsi_partition_walk) and
 * slice selection belong in initiator integration code before register.
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
#define SCSI_DISK_IOCTL_HANDLED 1 /*!< IOCTL handled; do not fall through */
#define SCSI_DISK_IOCTL_PASS    0 /*!< Fall through to generic SCSI disk IOCTL */

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
	/** disk_access registration and name */
	struct disk_info info;
	/** SCSI LUN backing this volume */
	struct scsi_device *sdev;
	/** First SCSI LBA exposed as disk_access sector 0 (partition offset) */
	uint64_t lba_offset;
	/**
	 * Visible sector count for disk_access. When zero, the span from
	 * @a lba_offset through the end of the LUN is exposed (@a lba_offset zero
	 * selects the full LUN). When non-zero, exactly that many sectors are
	 * visible. Invalid offset or size combinations fail @ref scsi_disk_register.
	 */
	uint64_t sector_count;
	/** Optional lock held around read/write */
	struct k_mutex *io_lock;
	/** Optional lazy init before first I/O */
	int (*init_fn)(struct scsi_disk *disk);
	/** Optional transport-specific IOCTL extension */
	scsi_disk_ioctl_hook_t ioctl_hook;
#if IS_ENABLED(CONFIG_SCSI_DISK_BOUNCE_BUF)
	/**
	 * Internal buffer for unaligned disk_access read/write (SD card_buffer
	 * analogue). Wider @kconfig:option:`CONFIG_SCSI_DISK_BUF_ALIGN` when the
	 * transport requires DMA-suitable alignment.
	 */
	uint8_t bounce_buf[CONFIG_SCSI_DISK_BOUNCE_SIZE] __aligned(CONFIG_SCSI_DISK_BUF_ALIGN);
#endif
	/** Set when registered with disk_access */
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
 * Call @c scsi_device_mark_removed() on detach before unregister.
 * Preset @a lba_offset and @a sector_count before registration to expose a
 * slice. @c lba_offset = 0 and @c sector_count = 0 register the full LUN.
 * @c sector_count = 0 with @c lba_offset &gt; 0 registers the remaining LUN
 * span. Explicit @c sector_count values must not extend past the backing LUN.
 * Registration returns @c -EINVAL when the slice is empty or out of range.
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
