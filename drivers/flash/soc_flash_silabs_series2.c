/*
 * Copyright (c) 2018, Piotr Mienkowski
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT     silabs_series2_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/dma.h>
#include <soc.h>
#include <em_msc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_silabs, CONFIG_FLASH_LOG_LEVEL);

struct flash_silabs_data {
	struct k_sem lock;
#if defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_WRITE) || defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_READ)
	const struct device *dma_dev;
	uint32_t dma_channel;
#endif
#if defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_READ)
	struct k_sem sync;
	int sync_status;
	struct dma_config dma_cfg;
	struct dma_block_config dma_block_cfg;
#endif
};

struct flash_silabs_config {
	struct flash_parameters flash_parameters;
	struct flash_pages_layout page_layout;
	bool lpwrite;
};

static bool flash_silabs_read_range_is_valid(off_t offset, uint32_t size)
{
	return (offset >= 0) && (offset < DT_REG_SIZE(SOC_NV_FLASH_NODE)) &&
	       ((DT_REG_SIZE(SOC_NV_FLASH_NODE) - offset) >= size);
}

static bool flash_silabs_write_range_is_valid(off_t offset, uint32_t size)
{
	/* Note:
	 * - A flash address to write to must be aligned to words.
	 * - Number of bytes to write must be divisible by 4.
	 */
	return flash_silabs_read_range_is_valid(offset, size) &&
	       IS_ALIGNED(offset, sizeof(uint32_t)) && IS_ALIGNED(size, 4);
}

static void flash_silabs_write_protection(bool enable)
{
	if (enable) {
		/* Lock the MSC module. */
		MSC->LOCK = 0;
	} else {
		/* Unlock the MSC module. */
		MSC->LOCK = MSC_LOCK_LOCKKEY_UNLOCK;
	}
}

#if defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_READ)
static void flash_silabs_read_callback(const struct device *dma_dev, void *user_data,
				       uint32_t channel, int status)
{
	const struct device *dev = user_data;
	struct flash_silabs_data *data = dev->data;

	data->sync_status = status;
	k_sem_give(&data->sync);
}
#endif

static int flash_silabs_read_dma(const struct device *dev, off_t offset, void *data, size_t size)
{
#if defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_READ)
	struct flash_silabs_data *const dev_data = dev->data;
	size_t data_size = 1;
	int err;

	if (IS_ALIGNED(offset, sizeof(uint32_t)) && IS_ALIGNED(data, sizeof(uint32_t)) &&
	    IS_ALIGNED(size, 4)) {
		data_size = 4;
	}

	k_sem_take(&dev_data->lock, K_FOREVER);

	dev_data->dma_cfg.source_data_size = data_size;
	dev_data->dma_cfg.dest_data_size = data_size;
	dev_data->dma_cfg.source_burst_length = data_size;
	dev_data->dma_cfg.dest_burst_length = data_size;

	dev_data->dma_block_cfg = (struct dma_block_config){
		.block_size = size,
		.source_address = DT_REG_ADDR(SOC_NV_FLASH_NODE) + offset,
		.dest_address = (uint32_t)data,
	};

	err = dma_config(dev_data->dma_dev, dev_data->dma_channel, &dev_data->dma_cfg);
	if (err < 0) {
		goto cleanup;
	}

	err = dma_start(dev_data->dma_dev, dev_data->dma_channel);
	if (err < 0) {
		goto cleanup;
	}

	k_sem_take(&dev_data->sync, K_FOREVER);
	if (dev_data->sync_status != DMA_STATUS_COMPLETE) {
		err = dev_data->sync_status;
	}

cleanup:
	k_sem_give(&dev_data->lock);
	return err;
#else
	return -ENOTSUP;
#endif
}

static int flash_silabs_read(const struct device *dev, off_t offset, void *data, size_t size)
{
	if (!flash_silabs_read_range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_SOC_FLASH_SILABS_S2_DMA_READ)) {
		return flash_silabs_read_dma(dev, offset, data, size);
	}

	memcpy(data, (uint8_t *)DT_REG_ADDR(SOC_NV_FLASH_NODE) + offset, size);

	return 0;
}

