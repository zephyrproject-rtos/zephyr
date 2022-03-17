/*
 * Copyright (c) 2022 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_flash_controller

#include <errno.h>
#include <kernel.h>
#include <devicetree.h>
#include <drivers/flash.h>
#include <soc.h>

#include "flash_gd32.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(flash_gd32, CONFIG_FLASH_LOG_LEVEL);

struct flash_gd32_data {
	struct k_sem mutex;
};

static struct flash_gd32_data flash_data;

static const struct flash_parameters flash_gd32_parameters = {
	.write_block_size = SOC_NV_FLASH_WRITE_BLOCK_SIZE,
	.erase_value = 0xff,
};

__weak int flash_gd32_programming(off_t offset, const void *data, size_t len)
{
	ARG_UNUSED(offset);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	return -EINVAL;
}

__weak int flash_gd32_page_erase(uint32_t page)
{
	ARG_UNUSED(page);
	return -EINVAL;
}

__weak void flash_gd32_pages_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(layout);
	ARG_UNUSED(layout_size);
}

static inline void flash_gd32_lock(void)
{
	FMC_CTL |= FMC_CTL_LK;
}

static inline void flash_gd32_unlock(void)
{
	FMC_KEY = UNLOCK_KEY0;
	FMC_KEY = UNLOCK_KEY1;
}

static bool flash_gd32_valid_range(off_t offset, size_t len, bool write)
{
	if ((offset < 0) || (offset >= SOC_NV_FLASH_SIZE)) {
		return false;
	}

	if (len == 0) {
		return true;
	}

	if ((offset + len) > SOC_NV_FLASH_SIZE) {
		return false;
	}

	if (write && (len % sizeof(flash_prog_t))) {
		return false;
	}

	return true;
}

static int flash_gd32_read(const struct device *dev, off_t offset,
			   void *data, size_t len)
{
	if (!flash_gd32_valid_range(offset, len, false)) {
		return -EINVAL;
	}

	memcpy(data, (uint8_t *)SOC_NV_FLASH_ADDR + offset, len);

	return 0;
}

static int flash_gd32_write(const struct device *dev, off_t offset,
			    const void *data, size_t len)
{
	struct flash_gd32_data *dev_data = dev->data;
	int ret;

	if (!flash_gd32_valid_range(offset, len, true)) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	k_sem_take(&dev_data->mutex, K_FOREVER);

	flash_gd32_unlock();

	ret = flash_gd32_programming(offset, data, len);
	if (ret < 0) {
		LOG_ERR("Failed to write data, error (%u)", ret);
		goto error;
	}

error:
	flash_gd32_lock();

	k_sem_give(&dev_data->mutex);

	return ret;
}

static int flash_gd32_erase(const struct device *dev, off_t offset,
			    size_t size)
{
	struct flash_gd32_data *dev_data = dev->data;
	struct flash_pages_info info;
	uint32_t begin, end;
	int ret;

	ret = flash_get_page_info_by_offs(dev, offset, &info);
	if (ret < 0) {
		return ret;
	}
	begin = info.index;

	ret = flash_get_page_info_by_offs(dev, offset + size - 1, &info);
	if (ret < 0) {
		return ret;
	}
	end = info.index;

	k_sem_take(&dev_data->mutex, K_FOREVER);

	flash_gd32_unlock();

	for ( ; begin <= end; begin++) {
		ret = flash_gd32_page_erase(begin);
		if (ret < 0) {
			LOG_ERR("Failed to erase the page (%u), error (%d)",
				begin, ret);
			goto error;
		}
	}
error:
	flash_gd32_lock();

	k_sem_give(&dev_data->mutex);

	return ret;
}

static const struct flash_parameters*
flash_gd32_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_gd32_parameters;
}

static const struct flash_driver_api flash_gd32_driver_api = {
	.read = flash_gd32_read,
	.write = flash_gd32_write,
	.erase = flash_gd32_erase,
	.get_parameters = flash_gd32_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_gd32_pages_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

static int flash_gd32_init(const struct device *dev)
{
	struct flash_gd32_data *data = dev->data;

	k_sem_init(&data->mutex, 1, 1);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_gd32_init, NULL,
		      &flash_data, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_gd32_driver_api);
