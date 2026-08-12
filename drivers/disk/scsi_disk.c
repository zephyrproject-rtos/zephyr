/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/disk/scsi_disk.h>
#include <zephyr/drivers/disk/sg_io.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(scsi_disk, CONFIG_SCSI_DISK_LOG_LEVEL);

#define SCSI_DISK_MAX_XFER_BLOCKS 65535U

static uint64_t scsi_disk_visible_sectors(const struct scsi_disk *ctx)
{
	if (ctx->sector_count != 0U) {
		return ctx->sector_count;
	}

	if (ctx->sdev == NULL) {
		return 0U;
	}

	return ctx->sdev->block_count;
}

static int scsi_disk_lock_io(struct scsi_disk *ctx)
{
	if (ctx->io_lock == NULL) {
		return 0;
	}

	return k_mutex_lock(ctx->io_lock, K_FOREVER);
}

static void scsi_disk_unlock_io(struct scsi_disk *ctx)
{
	if (ctx->io_lock != NULL) {
		(void)k_mutex_unlock(ctx->io_lock);
	}
}

#if IS_ENABLED(CONFIG_SCSI_DISK_BOUNCE_BUF)
static int scsi_disk_ensure_bounce(struct scsi_disk *ctx)
{
	if (ctx->bounce_buf != NULL) {
		return 0;
	}

	ctx->bounce_buf = k_aligned_alloc(CONFIG_SCSI_DISK_BUF_ALIGN, CONFIG_SCSI_DISK_BOUNCE_SIZE);
	if (ctx->bounce_buf == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static void scsi_disk_free_bounce(struct scsi_disk *ctx)
{
	if (ctx->bounce_buf != NULL) {
		k_free(ctx->bounce_buf);
		ctx->bounce_buf = NULL;
	}
}
#endif

static int scsi_disk_xfer_blocks_aligned(struct scsi_disk *ctx, uint64_t lba, uint32_t blocks,
					 void *buf, bool write)
{
	const uint32_t block_size = ctx->sdev->block_size;
	uint32_t offset = 0U;
	int ret;

	while (blocks > 0U) {
		const uint32_t chunk = MIN(blocks, SCSI_DISK_MAX_XFER_BLOCKS);
		const uint32_t len = chunk * block_size;
		void *ptr = (uint8_t *)buf + offset;

		if (write) {
			ret = scsi_io_write(ctx->sdev, lba, chunk, ptr, len);
		} else {
			ret = scsi_io_read(ctx->sdev, lba, chunk, ptr, len);
		}

		if (ret != 0) {
			return ret;
		}

		lba += chunk;
		blocks -= chunk;
		offset += len;
	}

	return 0;
}

static int scsi_disk_xfer_blocks(struct scsi_disk *ctx, uint64_t lba, uint32_t blocks, void *buf,
				 bool write)
{
#if IS_ENABLED(CONFIG_SCSI_DISK_BOUNCE_BUF)
	const uint32_t block_size = ctx->sdev->block_size;
	const uintptr_t align_mask = (uintptr_t)CONFIG_SCSI_DISK_BUF_ALIGN - 1U;
	uint32_t chunk_blocks;
	int ret;

	if ((((uintptr_t)buf) & align_mask) == 0U) {
		return scsi_disk_xfer_blocks_aligned(ctx, lba, blocks, buf, write);
	}

	ret = scsi_disk_ensure_bounce(ctx);
	if (ret != 0) {
		return ret;
	}

	chunk_blocks = CONFIG_SCSI_DISK_BOUNCE_SIZE / block_size;
	if (chunk_blocks == 0U) {
		return -ENOBUFS;
	}

	while (blocks > 0U) {
		const uint32_t chunk = MIN(blocks, chunk_blocks);
		const uint32_t len = chunk * block_size;

		if (write) {
			(void)memcpy(ctx->bounce_buf, (uint8_t *)buf, len);
			ret = scsi_io_write(ctx->sdev, lba, chunk, ctx->bounce_buf, len);
		} else {
			ret = scsi_io_read(ctx->sdev, lba, chunk, ctx->bounce_buf, len);
			if (ret == 0) {
				(void)memcpy(buf, ctx->bounce_buf, len);
			}
		}

		if (ret != 0) {
			return ret;
		}

		lba += chunk;
		blocks -= chunk;
		buf = (uint8_t *)buf + len;
	}

	return 0;
#else
	return scsi_disk_xfer_blocks_aligned(ctx, lba, blocks, buf, write);
#endif
}

static int scsi_disk_access_status(struct disk_info *disk)
{
	struct scsi_disk *ctx = CONTAINER_OF(disk, struct scsi_disk, info);

	if (ctx->sdev == NULL || ctx->sdev->state == SCSI_DEV_REMOVED) {
		return DISK_STATUS_UNINIT;
	}

	if (ctx->sdev->state == SCSI_DEV_READY && ctx->sdev->block_size != 0U) {
		return DISK_STATUS_OK;
	}

	return DISK_STATUS_UNINIT;
}

static int scsi_disk_access_init(struct disk_info *disk)
{
	struct scsi_disk *ctx = CONTAINER_OF(disk, struct scsi_disk, info);

	if (ctx->init_fn != NULL) {
		return ctx->init_fn(ctx);
	}

	return 0;
}

static int scsi_disk_access_read(struct disk_info *disk, uint8_t *data_buf, uint32_t start_sector,
				 uint32_t num_sector)
{
	struct scsi_disk *ctx = CONTAINER_OF(disk, struct scsi_disk, info);
	int ret;

	if (ctx->sdev == NULL || ctx->sdev->block_size == 0U) {
		return -ENODEV;
	}

	if (start_sector + num_sector < start_sector) {
		return -EINVAL;
	}

	if ((uint64_t)start_sector + num_sector > scsi_disk_visible_sectors(ctx)) {
		return -EINVAL;
	}

	ret = scsi_disk_lock_io(ctx);
	if (ret != 0) {
		return -EBUSY;
	}

	ret = scsi_disk_xfer_blocks(ctx, ctx->lba_offset + start_sector, num_sector, data_buf,
				    false);
	scsi_disk_unlock_io(ctx);

	return ret;
}

static int scsi_disk_access_write(struct disk_info *disk, const uint8_t *data_buf,
				  uint32_t start_sector, uint32_t num_sector)
{
	struct scsi_disk *ctx = CONTAINER_OF(disk, struct scsi_disk, info);
	int ret;

	if (ctx->sdev == NULL || ctx->sdev->block_size == 0U) {
		return -ENODEV;
	}

	if (ctx->sdev->write_protected) {
		return -EROFS;
	}

	if (start_sector + num_sector < start_sector) {
		return -EINVAL;
	}

	if ((uint64_t)start_sector + num_sector > scsi_disk_visible_sectors(ctx)) {
		return -EINVAL;
	}

	ret = scsi_disk_lock_io(ctx);
	if (ret != 0) {
		return -EBUSY;
	}

	ret = scsi_disk_xfer_blocks(ctx, ctx->lba_offset + start_sector, num_sector,
				    (void *)(uintptr_t)data_buf, true);
	scsi_disk_unlock_io(ctx);

	return ret;
}

static int scsi_disk_handle_sg_io(struct scsi_disk *ctx, void *buff)
{
	struct sg_io_req *req = buff;
	struct scsi_sg_io io;

	if (req == NULL || req->protocol != BSG_PROTOCOL_SCSI ||
	    req->subprotocol != BSG_SUB_PROTOCOL_SCSI_CMD) {
		return -EINVAL;
	}

	io.cdb = req->request;
	io.cdb_len = (uint8_t)req->request_len;
	io.dxfer_dir = req->dxfer_dir;
	io.dxferp = req->dxferp;
	io.dxfer_len = req->dxfer_len;
	io.sense = req->response;
	io.sense_len = req->max_response_len;

	return scsi_sg_io(ctx->sdev, &io);
}

static int scsi_disk_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct scsi_disk *ctx = CONTAINER_OF(disk, struct scsi_disk, info);
	int hook_result;
	int hook_ret;

	if (ctx->ioctl_hook != NULL) {
		hook_ret = ctx->ioctl_hook(ctx, cmd, buff, &hook_result);
		if (hook_ret == SCSI_DISK_IOCTL_HANDLED) {
			return hook_result;
		}
		if (hook_ret < 0) {
			return hook_ret;
		}
	}

	switch (cmd) {
	case DISK_IOCTL_CTRL_INIT:
		return scsi_disk_access_init(disk);
	case DISK_IOCTL_CTRL_DEINIT:
#if IS_ENABLED(CONFIG_SCSI_DISK_BOUNCE_BUF)
		scsi_disk_free_bounce(ctx);
#endif
		return 0;
	case DISK_IOCTL_CTRL_SYNC:
		if (ctx->sdev == NULL) {
			return -ENODEV;
		}
		return scsi_synchronize_cache_10(ctx->sdev);
	case DISK_IOCTL_GET_SECTOR_COUNT: {
		const uint64_t sectors = scsi_disk_visible_sectors(ctx);

		if (ctx->sdev == NULL || sectors == 0U || sectors > UINT32_MAX || buff == NULL) {
			return -ENODEV;
		}
		*(uint32_t *)buff = (uint32_t)sectors;
		return 0;
	}
	case DISK_IOCTL_GET_SECTOR_SIZE:
		if (ctx->sdev == NULL || ctx->sdev->block_size == 0U || buff == NULL) {
			return -ENODEV;
		}
		*(uint32_t *)buff = ctx->sdev->block_size;
		return 0;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		if (buff == NULL) {
			return -EINVAL;
		}
		*(uint32_t *)buff = 1U;
		return 0;
	case SG_IO:
		if (ctx->sdev == NULL) {
			return -ENODEV;
		}
		return scsi_disk_handle_sg_io(ctx, buff);
	default:
		return -ENOTSUP;
	}
}

const struct disk_operations scsi_disk_operations = {
	.init = scsi_disk_access_init,
	.status = scsi_disk_access_status,
	.read = scsi_disk_access_read,
	.write = scsi_disk_access_write,
	.ioctl = scsi_disk_access_ioctl,
};

void scsi_disk_attach(struct scsi_disk *disk, struct scsi_device *sdev)
{
	if (disk == NULL) {
		return;
	}

	disk->sdev = sdev;
}

int scsi_disk_register(struct scsi_disk *disk, struct scsi_device *sdev, const char *name)
{
	int ret;

	if (disk == NULL || sdev == NULL || name == NULL) {
		return -EINVAL;
	}

	if (disk->registered) {
		return -EALREADY;
	}

	scsi_disk_attach(disk, sdev);
	disk->info.name = name;
	disk->info.ops = &scsi_disk_operations;

	ret = disk_access_register(&disk->info);
	if (ret != 0) {
		disk->sdev = NULL;
		disk->info.name = NULL;
		disk->info.ops = NULL;
		return ret;
	}

	disk->registered = true;
	return 0;
}

void scsi_disk_unregister(struct scsi_disk *disk)
{
	if (disk == NULL || !disk->registered) {
		return;
	}

#if IS_ENABLED(CONFIG_SCSI_DISK_BOUNCE_BUF)
	scsi_disk_free_bounce(disk);
#endif

	(void)disk_access_unregister(&disk->info);
	disk->registered = false;
	disk->sdev = NULL;
	disk->lba_offset = 0U;
	disk->sector_count = 0U;
	disk->io_lock = NULL;
	disk->init_fn = NULL;
	disk->ioctl_hook = NULL;
	disk->info.name = NULL;
	disk->info.ops = NULL;
}
