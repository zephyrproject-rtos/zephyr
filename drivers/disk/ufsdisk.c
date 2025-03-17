/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * UFS disk driver using zephyr UFS-SCSI subsystem
 * - Uses scsi read/write APIs for disk read/write operations.
 * - Uses IOCTL to issue UFS Query requests.
 * - LUN number from device tree should be enabled in UFS device.
 * - LUN-0 is used, when LUN is not specified in device tree.
 * - For unaligned memory read/writes, a local buffer is used.
 */

#define DT_DRV_COMPAT zephyr_ufs_disk

#include <string.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/logging/log.h>
#include <zephyr/ufs/ufs.h>
#include <zephyr/ufs/ufs_ops.h>

LOG_MODULE_REGISTER(ufsdisk, CONFIG_UFSDISK_LOG_LEVEL);

/**
 * @brief UFS disk status.
 */
enum ufsdisk_status {
	UFS_UNINIT, /**< Un-Initialized UFS Disk. */
	UFS_ERROR,  /**< UFS Disk Initialization Failed. */
	UFS_OK,     /**< UFS Disk Operational. */
};

/**
 * @brief Runtime data structure for a UFS disk.
 */
struct ufsdisk_data {
	struct disk_info info;             /**< Disk information structure. */
	const struct device *ufshc_device; /**< UFS Host Controller device. */
	uint8_t lun;                       /**< Logical Unit Number (LUN) for the disk. */
	struct ufs_host_controller *ufshc; /**< UFS Host Controller instance. */
	struct scsi_device *sdev;          /**< SCSI device associated with the UFS disk. */
	uint8_t *card_buffer;              /**< Temporary buffer for unaligned I/O operations. */
	enum ufsdisk_status status;        /**< Current status of the UFS disk. */
};

/**
 * @brief Initializes the UFS disk access.
 *
 * This function registers the host controller and verify the device is ready.
 * It also allocates a buffer for unaligned reads/writes.
 *
 * @param disk Pointer to the disk_info structure representing the UFS disk.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int32_t disk_ufs_access_init(struct disk_info *disk)
{
	struct ufsdisk_data *data = (struct ufsdisk_data *)disk;
	int32_t err;

	if (data->status == UFS_OK) {
		/* called twice, don't reinit */
		err = 0;
		goto out;
	}

	/* set status as error */
	data->status = UFS_ERROR;

	/* Register host controller with UFS card */
	if ((data->ufshc != NULL) && (data->ufshc->is_initialized)) {
		err = 0; /* already initialized */
	} else {
		err = ufs_init(data->ufshc_device, &data->ufshc);
		if (err != 0) {
			LOG_ERR("UFS initialization failed %d", err);
			goto out;
		}
	}

	/* Look up the SCSI device for the specified LUN */
	data->sdev = scsi_device_lookup_by_host(data->ufshc->host, data->lun);
	if (data->sdev == NULL) {
		LOG_ERR("SCSI device for lun:%d is NULL", data->lun);
		err = -ENOTSUP;
		goto out;
	}

	/* Verify device readiness */
	err = scsi_ioctl(data->sdev, SCSI_IOCTL_TEST_UNIT_READY, NULL);
	if (err != 0) {
		LOG_ERR("Failed to execute TUR, lun:%d", data->lun);
		goto out;
	}

	/* Allocate a temporary buffer for unaligned reads/writes */
	data->card_buffer = (uint8_t *)k_aligned_alloc(CONFIG_UFSHC_BUFFER_ALIGNMENT,
						       CONFIG_UFS_BUFFER_SIZE);
	if (data->card_buffer == NULL) {
		err = -ENOMEM;
		goto out;
	}

	/* Initialization complete */
	data->status = UFS_OK;

out:
	return err;
}

/**
 * @brief Returns the current status of the UFS disk.
 *
 * @param disk Pointer to the disk_info structure representing the UFS disk.
 *
 * @return Disk status
 */
static int32_t disk_ufs_access_status(struct disk_info *disk)
{
	struct ufsdisk_data *data = (struct ufsdisk_data *)disk;
	int32_t err;

	if (data->status == UFS_OK) {
		err = DISK_STATUS_OK;
	} else {
		err = DISK_STATUS_UNINIT;
	}

	return err;
}

