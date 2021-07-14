/*
 * Copyright (c) 2018, Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <em_msc.h>
#include <zephyr/drivers/flash.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_gecko);

struct flash_gecko_data {
	struct k_sem mutex;
};


static const struct flash_parameters flash_gecko_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = 0xff,
};

static bool write_range_is_valid(off_t offset, uint32_t size);
static bool read_range_is_valid(off_t offset, uint32_t size);
static int erase_flash_block(off_t offset, size_t size);
static void flash_gecko_write_protection(bool enable);

static int flash_gecko_read(const struct device *dev, off_t offset,
			    void *data,
			    size_t size)
{
	if (!read_range_is_valid(offset, size)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	memcpy(data, (uint8_t *)CONFIG_FLASH_BASE_ADDRESS + offset, size);

	return 0;
}

static int flash_gecko_write(const struct device *dev, off_t offset,
			     const void *data, size_t size)
{
	struct flash_gecko_data *const dev_data = dev->data;
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
	flash_gecko_write_protection(false);

	address = (uint8_t *)CONFIG_FLASH_BASE_ADDRESS + offset;
	msc_ret = MSC_WriteWord(address, data, size);
	if (msc_ret < 0) {
		ret = -EIO;
	}

	flash_gecko_write_protection(true);
	k_sem_give(&dev_data->mutex);

	return ret;
}

static int flash_gecko_erase(const struct device *dev, off_t offset,
			     size_t size)
{
	struct flash_gecko_data *const dev_data = dev->data;
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
	flash_gecko_write_protection(false);

	ret = erase_flash_block(offset, size);

	flash_gecko_write_protection(true);
	k_sem_give(&dev_data->mutex);

	return ret;
}

static void flash_gecko_write_protection(bool enable)
{
	if (enable) {
		/* Lock the MSC module. */
		MSC->LOCK = 0;
	} else {
		/* Unlock the MSC module. */
	#if defined(MSC_LOCK_LOCKKEY_UNLOCK)
		MSC->LOCK = MSC_LOCK_LOCKKEY_UNLOCK;
	#else
		MSC->LOCK = MSC_UNLOCK_CODE;
	#endif
	}
}

/* Note:
 * - A flash address to write to must be aligned to words.
 * - Number of bytes to write must be divisible by 4.
 */
static bool write_range_is_valid(off_t offset, uint32_t size)
{
	return read_range_is_valid(offset, size)
		&& (offset % sizeof(uint32_t) == 0)
		&& (size % 4 == 0U);
}

static bool read_range_is_valid(off_t offset, uint32_t size)
{
	return (offset + size) <= (CONFIG_FLASH_SIZE * 1024);
}

static int erase_flash_block(off_t offset, size_t size)
{
	MSC_Status_TypeDef msc_ret;
	void *address;
	int ret = 0;

	for (off_t tmp = offset; tmp < offset + size; tmp += FLASH_PAGE_SIZE) {
		address = (uint8_t *)CONFIG_FLASH_BASE_ADDRESS + tmp;
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
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
				DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

void flash_gecko_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	*layout = &flash_gecko_0_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_gecko_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_gecko_parameters;
}

static int flash_gecko_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = (uint64_t)DT_REG_SIZE(SOC_NV_FLASH_NODE);

	return 0;
}

static int flash_gecko_init(const struct device *dev)
{
	struct flash_gecko_data *const dev_data = dev->data;

	k_sem_init(&dev_data->mutex, 1, 1);

	MSC_Init();

	/* Lock the MSC module. */
	MSC->LOCK = 0;

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct flash_driver_api flash_gecko_driver_api = {
	.read = flash_gecko_read,
	.write = flash_gecko_write,
	.erase = flash_gecko_erase,
	.get_parameters = flash_gecko_get_parameters,
	.get_size = flash_gecko_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_gecko_page_layout,
#endif
};

static struct flash_gecko_data flash_gecko_0_data;

DEVICE_DT_INST_DEFINE(0, flash_gecko_init, NULL,
		    &flash_gecko_0_data, NULL, POST_KERNEL,
		    CONFIG_FLASH_INIT_PRIORITY, &flash_gecko_driver_api);
