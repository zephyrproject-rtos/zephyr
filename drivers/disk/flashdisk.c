/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/disk.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flashdisk, CONFIG_FLASHDISK_LOG_LEVEL);

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) &&	\
	defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#define DISK_ERASE_RUNTIME_CHECK
#endif

struct flashdisk_data {
	struct disk_info info;
	struct k_mutex lock;
	const unsigned int area_id;
	const off_t offset;
	uint8_t *const cache;
	const size_t cache_size;
	const size_t size;
	const size_t sector_size;
	size_t page_size;
	off_t cached_addr;
	bool cache_valid;
	bool cache_dirty;
	bool erase_required;
};

#define GET_SIZE_TO_BOUNDARY(start, block_size) \
	(block_size - (start & (block_size - 1)))

/*
 * The default block size is used for devices not requiring erase.
 * It defaults to 512 as this is most widely used sector size
 * on storage devices.
 */
#define DEFAULT_BLOCK_SIZE	512

static inline bool flashdisk_with_erase(const struct flashdisk_data *ctx)
{
	ARG_UNUSED(ctx);
#if CONFIG_FLASH_HAS_EXPLICIT_ERASE
#if CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE
	return ctx->erase_required;
#else
	return true;
#endif
#endif
	return false;
}

static inline void flashdisk_probe_erase(struct flashdisk_data *ctx)
{
#if defined(DISK_ERASE_RUNTIME_CHECK)
	ctx->erase_required =
		flash_params_get_erase_cap(flash_get_parameters(ctx->info.dev)) &
			FLASH_ERASE_C_EXPLICIT;
#else
	ARG_UNUSED(ctx);
#endif
}

static int disk_flash_access_status(struct disk_info *disk)
{
	LOG_DBG("status : %s", disk->dev ? "okay" : "no media");
	if (!disk->dev) {
		return DISK_STATUS_NOMEDIA;
	}

	return DISK_STATUS_OK;
}

static int flashdisk_init_runtime(struct flashdisk_data *ctx,
				  const struct flash_area *fap)
{
	int rc;
	struct flash_pages_info page;
	off_t offset;

	flashdisk_probe_erase(ctx);

	if (IS_ENABLED(CONFIG_FLASHDISK_VERIFY_PAGE_LAYOUT) && flashdisk_with_erase(ctx)) {
		rc = flash_get_page_info_by_offs(ctx->info.dev, ctx->offset, &page);
		if (rc < 0) {
			LOG_ERR("Error %d while getting page info", rc);
			return rc;
		}

		ctx->page_size = page.size;
	} else {
		ctx->page_size = DEFAULT_BLOCK_SIZE;
	}

	LOG_INF("Initialize device %s", ctx->info.name);
	LOG_INF("offset %lx, sector size %zu, page size %zu, volume size %zu",
		(long)ctx->offset, ctx->sector_size, ctx->page_size, ctx->size);

	if (ctx->cache_size == 0) {
		/* Read-only flashdisk, no flash partition constraints */
		LOG_INF("%s is read-only", ctx->info.name);
		return 0;
	}

	if (IS_ENABLED(CONFIG_FLASHDISK_VERIFY_PAGE_LAYOUT) && flashdisk_with_erase(ctx)) {
		if (ctx->offset != page.start_offset) {
			LOG_ERR("Disk %s does not start at page boundary",
				ctx->info.name);
			return -EINVAL;
		}

		offset = ctx->offset + page.size;
		while (offset < ctx->offset + ctx->size) {
			rc = flash_get_page_info_by_offs(ctx->info.dev, offset, &page);
			if (rc < 0) {
				LOG_ERR("Error %d getting page info at offset %lx", rc, offset);
				return rc;
			}
			if (page.size != ctx->page_size) {
				LOG_ERR("Non-uniform page size is not supported");
				return rc;
			}
			offset += page.size;
		}

		if (offset != ctx->offset + ctx->size) {
			LOG_ERR("Last page crossess disk %s boundary",
				ctx->info.name);
			return -EINVAL;
		}
	}

	if (ctx->page_size > ctx->cache_size) {
		LOG_ERR("Cache too small (%zu needs %zu)",
			ctx->cache_size, ctx->page_size);
		return -ENOMEM;
	}

	return 0;
}