/**
 * @brief Reads data from UFS card using SCSI read API.
 *
 * @param disk Pointer to the disk_info structure representing the UFS disk.
 * @param rbuf Pointer to the buffer where the read data will be stored.
 * @param start_block The starting block number to read from.
 * @param num_blocks The number of blocks to read.
 *
 * @return 0 on success, a negative error code on failure.
 */
static int32_t ufs_card_read_blocks(struct disk_info *disk,
				    uint8_t *rbuf,
				    uint32_t start_block,
				    uint32_t num_blocks)
{
	int32_t ret;
	uint32_t rlen;
	uint32_t sector = 0;
	uint32_t buf_offset = 0;

	/* Get the UFS device and host controller from the SCSI device */
	struct ufsdisk_data *disk_data = (struct ufsdisk_data *)disk;
	struct scsi_device *scsi_dev = disk_data->sdev;
	const struct device *ufs_dev = scsi_dev->host->parent;
	struct ufs_host_controller *ufshc = (struct ufs_host_controller *)ufs_dev->data;

	/* Validate if requested blocks are within the disk's capacity */
	if ((start_block + num_blocks) > (scsi_dev->capacity)) {
		return -EINVAL;
	}

	/* Lock UFS host controller for access */
	ret = k_mutex_lock(&ufshc->ufs_lock, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("could not get UFS card mutex");
		return -EBUSY;
	}

	/* Handle aligned and unaligned buffer reads */
	if ((((uintptr_t)rbuf) & ((uintptr_t)CONFIG_UFSHC_BUFFER_ALIGNMENT - (uintptr_t)1))
		!= (uintptr_t)0) {
		/* Unaligned buffer, use temporary internal buffer */
		rlen = (uint32_t)((uint32_t)CONFIG_UFS_BUFFER_SIZE / (scsi_dev->sector_size));
		if (rlen == 0U) {
			LOG_ERR("Card buffer size is less than block size - unaligned");
			ret = -ENOBUFS;
			goto out;
		}
		rlen = MIN(num_blocks, rlen);
		if (disk_data->card_buffer == NULL) {
			ret = -ENOMEM;
			goto out;
		}
		(void)memset(disk_data->card_buffer, 0x0,
			     (size_t)(uint32_t)(rlen * (scsi_dev->sector_size)));
		while (sector < num_blocks) {
			/* Read from UFS device to internal buffer */
			ret = scsi_read(scsi_dev,
					(uint64_t)(uint32_t)(sector + start_block),
					rlen,
					disk_data->card_buffer);
			if (ret != 0) {
				LOG_ERR("UFS Card read failed");
				goto out;
			}
			/* Copy data from internal buffer to user buffer */
			(void)memcpy(&rbuf[buf_offset],
				     disk_data->card_buffer,
				     (size_t)(uint32_t)(rlen * (scsi_dev->sector_size)));
			sector += rlen;
			buf_offset += rlen * (scsi_dev->sector_size);
		}

	} else {
		/* Aligned buffer, use directly */
		ret = scsi_read(scsi_dev, start_block, num_blocks, rbuf);
		if (ret != 0) {
			LOG_ERR("UFS Card read failed");
			goto out;
		}
	}

out:
	(void)k_mutex_unlock(&ufshc->ufs_lock);

	return ret;
}

/**
 * @brief Reads data from the UFS disk.
 *
 * @param disk Pointer to the disk_info structure representing the UFS disk.
 * @param buf Pointer to the buffer where the read data will be stored.
 * @param sector The starting sector number to read from.
 * @param count The number of sectors to read.
 *
 * @return 0 on success, a negative error code on failure.
 */
static int32_t disk_ufs_access_read(struct disk_info *disk,
				    uint8_t *buf,
				    uint32_t sector,
				    uint32_t count)
{
	uint32_t last_sector = sector + count;

	if (last_sector < sector) {
		return -EIO; /* Overflow check */
	}

	return ufs_card_read_blocks(disk, buf, sector, count);
}

/**
 * @brief Writes data to UFS card using SCSI write API.
 *
 * @param disk Pointer to the disk_info structure representing the UFS disk.
 * @param wbuf Pointer to the buffer containing the data to be written.
 * @param start_block The starting block number to write to.
 * @param num_blocks The number of blocks to write.
 *
 * @return zero if successful or an error if Write failed.
 */
