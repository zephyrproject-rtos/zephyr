/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>

#include "flc.h"

struct max32_flash_dev_config {
	uint32_t flash_base;
	uint32_t flash_erase_blk_sz;
	struct flash_parameters parameters;
#if CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout pages_layouts;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

struct max32_flash_dev_data {
#ifdef CONFIG_MULTITHREADING
	struct k_sem sem;
#endif
};

#ifdef CONFIG_MULTITHREADING
static inline void max32_sem_take(const struct device *dev)
{
	struct max32_flash_dev_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static inline void max32_sem_give(const struct device *dev)
{
	struct max32_flash_dev_data *data = dev->data;

	k_sem_give(&data->sem);
}
#else

#define max32_sem_take(dev)
#define max32_sem_give(dev)

#endif /* CONFIG_MULTITHREADING */

static int api_read(const struct device *dev, off_t address, void *buffer, size_t length)
{
	const struct max32_flash_dev_config *const cfg = dev->config;

	address += cfg->flash_base;
	MXC_FLC_Read(address, buffer, length);
	return 0;
}

static int api_write(const struct device *dev, off_t address, const void *buffer, size_t length)
{
	const struct max32_flash_dev_config *const cfg = dev->config;
	int ret = 0;

	max32_sem_take(dev);

	address += cfg->flash_base;
	ret = MXC_FLC_Write(address, length, (uint32_t *)buffer);

	max32_sem_give(dev);

	return ret != 0 ? -EIO : 0;
}

static int api_erase(const struct device *dev, off_t start, size_t len)
{
	const struct max32_flash_dev_config *const cfg = dev->config;
	uint32_t page_size = cfg->flash_erase_blk_sz;
	uint32_t addr = (start + cfg->flash_base);
	int ret = 0;

	max32_sem_take(dev);

	while (len) {
		ret = MXC_FLC_PageErase(addr);
		if (ret) {
			break;
		}

		addr += page_size;
		if (len > page_size) {
			len -= page_size;
		} else {
			len = 0;
		}
	}

	max32_sem_give(dev);

	return ret != 0 ? -EIO : 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static void api_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			    size_t *layout_size)
{
	const struct max32_flash_dev_config *const cfg = dev->config;

	*layout = &cfg->pages_layouts;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *api_get_parameters(const struct device *dev)
{
	const struct max32_flash_dev_config *const cfg = dev->config;

	return &cfg->parameters;
}

static int flash_max32_init(const struct device *dev)
{
	int ret = MXC_FLC_Init();

#ifdef CONFIG_MULTITHREADING
	struct max32_flash_dev_data *data = dev->data;

	/* Mutex for flash controller */
	k_sem_init(&data->sem, 1, 1);
#endif
	return ret != 0 ? -EIO : 0;
}

static DEVICE_API(flash, flash_max32_driver_api) = {
	.read = api_read,
	.write = api_write,
	.erase = api_erase,
	.get_parameters = api_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = api_page_layout,
#endif
};

#if CONFIG_FLASH_PAGE_LAYOUT
#define FLASH_MAX32_CONFIG_PAGE_LAYOUT(n)                                                          \
	.pages_layouts = {                                                                         \
		.pages_count = DT_INST_FOREACH_CHILD(n, GET_FLASH_SIZE) /                          \
			       DT_INST_FOREACH_CHILD(n, GET_ERASE_BLOCK_SIZE),                     \
		.pages_size = DT_INST_FOREACH_CHILD(n, GET_ERASE_BLOCK_SIZE),                      \
	},
#else
#define FLASH_MAX32_CONFIG_PAGE_LAYOUT(n)
#endif

#define GET_WRITE_BLOCK_SIZE(n) DT_PROP(n, write_block_size)
#define GET_ERASE_BLOCK_SIZE(n) DT_PROP(n, erase_block_size)
#define GET_FLASH_BASE(n)       DT_REG_ADDR(n)
#define GET_FLASH_SIZE(n)       DT_REG_SIZE(n)

#define DEFINE_FLASH_MAX32(_num)                                                                   \
	static const struct max32_flash_dev_config max32_flash_dev_cfg_##_num = {                  \
		.flash_base = DT_INST_FOREACH_CHILD(_num, GET_FLASH_BASE),                         \
		.flash_erase_blk_sz = DT_INST_FOREACH_CHILD(_num, GET_ERASE_BLOCK_SIZE),           \
		.parameters =                                                                      \
			{                                                                          \
				.write_block_size =                                                \
					DT_INST_FOREACH_CHILD(_num, GET_WRITE_BLOCK_SIZE),         \
				.erase_value = 0xFF,                                               \
			},                                                                         \
		FLASH_MAX32_CONFIG_PAGE_LAYOUT(_num)};                                             \
	static struct max32_flash_dev_data max32_flash_dev_data_##_num;                            \
	DEVICE_DT_INST_DEFINE(_num, flash_max32_init, NULL, &max32_flash_dev_data_##_num,          \
			      &max32_flash_dev_cfg_##_num, POST_KERNEL,                            \
			      CONFIG_FLASH_INIT_PRIORITY, &flash_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_FLASH_MAX32)
