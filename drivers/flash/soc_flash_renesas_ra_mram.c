/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>

#include <r_flash_api.h>
#include <r_mram.h>
#include <soc.h>

#define DT_DRV_COMPAT renesas_ra_mram_controller

LOG_MODULE_REGISTER(flash_renesas_ra_mram, CONFIG_FLASH_LOG_LEVEL);

struct mram_renesas_ra_controller_data {
	mram_instance_ctrl_t mram_controller;
	struct st_flash_cfg f_config;
	struct k_mutex code_mram_mtx;
};

struct mram_renesas_ra_config {
	struct flash_parameters mram_parameters;
	size_t erase_block_size;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout device_page_layout;
#endif
};

struct mram_renesas_ra_data {
	struct mram_renesas_ra_controller_data *controller_data;
	uint32_t area_address;
	uint32_t area_size;
};

static struct mram_renesas_ra_controller_data mram_controller_data = {
	.f_config = {
		.data_flash_bgo = false,
		.irq = FSP_INVALID_VECTOR,
		.err_irq = FSP_INVALID_VECTOR,
		.ipl = BSP_IRQ_DISABLED,
		.err_ipl = BSP_IRQ_DISABLED,
		.p_callback = NULL,
	},
};

static bool mram_renesas_ra_valid_range(struct mram_renesas_ra_data *mram_data, off_t offset,
					size_t len)
{
	if ((offset < 0) || (offset >= mram_data->area_size) ||
	    (mram_data->area_size - offset < len) || (len > UINT32_MAX - offset)) {
		return false;
	}

	return true;
}

static int mram_renesas_ra_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct mram_renesas_ra_data *mram_data = dev->data;
	struct mram_renesas_ra_controller_data *ctrl_data = mram_data->controller_data;

	if (!len) {
		return 0;
	}

	if (!mram_renesas_ra_valid_range(mram_data, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("mram: read 0x%lx, len: %u", (long)(offset), len);

	k_mutex_lock(&ctrl_data->code_mram_mtx, K_FOREVER);

	memcpy(data, (uint8_t *)(offset + mram_data->area_address), len);

	k_mutex_unlock(&ctrl_data->code_mram_mtx);

	return 0;
}

static int mram_renesas_ra_write(const struct device *dev, off_t offset, const void *data,
				 size_t len)
{
	struct mram_renesas_ra_data *mram_data = dev->data;
	struct mram_renesas_ra_controller_data *ctrl_data = mram_data->controller_data;
	fsp_err_t err;

	if (!len) {
		return 0;
	}

	if (!mram_renesas_ra_valid_range(mram_data, offset, len)) {
		return -EINVAL;
	}

	k_mutex_lock(&ctrl_data->code_mram_mtx, K_FOREVER);

	err = R_MRAM_Write(ctrl_data, (uint32_t)data, (uint32_t)(offset + mram_data->area_address),
			   len);

	k_mutex_unlock(&ctrl_data->code_mram_mtx);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int mram_renesas_ra_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct mram_renesas_ra_config *mram_config = dev->config;
	struct mram_renesas_ra_data *mram_data = dev->data;
	struct mram_renesas_ra_controller_data *ctrl_data = mram_data->controller_data;
	fsp_err_t err;
	uint32_t block_num;

	if (!size) {
		return 0;
	}

	if (!mram_renesas_ra_valid_range(mram_data, offset, size)) {
		return -EINVAL;
	}

	block_num = DIV_ROUND_UP(size, mram_config->erase_block_size);

	k_mutex_lock(&ctrl_data->code_mram_mtx, K_FOREVER);

	err = R_MRAM_Erase(ctrl_data, (uint32_t)(offset + mram_data->area_address), block_num);

	k_mutex_unlock(&ctrl_data->code_mram_mtx);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static const struct flash_parameters *mram_renesas_ra_get_parameters(const struct device *dev)
{
	const struct mram_renesas_ra_config *mram_config = dev->config;

	return &mram_config->mram_parameters;
}

static int mram_renesas_ra_get_size(const struct device *dev, uint64_t *size)
{
	struct mram_renesas_ra_data *mram_data = dev->data;

	*size = (uint64_t)mram_data->area_size;
	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT

void mram_renesas_ra_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	const struct mram_renesas_ra_config *mram_config = dev->config;

	*layout = &(mram_config->device_page_layout);
	*layout_size = 1;
}

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int mram_renesas_ra_controller_init(const struct device *dev)
{
	fsp_err_t err;
	struct mram_renesas_ra_controller_data *data = dev->data;

	k_mutex_init(&data->code_mram_mtx);

	err = R_MRAM_Open(&data->mram_controller, &data->f_config);

	if (err != FSP_SUCCESS) {
		LOG_DBG("flash: open error=%d", (int)err);
		return -EIO;
	}

	return 0;
}

static int mram_renesas_ra_init(const struct device *dev)
{
	const struct device *dev_ctrl = DEVICE_DT_INST_GET(0);
	struct mram_renesas_ra_data *mram_data = dev->data;

	if (!device_is_ready(dev_ctrl)) {
		return -ENODEV;
	}

	mram_data->controller_data = dev_ctrl->data;

	return 0;
}

static DEVICE_API(flash, mram_renesas_ra_api) = {
	.erase = mram_renesas_ra_erase,
	.write = mram_renesas_ra_write,
	.read = mram_renesas_ra_read,
	.get_parameters = mram_renesas_ra_get_parameters,
	.get_size = mram_renesas_ra_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = mram_renesas_ra_page_layout,
#endif
};

#ifdef CONFIG_FLASH_PAGE_LAYOUT
#define MRAM_RENESAS_RA_INIT_DEVICE_PAGE_LAYOUT(index)                                             \
	.device_page_layout = {                                                                    \
		.pages_count = (DT_REG_SIZE(index) / DT_PROP(index, erase_block_size)),            \
		.pages_size = DT_PROP(index, erase_block_size),                                    \
	}
#else
#define MRAM_RENESAS_RA_INIT_DEVICE_PAGE_LAYOUT(index)
#endif

#define MRAM_RENESAS_RA_INIT(index)                                                                \
	static struct mram_renesas_ra_data mram_renesas_ra_data_##index = {                        \
		.area_address = DT_REG_ADDR(index),                                                \
		.area_size = DT_REG_SIZE(index),                                                   \
	};                                                                                         \
	static const struct mram_renesas_ra_config mram_renesas_ra_config_##index = {              \
		.mram_parameters =                                                                 \
			{                                                                          \
				.write_block_size = DT_PROP(index, write_block_size),              \
				.erase_value = 0xff,                                               \
				.caps =                                                            \
					{                                                          \
						.no_explicit_erase = true,                         \
					},                                                         \
			},                                                                         \
		.erase_block_size = DT_PROP(index, erase_block_size),                              \
		MRAM_RENESAS_RA_INIT_DEVICE_PAGE_LAYOUT(index),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(index, mram_renesas_ra_init, NULL, &mram_renesas_ra_data_##index,         \
			 &mram_renesas_ra_config_##index, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, \
			 &mram_renesas_ra_api);

DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), MRAM_RENESAS_RA_INIT);

DEVICE_DT_DEFINE(DT_DRV_INST(0), mram_renesas_ra_controller_init, NULL, &mram_controller_data, NULL,
		 PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);
