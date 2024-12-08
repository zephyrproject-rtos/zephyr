/*
 * Copyright (c) 2022 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_flash_controller

#include "flash_gd32.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include <gd32_fmc.h>

LOG_MODULE_REGISTER(flash_gd32, CONFIG_FLASH_LOG_LEVEL);

struct flash_gd32_data {
	struct k_sem mutex;
};

static struct flash_gd32_data flash_data;

static const struct flash_parameters flash_gd32_parameters = {
	.write_block_size = SOC_NV_FLASH_PRG_SIZE,
	.erase_value = 0xff,
};

static int flash_gd32_read(const struct device *dev, off_t offset,
			   void *data, size_t len)
{
	if ((offset > SOC_NV_FLASH_SIZE) ||
	    ((offset + len) > SOC_NV_FLASH_SIZE)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	memcpy(data, (uint8_t *)SOC_NV_FLASH_ADDR + offset, len);

	return 0;
}

static int flash_gd32_write(const struct device *dev, off_t offset,
			    const void *data, size_t len)
{
	struct flash_gd32_data *dev_data = dev->data;
	int ret = 0;

	if (!flash_gd32_valid_range(offset, len, true)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	k_sem_take(&dev_data->mutex, K_FOREVER);

	ret = flash_gd32_write_range(offset, data, len);

	k_sem_give(&dev_data->mutex);

	return ret;
}

static int flash_gd32_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_gd32_data *data = dev->data;
	int ret = 0;

	if (size == 0U) {
		return 0;
	}

	if (!flash_gd32_valid_range(offset, size, false)) {
		return -EINVAL;
	}

	k_sem_take(&data->mutex, K_FOREVER);

	ret = flash_gd32_erase_block(offset, size);

	k_sem_give(&data->mutex);

	return ret;
}

static const struct flash_parameters*
flash_gd32_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_gd32_parameters;
}

static DEVICE_API(flash, flash_gd32_driver_api) = {
	.read = flash_gd32_read,
	.write = flash_gd32_write,
	.erase = flash_gd32_erase,
	.get_parameters = flash_gd32_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_gd32_pages_layout,
#endif
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
