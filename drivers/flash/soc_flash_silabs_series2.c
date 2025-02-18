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
#ifdef CONFIG_SOC_FLASH_SILABS_S2_DMA_WRITE
	const struct device *dma_dev;
	uint32_t dma_channel;
#endif
};

struct flash_silabs_config {
	struct flash_parameters flash_parameters;
	struct flash_pages_layout page_layout;
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

static int flash_silabs_read(const struct device *dev, off_t offset, void *data, size_t size)
{
	if (!flash_silabs_read_range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
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
	struct flash_silabs_data *const dev_data = dev->data;

	MSC_Init();

	/* Lock the MSC module. */
	flash_silabs_write_protection(true);

#ifdef CONFIG_SOC_FLASH_SILABS_S2_DMA_WRITE
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
#ifdef CONFIG_SOC_FLASH_SILABS_S2_DMA_WRITE
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR(0)),
	.dma_channel = -1,
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
};
/* clang-format on */

DEVICE_DT_INST_DEFINE(0, flash_silabs_init, NULL, &flash_silabs_data_0, &flash_silabs_config_0,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_silabs_driver_api);