static int disk_flash_access_init(struct disk_info *disk)
{
	struct flashdisk_data *ctx;
	const struct flash_area *fap;
	int rc;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	rc = flash_area_open(ctx->area_id, &fap);
	if (rc < 0) {
		LOG_ERR("Flash area %u open error %d", ctx->area_id, rc);
		return rc;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	disk->dev = flash_area_get_device(fap);

	rc = flashdisk_init_runtime(ctx, fap);
	if (rc < 0) {
		flash_area_close(fap);
	}
	k_mutex_unlock(&ctx->lock);

	return rc;
}

static bool sectors_in_range(struct flashdisk_data *ctx,
			     uint32_t start_sector, uint32_t sector_count)
{
	uint32_t start, end;

	start = ctx->offset + (start_sector * ctx->sector_size);
	end = start + (sector_count * ctx->sector_size);

	if ((end >= start) && (start >= ctx->offset) && (end <= ctx->offset + ctx->size)) {
		return true;
	}

	LOG_ERR("sector start %" PRIu32 " count %" PRIu32
		" outside partition boundary", start_sector, sector_count);
	return false;
}

static int disk_flash_access_read(struct disk_info *disk, uint8_t *buff,
				uint32_t start_sector, uint32_t sector_count)
{
	struct flashdisk_data *ctx;
	off_t fl_addr;
	uint32_t remaining;
	uint32_t offset;
	uint32_t len;
	int rc = 0;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	if (!sectors_in_range(ctx, start_sector, sector_count)) {
		return -EINVAL;
	}

	fl_addr = ctx->offset + start_sector * ctx->sector_size;
	remaining = (sector_count * ctx->sector_size);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	/* Operate on page addresses to easily check for cached data */
	offset = fl_addr & (ctx->page_size - 1);
	fl_addr = ROUND_DOWN(fl_addr, ctx->page_size);

	/* Read up to page boundary on first iteration */
	len = ctx->page_size - offset;
	while (remaining) {
		if (remaining < len) {
			len = remaining;
		}

		if (ctx->cache_valid && ctx->cached_addr == fl_addr) {
			memcpy(buff, &ctx->cache[offset], len);
		} else if (flash_read(disk->dev, fl_addr + offset, buff, len) < 0) {
			rc = -EIO;
			goto end;
		}

		fl_addr += ctx->page_size;
		remaining -= len;
		buff += len;

		/* Try to read whole page on next iteration */
		len = ctx->page_size;
		offset = 0;
	}

end:
	k_mutex_unlock(&ctx->lock);

	return rc;
}

static int flashdisk_cache_commit(struct flashdisk_data *ctx)
{
	if (!ctx->cache_valid || !ctx->cache_dirty) {
		/* Either no cached data or cache matches flash data */
		return 0;
	}

	if (flashdisk_with_erase(ctx)) {
		if (flash_erase(ctx->info.dev, ctx->cached_addr, ctx->page_size) < 0) {
			return -EIO;
		}
	}

	/* write data to flash */
	if (flash_write(ctx->info.dev, ctx->cached_addr, ctx->cache, ctx->page_size) < 0) {
		return -EIO;
	}

	ctx->cache_dirty = false;
	return 0;
}

static int flashdisk_cache_load(struct flashdisk_data *ctx, off_t fl_addr)
{
	int rc;

	__ASSERT_NO_MSG((fl_addr & (ctx->page_size - 1)) == 0);

	if (ctx->cache_valid) {
		if (ctx->cached_addr == fl_addr) {
			/* Page is already cached */
			return 0;
		}
		/* Different page is in cache, commit it first */
		rc = flashdisk_cache_commit(ctx);
		if (rc < 0) {
			/* Failed to commit dirty page, abort */
			return rc;
		}
	}

	/* Load page into cache */
	ctx->cache_valid = false;
	ctx->cache_dirty = false;
	ctx->cached_addr = fl_addr;
	rc = flash_read(ctx->info.dev, fl_addr, ctx->cache, ctx->page_size);
	if (rc == 0) {
		/* Successfully loaded into cache, mark as valid */
		ctx->cache_valid = true;
		return 0;
	}

	return -EIO;
}

/* input size is either less or equal to a block size (ctx->page_size)
 * and write data never spans across adjacent blocks.
 */
static int flashdisk_cache_write(struct flashdisk_data *ctx, off_t start_addr,
				uint32_t size, const void *buff)
{
	int rc;
	off_t fl_addr;
	uint32_t offset;

	/* adjust offset if starting address is not erase-aligned address */
	offset = start_addr & (ctx->page_size - 1);

	/* always align starting address for flash cache operations */
	fl_addr = ROUND_DOWN(start_addr, ctx->page_size);

	/* when writing full page the address must be page aligned
	 * when writing partial page user data must be within a single page
	 */
	__ASSERT_NO_MSG(fl_addr + ctx->page_size >= start_addr + size);

	rc = flashdisk_cache_load(ctx, fl_addr);
	if (rc < 0) {
		return rc;
	}

	/* Do not mark cache as dirty if data to be written matches cache.
	 * If cache is already dirty, copy data to cache without compare.
	 */
	if (ctx->cache_dirty || memcmp(&ctx->cache[offset], buff, size)) {
		/* Update cache and mark it as dirty */
		memcpy(&ctx->cache[offset], buff, size);
		ctx->cache_dirty = true;
	}

	return 0;
}

static int disk_flash_access_write(struct disk_info *disk, const uint8_t *buff,
				 uint32_t start_sector, uint32_t sector_count)
{
	struct flashdisk_data *ctx;
	off_t fl_addr;
	uint32_t remaining;
	uint32_t size;
	int rc = 0;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	if (ctx->cache_size == 0) {
		return -ENOTSUP;
	}

	if (!sectors_in_range(ctx, start_sector, sector_count)) {
		return -EINVAL;
	}

	fl_addr = ctx->offset + start_sector * ctx->sector_size;
	remaining = (sector_count * ctx->sector_size);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	/* check if start address is erased-aligned address  */
	if (fl_addr & (ctx->page_size - 1)) {
		off_t block_bnd;

		/* not aligned */
		/* check if the size goes over flash block boundary */
		block_bnd = fl_addr + ctx->page_size;
		block_bnd = block_bnd & ~(ctx->page_size - 1);
		if ((fl_addr + remaining) <= block_bnd) {
			/* not over block boundary (a partial block also) */
			if (flashdisk_cache_write(ctx, fl_addr, remaining, buff) < 0) {
				rc = -EIO;
			}
			goto end;
		}

		/* write goes over block boundary */
		size = GET_SIZE_TO_BOUNDARY(fl_addr, ctx->page_size);

		/* write first partial block */
		if (flashdisk_cache_write(ctx, fl_addr, size, buff) < 0) {
			rc = -EIO;
			goto end;
		}

		fl_addr += size;
		remaining -= size;
		buff += size;
	}

	/* start is an erase-aligned address */
	while (remaining) {
		if (remaining < ctx->page_size) {
			break;
		}

		if (flashdisk_cache_write(ctx, fl_addr, ctx->page_size, buff) < 0) {
			rc = -EIO;
			goto end;
		}

		fl_addr += ctx->page_size;
		remaining -= ctx->page_size;
		buff += ctx->page_size;
	}

	/* remaining partial block */
	if (remaining) {
		if (flashdisk_cache_write(ctx, fl_addr, remaining, buff) < 0) {
			rc = -EIO;
			goto end;
		}
	}

end:
	k_mutex_unlock(&ctx->lock);

	return 0;
}

static int disk_flash_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	int rc;
	struct flashdisk_data *ctx;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	switch (cmd) {
	case DISK_IOCTL_CTRL_DEINIT:
	case DISK_IOCTL_CTRL_SYNC:
		k_mutex_lock(&ctx->lock, K_FOREVER);
		rc = flashdisk_cache_commit(ctx);
		k_mutex_unlock(&ctx->lock);
		return rc;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buff = ctx->size / ctx->sector_size;
		return 0;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buff = ctx->sector_size;
		return 0;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ: /* in sectors */
		k_mutex_lock(&ctx->lock, K_FOREVER);
		*(uint32_t *)buff = ctx->page_size / ctx->sector_size;
		k_mutex_unlock(&ctx->lock);
		return 0;
	case DISK_IOCTL_CTRL_INIT:
		return disk_flash_access_init(disk);
	default:
		break;
	}

	return -EINVAL;
}

