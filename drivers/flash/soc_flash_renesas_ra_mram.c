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

#if DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_nv_mram)
#define SOC_NV_MRAM_NODE DT_INST(0, renesas_ra_nv_mram)
#else
#error "Enable the 'renesas,ra-nv-mram' compatible node to use the soc_flash_renesas_ra_mram driver"
#endif

LOG_MODULE_REGISTER(flash_renesas_ra_mram, CONFIG_FLASH_LOG_LEVEL);

struct mram_renesas_ra_controller_data {
	mram_instance_ctrl_t ctrl;
	struct st_flash_cfg f_config;
	struct k_mutex code_mram_mtx;
};

struct mram_renesas_ra_controller_config {
	struct flash_parameters mram_parameters;
	size_t erase_block_size;
	size_t mram_size;
	uint32_t start_address;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout device_page_layout;
#endif
};

static struct mram_renesas_ra_controller_config mram_controller_config = {
	.mram_parameters = {.write_block_size = DT_PROP(SOC_NV_MRAM_NODE, write_block_size),
			    .erase_value = 0xff,
			    .caps = {.no_explicit_erase = true}},
	.erase_block_size = DT_PROP(SOC_NV_MRAM_NODE, erase_block_size),
	.mram_size = DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(0, DT_REG_SIZE, (+)),
	.start_address = DT_REG_ADDR(SOC_NV_MRAM_NODE),
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.device_page_layout = {.pages_count = (DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(0, DT_REG_SIZE,
										     (+))) /
					      DT_PROP(SOC_NV_MRAM_NODE, erase_block_size),
			       .pages_size = DT_PROP(SOC_NV_MRAM_NODE, erase_block_size)}
#endif
};

static struct mram_renesas_ra_controller_data mram_controller_data = {
	.f_config = {.data_flash_bgo = false,
		     .irq = FSP_INVALID_VECTOR,
		     .err_irq = FSP_INVALID_VECTOR,
		     .ipl = BSP_IRQ_DISABLED,
		     .err_ipl = BSP_IRQ_DISABLED,
		     .p_callback = NULL},
};

static bool mram_renesas_ra_valid_range(const struct mram_renesas_ra_controller_config *mram_cfg,
					off_t offset, size_t len)
{
	if ((offset < 0) || (offset >= mram_cfg->mram_size) ||
	    (mram_cfg->mram_size - offset < len) || (len > UINT32_MAX - offset)) {
		return false;
	}

	return true;
}

static int mram_renesas_ra_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct mram_renesas_ra_controller_config *mram_cfg = dev->config;
	struct mram_renesas_ra_controller_data *mram_data = dev->data;

	if (!len) {
		return 0;
	}

	if (!mram_renesas_ra_valid_range(mram_cfg, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("mram: read 0x%lx, len: %u", (long)(offset), len);

	k_mutex_lock(&mram_data->code_mram_mtx, K_FOREVER);

	memcpy(data, (uint8_t *)(offset + mram_cfg->start_address), len);

	k_mutex_unlock(&mram_data->code_mram_mtx);

	return 0;
}

static int mram_renesas_ra_write(const struct device *dev, off_t offset, const void *data,
				 size_t len)
{
	const struct mram_renesas_ra_controller_config *mram_cfg = dev->config;
	struct mram_renesas_ra_controller_data *mram_data = dev->data;
	fsp_err_t err;

	if (!len) {
		return 0;
	}

	if (!mram_renesas_ra_valid_range(mram_cfg, offset, len)) {
		return -EINVAL;
	}

	k_mutex_lock(&mram_data->code_mram_mtx, K_FOREVER);

	err = R_MRAM_Write(&mram_data->ctrl, (uint32_t)data,
			   (uint32_t)(offset + mram_cfg->start_address), len);

	k_mutex_unlock(&mram_data->code_mram_mtx);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int mram_renesas_ra_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct mram_renesas_ra_controller_config *mram_cfg = dev->config;
	struct mram_renesas_ra_controller_data *mram_data = dev->data;
	fsp_err_t err;
	uint32_t block_num;

	if (!size) {
		return 0;
	}

	if (!mram_renesas_ra_valid_range(mram_cfg, offset, size)) {
		return -EINVAL;
	}

	block_num = DIV_ROUND_UP(size, mram_cfg->erase_block_size);

	k_mutex_lock(&mram_data->code_mram_mtx, K_FOREVER);

	err = R_MRAM_Erase(&mram_data->ctrl, (uint32_t)(offset + mram_cfg->start_address),
			   block_num);

	k_mutex_unlock(&mram_data->code_mram_mtx);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static const struct flash_parameters *mram_renesas_ra_get_parameters(const struct device *dev)
{
	const struct mram_renesas_ra_controller_config *mram_cfg = dev->config;

	return &mram_cfg->mram_parameters;
}

static int mram_renesas_ra_get_size(const struct device *dev, uint64_t *size)
{
	const struct mram_renesas_ra_controller_config *mram_cfg = dev->config;

	*size = (uint64_t)mram_cfg->mram_size;
	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT

void mram_renesas_ra_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	const struct mram_renesas_ra_controller_config *mram_cfg = dev->config;

	*layout = &(mram_cfg->device_page_layout);
	*layout_size = 1;
}

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int mram_renesas_ra_controller_init(const struct device *dev)
{
	struct mram_renesas_ra_controller_data *mram_data = dev->data;
	fsp_err_t err;

	k_mutex_init(&mram_data->code_mram_mtx);

	err = R_MRAM_Open(&mram_data->ctrl, &mram_data->f_config);

	if (err != FSP_SUCCESS) {
		LOG_DBG("flash: open error=%d", (int)err);
		return -EIO;
	}

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

DEVICE_DT_DEFINE(DT_DRV_INST(0), mram_renesas_ra_controller_init, NULL, &mram_controller_data,
		 &mram_controller_config, PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY,
		 &mram_renesas_ra_api);