static int32_t ufs_card_write_blocks(struct disk_info *disk,
				     const uint8_t *wbuf,
				     uint32_t start_block,
				     uint32_t num_blocks)
{
	int32_t ret;
	uint32_t wlen;
	uint32_t sector;
	const uint8_t *buf_offset;

	/* Get the UFS device and host controller from the SCSI device */
	struct ufsdisk_data *disk_data = (struct ufsdisk_data *)disk;
	struct scsi_device *scsi_dev = disk_data->sdev;
	const struct device *ufs_dev = scsi_dev->host->parent;
	struct ufs_host_controller *ufshc = (struct ufs_host_controller *)ufs_dev->data;

	/* Validate if requested blocks are within the disk's capacity */
	if ((start_block + num_blocks) > (scsi_dev->capacity)) {
		return -EINVAL;
	}

	/* Lock UFS host controller for access */
	ret = k_mutex_lock(&ufshc->ufs_lock, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("could not get UFS card mutex");
		return -EBUSY;
	}

	/* Handle aligned and unaligned buffer writes */
	if ((((uintptr_t)wbuf) & ((uintptr_t)CONFIG_UFSHC_BUFFER_ALIGNMENT - (uintptr_t)1))
		!= (uintptr_t)0) {
		/* Unaligned buffer, use temporary internal buffer */
		wlen = (uint32_t)((uint32_t)CONFIG_UFS_BUFFER_SIZE / (scsi_dev->sector_size));
		if (wlen == 0U) {
			LOG_ERR("Card buffer size is less than block size - unaligned");
			ret = -ENOBUFS;
			goto out;
		}
		wlen = MIN(num_blocks, wlen);
		sector = 0;
		buf_offset = wbuf;
		if (disk_data->card_buffer == NULL) {
			ret = -ENOMEM;
			goto out;
		}
		(void)memset(disk_data->card_buffer,
			     0x0,
			     (size_t)(uint32_t)(wlen * (scsi_dev->sector_size)));
		while (sector < num_blocks) {
			/* Copy data from user buffer to internal buffer */
			(void)memcpy(disk_data->card_buffer, buf_offset,
				   (size_t)(uint32_t)(wlen * (scsi_dev->sector_size)));
			/* Write from internal buffer to UFS device */
			ret = scsi_write(scsi_dev,
					(uint64_t)(uint32_t)(sector + start_block),
					 wlen,
					 disk_data->card_buffer);
			if (ret != 0) {
				LOG_ERR("UFS Card write failed");
				goto out;
			}
			/* Increase sector count and buffer offset */
			sector += wlen;
			buf_offset += wlen * (scsi_dev->sector_size);
		}
	} else {
		/* Aligned buffer, use directly */
		ret = scsi_write(scsi_dev, start_block, num_blocks, wbuf);
		if (ret != 0) {
			LOG_ERR("UFS Card write failed");
			goto out;
		}
	}

out:
	(void)k_mutex_unlock(&ufshc->ufs_lock);

	return ret;
}

/**
 * @brief Write data to UFS disk.
 *
 * @param disk Pointer to the disk information structure.
 * @param buf Pointer to the buffer containing data to be written.
 * @param sector The starting sector to write to.
 * @param count The number of sectors to write.
 *
 * @return 0 on success, negative error code on failure.
 */
static int32_t disk_ufs_access_write(struct disk_info *disk,
				     const uint8_t *buf,
				     uint32_t sector,
				     uint32_t count)
{
	uint32_t last_sector = sector + count;

	if (last_sector < sector) {
		return -EINVAL; /* Overflow check */
	}

	return ufs_card_write_blocks(disk, buf, sector, count);
}

/**
 * @brief Handle IOCTL commands for UFS disk.
 *
 * This function handles various IOCTL commands such as initialization,
 * de-initialization, sector size retrieval, or executing SCSI or UFS requests.
 *
 * The behavior of the function depends on the value of the @cmd parameter:
 *
 * - DISK_IOCTL_CTRL_INIT: Initialize the UFS disk.
 * - DISK_IOCTL_CTRL_DEINIT: Deinitialize the UFS disk.
 * - DISK_IOCTL_CTRL_SYNC: No operation (typically used for synchronization).
 * - DISK_IOCTL_GET_SECTOR_COUNT: Return the total number of sectors in the disk.
 * - DISK_IOCTL_GET_SECTOR_SIZE: Return the sector size of the disk.
 * - DISK_IOCTL_GET_ERASE_BLOCK_SZ: Return the erase block size of the disk (in sectors).
 * - SG_IO: Handle SCSI or UFS-specific IO requests via the sg_io_req structure
 *   pointed to by buff. This includes processing SCSI commands or UFS-specific
 *   commands using the appropriate protocol and subprotocol.
 *
 * @param disk Pointer to the disk_info structure representing the UFS disk.
 * @param cmd The IOCTL command to process.
 * @param buff Pointer to the buffer for input/output data (depends on command).
 *
 * @return 0 on success, negative error code on failure.
 */