static const struct disk_operations flash_disk_ops = {
	.init = disk_flash_access_init,
	.status = disk_flash_access_status,
	.read = disk_flash_access_read,
	.write = disk_flash_access_write,
	.ioctl = disk_flash_access_ioctl,
};

#define DT_DRV_COMPAT zephyr_flash_disk

#define PARTITION_PHANDLE(n) DT_PHANDLE_BY_IDX(DT_DRV_INST(n), partition, 0)
/* Force cache size to 0 if partition is read-only */
#define CACHE_SIZE(n) (DT_INST_PROP(n, cache_size) * !DT_PROP(PARTITION_PHANDLE(n), read_only))

#define DEFINE_FLASHDISKS_CACHE(n) \
	static uint8_t __aligned(4) flashdisk##n##_cache[CACHE_SIZE(n)];
DT_INST_FOREACH_STATUS_OKAY(DEFINE_FLASHDISKS_CACHE)

#define DEFINE_FLASHDISKS_DEVICE(n)						\
{										\
	.info = {								\
		.ops = &flash_disk_ops,						\
		.name = DT_INST_PROP(n, disk_name),				\
	},									\
	.area_id = DT_FIXED_PARTITION_ID(PARTITION_PHANDLE(n)),			\
	.offset = DT_REG_ADDR(PARTITION_PHANDLE(n)),				\
	.cache = flashdisk##n##_cache,						\
	.cache_size = sizeof(flashdisk##n##_cache),				\
	.size = DT_REG_SIZE(PARTITION_PHANDLE(n)),				\
	.sector_size = DT_INST_PROP(n, sector_size),				\
},

static struct flashdisk_data flash_disks[] = {
	DT_INST_FOREACH_STATUS_OKAY(DEFINE_FLASHDISKS_DEVICE)
};

#define VERIFY_CACHE_SIZE_IS_NOT_ZERO_IF_NOT_READ_ONLY(n)			\
	COND_CODE_1(DT_PROP(PARTITION_PHANDLE(n), read_only),			\
		(/* cache-size is not used for read-only disks */),		\
		(BUILD_ASSERT(DT_INST_PROP(n, cache_size) != 0,			\
		"Devicetree node " DT_NODE_PATH(DT_DRV_INST(n))			\
		" must have non-zero cache-size");))
DT_INST_FOREACH_STATUS_OKAY(VERIFY_CACHE_SIZE_IS_NOT_ZERO_IF_NOT_READ_ONLY)

#define VERIFY_CACHE_SIZE_IS_MULTIPLY_OF_SECTOR_SIZE(n)					\
	BUILD_ASSERT(DT_INST_PROP(n, cache_size) % DT_INST_PROP(n, sector_size) == 0,	\
		"Devicetree node " DT_NODE_PATH(DT_DRV_INST(n))				\
		" has cache size which is not a multiple of its sector size");
DT_INST_FOREACH_STATUS_OKAY(VERIFY_CACHE_SIZE_IS_MULTIPLY_OF_SECTOR_SIZE)

static int disk_flash_init(void)
{
	int err = 0;

	for (int i = 0; i < ARRAY_SIZE(flash_disks); i++) {
		int rc;

		k_mutex_init(&flash_disks[i].lock);

		rc = disk_access_register(&flash_disks[i].info);
		if (rc < 0) {
			LOG_ERR("Failed to register disk %s error %d",
				flash_disks[i].info.name, rc);
			err = rc;
		}
	}

	return err;
}

SYS_INIT(disk_flash_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
