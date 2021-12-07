/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_sdmmc

#include <kernel.h>
#include <string.h>
#include <devicetree.h>
#include <drivers/disk.h>
#include <device.h>
#include <soc.h>
#include "mmc_dw_ll.h"
#include "mmc_ll.h"

#define EMMC_DESC_SIZE		(1<<20)

struct dw_sdmmc_priv {
	int status;
	dw_mmc_params_t params;
	struct mmc_device_info info;
	uint32_t sector_count;
};

static int dw_sdmmc_access_init(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct dw_sdmmc_priv *priv = dev->data;

	if (priv->status == DISK_STATUS_OK) {
		return 0;
	}

	if (priv->status == DISK_STATUS_NOMEDIA) {
		return -ENODEV;
	}

	/* Porting from ATF */
	dw_mmc_init(&priv->params, &priv->info);

	priv->sector_count = priv->info.device_size / priv->info.block_size;

	priv->status = DISK_STATUS_OK;
	return 0;
}

static int dw_sdmmc_access_status(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct dw_sdmmc_priv *priv = dev->data;

	return priv->status;
}

static int dw_sdmmc_access_read(struct disk_info *disk, uint8_t *data_buf,
				   uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct dw_sdmmc_priv *priv = dev->data;

	size_t data_size;

	data_size = num_sector * priv->info.block_size;

	/* Return as size */
	mmc_read_blocks(start_sector, (uintptr_t)data_buf, data_size);

	return 0;

}

static int dw_sdmmc_access_write(struct disk_info *disk,
				    const uint8_t *data_buf,
				    uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct dw_sdmmc_priv *priv = dev->data;

	size_t data_size;

	data_size = num_sector * priv->info.block_size;

	/* Return as size */
	mmc_write_blocks(start_sector, (uintptr_t)data_buf, data_size);

	return 0;

}

static int dw_sdmmc_access_ioctl(struct disk_info *disk, uint8_t cmd,
				    void *buff)
{
	const struct device *dev = disk->dev;
	struct dw_sdmmc_priv *priv = dev->data;

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buff = priv->sector_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buff = priv->info.block_size;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buff = 1;
		break;
	case DISK_IOCTL_CTRL_SYNC:
	/* we use a blocking API, so nothing to do for sync */
		break;
	default:
		return -EINVAL;
	}
	return 0;

}

static const struct disk_operations dw_sdmmc_ops = {
	.init = dw_sdmmc_access_init,
	.status = dw_sdmmc_access_status,
	.read = dw_sdmmc_access_read,
	.write = dw_sdmmc_access_write,
	.ioctl = dw_sdmmc_access_ioctl,
};

static struct disk_info dw_sdmmc_info = {
	.name = CONFIG_SDMMC_VOLUME_NAME,
	.ops = &dw_sdmmc_ops,
};

static int disk_dw_sdmmc_init(const struct device *dev)
{

	struct dw_sdmmc_priv *priv = dev->data;
	int err;

	priv->status = DISK_STATUS_UNINIT;

	dw_sdmmc_info.dev = dev;
	err = disk_access_register(&dw_sdmmc_info);
	if (err) {
		return err;
	}

	return 0;
}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

static struct dw_idmac_desc dw_desc __aligned(512);

static struct dw_sdmmc_priv dw_sdmmc_priv_1 = {
	.params = {
		.bus_width = MMC_BUS_WIDTH_4,
		.clk_rate = DT_INST_PROP(0, clock_frequency),
		.desc_base = (uintptr_t) &dw_desc,
		.desc_size = EMMC_DESC_SIZE,
		.flags = 0,
		.reg_base = DT_INST_REG_ADDR(0),
	},
	.info = {
		.mmc_dev_type = MMC_IS_SD,
		.ocr_voltage = OCR_3_3_3_4 | OCR_3_2_3_3,
	},
};

DEVICE_DT_INST_DEFINE(0, disk_dw_sdmmc_init, NULL,
		    &dw_sdmmc_priv_1, NULL, POST_KERNEL,
		    CONFIG_SDMMC_INIT_PRIORITY,
		    NULL);
#endif