static int flash_silabs_write(const struct device *dev, off_t offset, const void *data, size_t size)
{
	struct flash_silabs_data *const dev_data = dev->data;
	void *address;
	int ret = 0;

	if (!flash_silabs_write_range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	k_sem_take(&dev_data->lock, K_FOREVER);
	flash_silabs_write_protection(false);

	address = (uint8_t *)DT_REG_ADDR(SOC_NV_FLASH_NODE) + offset;
#ifdef CONFIG_SOC_FLASH_SILABS_S2_DMA_WRITE
	/* If the DMA channel has previously been used for a different purpose,
	 * clear any lingering configuration that the MSC API doesn't tolerate.
	 */
	LDMA->IEN_CLR = BIT(dev_data->dma_channel);
	LDMA->CHDONE_CLR = BIT(dev_data->dma_channel);
	ret = MSC_WriteWordDma(dev_data->dma_channel, address, data, size);
#else
	ret = MSC_WriteWord(address, data, size);
#endif
	if (ret < 0) {
		ret = -EIO;
	}

	flash_silabs_write_protection(true);
	k_sem_give(&dev_data->lock);

	return ret;
}

static int flash_silabs_erase_block(off_t offset, size_t size)
{
	void *address;
	int ret = 0;

	for (off_t tmp = offset; tmp < offset + size;
	     tmp += DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)) {
		address = (uint8_t *)DT_REG_ADDR(SOC_NV_FLASH_NODE) + tmp;
		ret = MSC_ErasePage(address);
		if (ret < 0) {
			ret = -EIO;
			break;
		}
	}

	return ret;
}

static int flash_silabs_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_silabs_data *const dev_data = dev->data;
	int ret;

	if (!flash_silabs_read_range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!IS_ALIGNED(offset, DT_PROP(SOC_NV_FLASH_NODE, erase_block_size))) {
		LOG_ERR("offset 0x%lx: not on a page boundary", (long)offset);
		return -EINVAL;
	}

	if (!IS_ALIGNED(size, DT_PROP(SOC_NV_FLASH_NODE, erase_block_size))) {
		LOG_ERR("size %zu: not multiple of a page size", size);
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	k_sem_take(&dev_data->lock, K_FOREVER);
	flash_silabs_write_protection(false);

	ret = flash_silabs_erase_block(offset, size);

	flash_silabs_write_protection(true);
	k_sem_give(&dev_data->lock);

	return ret;
}

#if CONFIG_FLASH_PAGE_LAYOUT
void flash_silabs_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			      size_t *layout_size)
{
	const struct flash_silabs_config *const config = dev->config;

	*layout = &config->page_layout;
	*layout_size = 1;
}
#endif

static const struct flash_parameters *flash_silabs_get_parameters(const struct device *dev)
{
	const struct flash_silabs_config *const config = dev->config;

	return &config->flash_parameters;
}

static int flash_silabs_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = (uint64_t)DT_REG_SIZE(SOC_NV_FLASH_NODE);

	return 0;
}

static int flash_silabs_init(const struct device *dev)
{
	__maybe_unused struct flash_silabs_data *const dev_data = dev->data;
	const struct flash_silabs_config *config = dev->config;

	MSC_Init();

	if (config->lpwrite) {
		MSC->WRITECTRL_SET = MSC_WRITECTRL_LPWRITE;
	}

	/* Lock the MSC module. */
	flash_silabs_write_protection(true);

#if defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_WRITE) || defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_READ)
	dev_data->dma_channel = dma_request_channel(dev_data->dma_dev, NULL);
#endif

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static DEVICE_API(flash, flash_silabs_driver_api) = {
	.read = flash_silabs_read,
	.write = flash_silabs_write,
	.erase = flash_silabs_erase,
	.get_parameters = flash_silabs_get_parameters,
	.get_size = flash_silabs_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_silabs_page_layout,
#endif
};

/* clang-format off */
static struct flash_silabs_data flash_silabs_data_0 = {
	.lock = Z_SEM_INITIALIZER(flash_silabs_data_0.lock, 1, 1),
#if defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_WRITE) || defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_READ)
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR(0)),
	.dma_channel = -1,
#endif
#if defined(CONFIG_SOC_FLASH_SILABS_S2_DMA_READ)
	.sync = Z_SEM_INITIALIZER(flash_silabs_data_0.sync, 0, 1),
	.sync_status = -EIO,
	.dma_cfg = {
		.channel_direction = MEMORY_TO_MEMORY,
		.block_count = 1,
		.head_block = &flash_silabs_data_0.dma_block_cfg,
		.dma_callback = flash_silabs_read_callback,
		.user_data = (void *)DEVICE_DT_INST_GET(0),
	},
#endif
};

static const struct flash_silabs_config flash_silabs_config_0 = {
	.flash_parameters = {
		.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
		.erase_value = 0xff,
	},
	.page_layout = {
		.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
				DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
		.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	},
	.lpwrite = DT_INST_PROP(0, low_power_write),
};
/* clang-format on */

DEVICE_DT_INST_DEFINE(0, flash_silabs_init, NULL, &flash_silabs_data_0, &flash_silabs_config_0,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_silabs_driver_api);
