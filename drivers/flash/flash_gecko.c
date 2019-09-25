/*
 * Copyright (c) 2018, Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <em_msc.h>
#include <drivers/flash.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(flash_gecko);

struct flash_gecko_data {
	struct k_sem mutex;
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_DATA(dev) \
	((struct flash_gecko_data *const)(dev)->driver_data)

static bool write_range_is_valid(off_t offset, u32_t size);
static bool read_range_is_valid(off_t offset, u32_t size);
static int erase_flash_block(off_t offset, size_t size);

static int flash_gecko_read(struct device *dev, off_t offset, void *data,
			    size_t size)
{
	if (!read_range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	memcpy(data, (u8_t *)CONFIG_FLASH_BASE_ADDRESS + offset, size);

	return 0;
}

static int flash_gecko_write(struct device *dev, off_t offset,
			     const void *data, size_t size)
{
	struct flash_gecko_data *const dev_data = DEV_DATA(dev);
	MSC_Status_TypeDef msc_ret;
	void *address;
	int ret = 0;

	if (!write_range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	k_sem_take(&dev_data->mutex, K_FOREVER);

	address = (u8_t *)CONFIG_FLASH_BASE_ADDRESS + offset;
	msc_ret = MSC_WriteWord(address, data, size);
	if (msc_ret < 0) {
		ret = -EIO;
	}

	k_sem_give(&dev_data->mutex);

	return ret;
}

static int flash_gecko_erase(struct device *dev, off_t offset, size_t size)
{
	struct flash_gecko_data *const dev_data = DEV_DATA(dev);
	int ret;

	if (!read_range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if ((offset % FLASH_PAGE_SIZE) != 0) {
		LOG_ERR("offset 0x%lx: not on a page boundary", (long)offset);
		return -EINVAL;
	}

	if ((size % FLASH_PAGE_SIZE) != 0) {
		LOG_ERR("size %zu: not multiple of a page size", size);
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	k_sem_take(&dev_data->mutex, K_FOREVER);

	ret = erase_flash_block(offset, size);

	k_sem_give(&dev_data->mutex);

	return ret;
}

static int flash_gecko_write_protection(struct device *dev, bool enable)
{
	struct flash_gecko_data *const dev_data = DEV_DATA(dev);

	k_sem_take(&dev_data->mutex, K_FOREVER);

	if (enable) {
		/* Lock the MSC module. */
		MSC->LOCK = 0;
	} else {
		/* Unlock the MSC module. */
		MSC->LOCK = MSC_UNLOCK_CODE;
	}

	k_sem_give(&dev_data->mutex);

	return 0;
}

/* Note:
 * - A flash address to write to must be aligned to words.
 * - Number of bytes to write must be divisible by 4.
 */
static bool write_range_is_valid(off_t offset, u32_t size)
{
	return read_range_is_valid(offset, size)
		&& (offset % sizeof(u32_t) == 0)
		&& (size % 4 == 0U);
}

static bool read_range_is_valid(off_t offset, u32_t size)
{
	return (offset + size) <= (CONFIG_FLASH_SIZE * 1024);
}

static int erase_flash_block(off_t offset, size_t size)
{
	MSC_Status_TypeDef msc_ret;
	void *address;
	int ret = 0;

	for (off_t tmp = offset; tmp < offset + size; tmp += FLASH_PAGE_SIZE) {
		address = (u8_t *)CONFIG_FLASH_BASE_ADDRESS + tmp;
		msc_ret = MSC_ErasePage(address);
		if (msc_ret < 0) {
			ret = -EIO;
			break;
		}
	}

	return ret;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_gecko_0_pages_layout = {
	.pages_count = (CONFIG_FLASH_SIZE * 1024) /
			DT_INST_0_SOC_NV_FLASH_ERASE_BLOCK_SIZE,
	.pages_size = DT_INST_0_SOC_NV_FLASH_ERASE_BLOCK_SIZE,
};

void flash_gecko_page_layout(struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	*layout = &flash_gecko_0_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int flash_gecko_init(struct device *dev)
{
	struct flash_gecko_data *const dev_data = DEV_DATA(dev);

	k_sem_init(&dev_data->mutex, 1, 1);

	MSC_Init();

	/* Lock the MSC module. */
	MSC->LOCK = 0;

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct flash_driver_api flash_gecko_driver_api = {
	.read = flash_gecko_read,
	.write = flash_gecko_write,
	.erase = flash_gecko_erase,
	.write_protection = flash_gecko_write_protection,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_gecko_page_layout,
#endif
	.write_block_size = DT_INST_0_SOC_NV_FLASH_WRITE_BLOCK_SIZE,
};

static struct flash_gecko_data flash_gecko_0_data;

DEVICE_AND_API_INIT(flash_gecko_0, DT_FLASH_DEV_NAME,
		    flash_gecko_init, &flash_gecko_0_data, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_gecko_driver_api);
