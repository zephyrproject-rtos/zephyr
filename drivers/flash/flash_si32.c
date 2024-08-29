/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si32_flash_controller

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>

#include <SI32_FLASHCTRL_A_Type.h>

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_si32);

#define SOC_NV_FLASH_NODE             DT_INST(0, soc_nv_flash)
#define SOC_NV_FLASH_SIZE             DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define SOC_NV_FLASH_ADDR             DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define SOC_NV_FLASH_WRITE_BLOCK_SIZE DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define SOC_NV_FLASH_ERASE_BLOCK_SIZE DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
BUILD_ASSERT(SOC_NV_FLASH_WRITE_BLOCK_SIZE == 2, "other values weren't tested yet");

struct flash_si32_data {
	struct k_sem mutex;
};

struct flash_si32_config {
	SI32_FLASHCTRL_A_Type *controller;
};

static const struct flash_parameters flash_si32_parameters = {
	.write_block_size = SOC_NV_FLASH_WRITE_BLOCK_SIZE,
	.erase_value = 0xff,
};

static bool flash_si32_valid_range(off_t offset, uint32_t size, bool write)
{
	if (offset < 0) {
		return false;
	}

	if ((offset > SOC_NV_FLASH_SIZE) || ((offset + size) > SOC_NV_FLASH_SIZE)) {
		return false;
	}

	if (write) {
		if ((offset % SOC_NV_FLASH_WRITE_BLOCK_SIZE) != 0) {
			return -EINVAL;
		}

		if ((size % SOC_NV_FLASH_WRITE_BLOCK_SIZE) != 0) {
			return -EINVAL;
		}
	}

	return true;
}

static int flash_si32_read(const struct device *dev, off_t offset, void *data, size_t size)
{
	if (!flash_si32_valid_range(offset, size, false)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	memcpy(data, (uint8_t *)CONFIG_FLASH_BASE_ADDRESS + offset, size);

	return 0;
}

static int flash_si32_write(const struct device *dev, off_t offset, const void *data_, size_t size)
{
	const uint8_t *data = data_;
	struct flash_si32_data *const dev_data = dev->data;
	const struct flash_si32_config *const config = dev->config;

	if (!flash_si32_valid_range(offset, size, true)) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	k_sem_take(&dev_data->mutex, K_FOREVER);

	SI32_FLASHCTRL_A_write_wraddr(config->controller, offset);
	SI32_FLASHCTRL_A_enter_multi_byte_write_mode(config->controller);
	SI32_FLASHCTRL_A_write_flash_key(config->controller, 0xA5);
	SI32_FLASHCTRL_A_write_flash_key(config->controller, 0xF2);

	for (size_t i = 0; i < size; i += SOC_NV_FLASH_WRITE_BLOCK_SIZE) {
		const uint16_t halfword = (uint16_t)(data[i]) | ((uint16_t)data[i + 1] << 8);

		SI32_FLASHCTRL_A_write_wrdata(config->controller, (uint32_t)halfword);

		while (SI32_FLASHCTRL_A_is_flash_busy(config->controller)) {
		}
	}

	while (SI32_FLASHCTRL_A_is_buffer_full(config->controller)) {
	}

	SI32_FLASHCTRL_A_write_flash_key(config->controller, 0x5A);

	k_sem_give(&dev_data->mutex);

	return 0;
}

static int flash_si32_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_si32_data *const dev_data = dev->data;
	const struct flash_si32_config *const config = dev->config;

	if (!flash_si32_valid_range(offset, size, false)) {
		return -EINVAL;
	}

	if ((offset % SOC_NV_FLASH_ERASE_BLOCK_SIZE) != 0) {
		LOG_ERR("offset 0x%lx: not on a page boundary", (long)offset);
		return -EINVAL;
	}

	if ((size % SOC_NV_FLASH_ERASE_BLOCK_SIZE) != 0) {
		LOG_ERR("size %zu: not multiple of a page size", size);
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	k_sem_take(&dev_data->mutex, K_FOREVER);

	SI32_FLASHCTRL_A_enter_flash_erase_mode(config->controller);
	SI32_FLASHCTRL_A_write_flash_key(config->controller, 0xA5);
	SI32_FLASHCTRL_A_write_flash_key(config->controller, 0xF2);

	for (size_t i = 0; i < size; i += SOC_NV_FLASH_ERASE_BLOCK_SIZE) {
		SI32_FLASHCTRL_A_write_wraddr(config->controller, offset + i);
		SI32_FLASHCTRL_A_write_wrdata(config->controller, 0);

		while (SI32_FLASHCTRL_A_is_buffer_full(config->controller)) {
		}
	}

	SI32_FLASHCTRL_A_write_flash_key(config->controller, 0x5A);
	SI32_FLASHCTRL_A_exit_flash_erase_mode(config->controller);

	k_sem_give(&dev_data->mutex);

	return 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_si32_0_pages_layout = {
	.pages_count = SOC_NV_FLASH_SIZE / SOC_NV_FLASH_ERASE_BLOCK_SIZE,
	.pages_size = SOC_NV_FLASH_ERASE_BLOCK_SIZE,
};

void flash_si32_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			    size_t *layout_size)
{
	*layout = &flash_si32_0_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_si32_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_si32_parameters;
}

static int flash_si32_init(const struct device *dev)
{
	struct flash_si32_data *const dev_data = dev->data;
	const struct flash_si32_config *const config = dev->config;

	k_sem_init(&dev_data->mutex, 1, 1);

	SI32_FLASHCTRL_A_exit_read_store_mode(config->controller);

	return 0;
}

static const struct flash_driver_api flash_si32_driver_api = {
	.read = flash_si32_read,
	.write = flash_si32_write,
	.erase = flash_si32_erase,
	.get_parameters = flash_si32_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_si32_page_layout,
#endif
};

static struct flash_si32_data flash_si32_0_data;

static const struct flash_si32_config flash_si32_config = {
	.controller = (SI32_FLASHCTRL_A_Type *)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, flash_si32_init, NULL, &flash_si32_0_data, &flash_si32_config, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_si32_driver_api);