static int32_t disk_ufs_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct ufsdisk_data *data = (struct ufsdisk_data *)disk;
	struct scsi_device *sdev = data->sdev;
	struct ufs_host_controller *ufshc = data->ufshc;
	struct sg_io_req *req;
	int32_t ret = 0;

	switch (cmd) {
	case DISK_IOCTL_CTRL_INIT:
		/* Initialize the disk */
		ret = disk_ufs_access_init(disk);
		break;
	case DISK_IOCTL_CTRL_DEINIT:
		/* De-initialize the disk */
		/* mark the disk as uninitialized and free memory */
		data->status = UFS_UNINIT;
		k_free(data->card_buffer);
		data->card_buffer = NULL;
		break;
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		/* Retrieve the total number of sectors on the disk */
		if (buff != NULL) {
			*(uint32_t *)buff = (uint32_t)(sdev->capacity);
		} else {
			ret = -EINVAL;
		}
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		/* Retrieve the size of each sector on the disk */
		if (buff != NULL) {
			*(uint32_t *)buff = (sdev->sector_size);
		} else {
			ret = -EINVAL;
		}
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		/* Retrieve the block size for erase */
		if (buff != NULL) {
			*(uint32_t *)buff = 1U;
		} else {
			ret = -EINVAL;
		}
		break;
	case SG_IO:
		/* Handle SCSI or UFS-specific I/O requests */
		req = (struct sg_io_req *)buff;

		if ((req == NULL) ||
		   (req->protocol != BSG_PROTOCOL_SCSI)) {
			ret = -EINVAL;
		} else {
			/*Process SCSI or UFS depending on the subprotocol */
			if (req->subprotocol == BSG_SUB_PROTOCOL_SCSI_CMD) {
				/* Handle SCSI-specific I/O operation */
				ret = scsi_ioctl(sdev, (int32_t)cmd, req);
			} else {
				/* Handle UFS-specific I/O operation */
				ret = ufs_sg_request(ufshc, req);
			}
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * @brief Disk operations for UFS disk.
 *
 * This structure contains the disk operations for UFS disk management,
 * including initialization, status checking, read, write, and ioctl handling.
 */
static const struct disk_operations ufs_disk_ops = {
	.init = disk_ufs_access_init,
	.status = disk_ufs_access_status,
	.read = disk_ufs_access_read,
	.write = disk_ufs_access_write,
	.ioctl = disk_ufs_access_ioctl,
};

#define DEFINE_UFSDISKS_DEVICE(n)					\
	{								\
		.info = {						\
				.ops = &ufs_disk_ops,			\
				.name = DT_INST_PROP(n, disk_name),	\
			},						\
		.ufshc_device = DEVICE_DT_GET(DT_INST_PARENT(n)),	\
		.lun = DT_INST_PROP(n, lun),				\
	},

/**
 * @brief Array of UFS disk data structures.
 *
 * This array is populated using the device tree macros, which define the
 * UFS disks and their properties such as the disk name and LUN.
 */
static struct ufsdisk_data ufs_disks[] = {
	DT_INST_FOREACH_STATUS_OKAY(DEFINE_UFSDISKS_DEVICE)
};

/**
 * @brief Register UFS disks.
 *
 * This function registers the UFS disk devices during the
 * system initialization phase.
 *
 * @return 0 on success, negative error code on failure.
 */
static int32_t disk_ufs_register(void)
{
	int32_t err = 0, rc;
	int32_t index, num_of_disks;

	num_of_disks = (int32_t)ARRAY_SIZE(ufs_disks);
	for (index = 0; index < num_of_disks; index++) {
		rc = disk_access_register(&ufs_disks[index].info);
		if (rc < 0) {
			LOG_ERR("Failed to register disk %s error %d",
				ufs_disks[index].info.name, rc);
			err = rc;
		}
	}

	return err;
}

SYS_INIT(disk_ufs_register, POST_KERNEL, CONFIG_UFSDISK_INIT_PRIORITY);
