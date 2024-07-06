/*
 * Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	  infineon_cat1_flash_controller
#define SOC_NV_FLASH_NODE DT_PARENT(DT_INST(0, fixed_partitions))

#define PAGE_LEN DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include "cyhal_flash.h"

LOG_MODULE_REGISTER(flash_infineon_cat1, CONFIG_FLASH_LOG_LEVEL);

/* Device config structure */
struct ifx_cat1_flash_config {
	uint32_t base_addr;
	uint32_t max_addr;
};

/* Data structure */
struct ifx_cat1_flash_data {
	cyhal_flash_t flash_obj;
	struct k_sem sem;
};

static struct flash_parameters ifx_cat1_flash_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = 0x00,
};

static inline void flash_ifx_sem_take(const struct device *dev)
{
	struct ifx_cat1_flash_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static inline void flash_ifx_sem_give(const struct device *dev)
{
	struct ifx_cat1_flash_data *data = dev->data;

	k_sem_give(&data->sem);
}

static int ifx_cat1_flash_read(const struct device *dev, k_off_t offset, void *data,
			       size_t data_len)
{
	struct ifx_cat1_flash_data *dev_data = dev->data;
	const struct ifx_cat1_flash_config *dev_config = dev->config;
	uint32_t read_offset = dev_config->base_addr + offset;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	int ret = 0;

	flash_ifx_sem_take(dev);

	rslt = cyhal_flash_read(&dev_data->flash_obj, read_offset, (uint8_t *)data, data_len);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Error reading @ 0x%x (Err:0x%x)", read_offset, rslt);
		ret = -EIO;
		goto out;
	}

out:
	flash_ifx_sem_give(dev);
	return ret;
}

static int ifx_cat1_flash_write(const struct device *dev, k_off_t offset, const void *data,
				size_t data_len)
{
	struct ifx_cat1_flash_data *dev_data = dev->data;
	const struct ifx_cat1_flash_config *dev_config = dev->config;
	uint32_t write_offset = dev_config->base_addr + (uint32_t)offset;
	const uint8_t *data_ptr = (const uint8_t *)data;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	int ret = 0;

	if (data_len == 0) {
		return 0;
	}

	if ((offset < 0) || (data_len % PAGE_LEN != 0) || (offset % PAGE_LEN != 0)) {
		return -EINVAL;
	}

	flash_ifx_sem_take(dev);

	while (data_len) {
		rslt = cyhal_flash_write(&dev_data->flash_obj, write_offset,
					 (const uint32_t *)data_ptr);
		if (rslt != CY_RSLT_SUCCESS) {
			LOG_ERR("Error in writing @ 0x%x (Err:0x%x)", write_offset, rslt);
			ret = -EIO;
			goto out;
		}

		data_ptr += PAGE_LEN;
		write_offset += PAGE_LEN;
		data_len -= PAGE_LEN;
	}

out:
	flash_ifx_sem_give(dev);
	return ret;
}

static int ifx_cat1_flash_erase(const struct device *dev, k_off_t offset, size_t size)
{
	struct ifx_cat1_flash_data *data = dev->data;
	const struct ifx_cat1_flash_config *config = dev->config;
	uint32_t erase_offset = config->base_addr + (uint32_t)offset;
	cy_rslt_t rslt;

	if ((offset < 0) || ((offset % PAGE_LEN) != 0)) {
		return -EINVAL;
	}

	if (((erase_offset + size) > config->max_addr) || ((size % PAGE_LEN) != 0)) {
		return -EINVAL;
	}

	while (size) {
		rslt = cyhal_flash_erase(&data->flash_obj, erase_offset);
		if (rslt != CY_RSLT_SUCCESS) {
			LOG_ERR("Error in erasing : 0x%x", rslt);
			return -EIO;
		}

		size -= PAGE_LEN;
		erase_offset += PAGE_LEN;
	}

	return 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout ifx_cat1_flash_pages_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) / PAGE_LEN,
	.pages_size = PAGE_LEN,
};

static void ifx_cat1_flash_page_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	*layout = &ifx_cat1_flash_pages_layout;

	/*
	 * For flash memories which have uniform page sizes, this routine
	 * returns an array of length 1, which specifies the page size and
	 * number of pages in the memory.
	 */
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *ifx_cat1_flash_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &ifx_cat1_flash_parameters;
}

static int ifx_cat1_flash_init(const struct device *dev)
{
	struct ifx_cat1_flash_data *data = dev->data;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;

	rslt = cyhal_flash_init(&data->flash_obj);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Failed to init flash hal driver (Err:0x%x)", rslt);
		return -EIO;
	}

	k_sem_init(&data->sem, 1, 1);

	return 0;
}

static const struct flash_driver_api ifx_cat1_flash_driver_api = {
	.read = ifx_cat1_flash_read,
	.write = ifx_cat1_flash_write,
	.erase = ifx_cat1_flash_erase,
	.get_parameters = ifx_cat1_flash_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = ifx_cat1_flash_page_layout,
#endif
};

static struct ifx_cat1_flash_data flash_data;

static const struct ifx_cat1_flash_config ifx_cat1_flash_config = {
	.base_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE),
	.max_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE) + DT_REG_SIZE(SOC_NV_FLASH_NODE)};

DEVICE_DT_INST_DEFINE(0, ifx_cat1_flash_init, NULL, &flash_data, &ifx_cat1_flash_config,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &ifx_cat1_flash_driver_api);
