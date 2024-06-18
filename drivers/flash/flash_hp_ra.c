/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL  CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <string.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include "flash_hp_ra.h"

#define DT_DRV_COMPAT renesas_ra_flash_hp_controller

LOG_MODULE_REGISTER(flash_hp_ra, CONFIG_FLASH_LOG_LEVEL);

static int flash_ra_init(const struct device *dev);
static int flash_ra_erase(const struct device *dev, off_t offset, size_t len);
static int flash_ra_read(const struct device *dev, off_t offset, void *data, size_t len);
static int flash_ra_write(const struct device *dev, off_t offset, const void *data, size_t len);
static const struct flash_parameters *flash_ra_get_parameters(const struct device *dev);
#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_ra_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			  size_t *layout_size);
#endif

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_ra_ex_op(const struct device *dev, uint16_t code, const uintptr_t in, void *out);
#endif

static struct flash_hp_ra_data flash_hp_ra_data = {.fsp_config = {
								.data_flash_bgo = true,
								.p_context = NULL,
#if defined(VECTOR_NUMBER_FCU_FRDYI)
								.irq = VECTOR_NUMBER_FCU_FRDYI,
#else
								.irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_FCU_FIFERR)
								.err_irq = VECTOR_NUMBER_FCU_FIFERR,
#else
								.err_irq = FSP_INVALID_VECTOR,
#endif
								.err_ipl = (3),
								.ipl = (3),
						   }};

static struct flash_hp_ra_config flash_hp_ra_config = {
	.flash_ra_parameters = {
		.write_block_size = FLASH_RA_WRITE_BLOCK_SIZE,
		.erase_value = 0xff,
	}
};

static const struct flash_driver_api flash_ra_api = {
	.erase = flash_ra_erase,
	.write = flash_ra_write,
	.read = flash_ra_read,
	.get_parameters = flash_ra_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_ra_page_layout,
#endif
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_ra_ex_op,
#endif
};

struct flash_pages_layout flash_ra_layout[5];

static int flash_ra_init(const struct device *dev)
{
	fsp_err_t err = FSP_SUCCESS;
	struct flash_hp_ra_data *data = dev->data;

	err = R_FLASH_HP_Open(&data->flash_ctrl, &data->fsp_config);
	LOG_DBG("flash: open error=%d", err);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int flash_ra_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	if ((offset < 0) || offset >= SOC_NV_FLASH_SIZE || (SOC_NV_FLASH_SIZE - offset) < len) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("flash: read 0x%lx, len: %u", (long)(offset + CONFIG_FLASH_BASE_ADDRESS), len);

	memcpy(data, (uint8_t *)(offset + CONFIG_FLASH_BASE_ADDRESS), len);

	return 0;
}

static int flash_ra_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_hp_ra_data *data = dev->data;
	static struct flash_pages_info page_info_off, page_info_len;
	fsp_err_t err = FSP_ERR_ASSERTION;
	uint32_t block_num;
	int rc, rc2;

	if ((offset < 0) || offset >= SOC_NV_FLASH_SIZE || (SOC_NV_FLASH_SIZE - offset) < len) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("flash: erase 0x%lx, len: %u", (long)(offset + CONFIG_FLASH_BASE_ADDRESS), len);

	rc = flash_get_page_info_by_offs(dev, offset, &page_info_off);
	if (rc != 0) {
		return -EIO;
	}

	if (offset != page_info_off.start_offset) {
		return -EIO;
	}

	rc2 = flash_get_page_info_by_offs(dev, (offset + len), &page_info_len);
	if (rc2 != 0) {
		return -EIO;
	}

#if defined(CONFIG_DUAL_BANK_MODE)
	/* Invalid offset in dual bank mode, this is reversed area. */
	if ((page_info_off.index > FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_END
		&& page_info_off.index < FLASH_HP_CF_BLOCK_8KB_HIGH_START)
		|| (page_info_len.index > FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_END
		&& page_info_len.index < FLASH_HP_CF_BLOCK_8KB_HIGH_START)) {
		return -EIO;
	}
#endif

	if ((offset + len) != (page_info_len.start_offset)) {
		return -EIO;
	}

	block_num = (uint32_t)((page_info_len.index) - page_info_off.index);

	if (block_num > 0) {
		int key = irq_lock();

		err = R_FLASH_HP_Erase(&data->flash_ctrl,
		(long)(CONFIG_FLASH_BASE_ADDRESS + offset), block_num);

		irq_unlock(key);
	}

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int flash_ra_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	fsp_err_t err = FSP_ERR_ASSERTION;
	struct flash_hp_ra_data *flash_data = dev->data;

	if ((offset < 0) || offset >= SOC_NV_FLASH_SIZE || (SOC_NV_FLASH_SIZE - offset) < len) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("flash: write 0x%lx, len: %u", (long)(offset + CONFIG_FLASH_BASE_ADDRESS), len);

	int key = irq_lock();

	err = R_FLASH_HP_Write(&flash_data->flash_ctrl, (uint32_t)data,
	(long)(offset + CONFIG_FLASH_BASE_ADDRESS), len);

	irq_unlock(key);

	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

void flash_ra_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			  size_t *layout_size)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_DUAL_BANK_MODE)
	flash_ra_layout[0].pages_count =
			(FLASH_HP_CF_BLOCK_8KB_LOW_END - FLASH_HP_CF_BLOCK_8KB_LOW_START) + 1;
		flash_ra_layout[0].pages_size = FLASH_HP_CF_BLOCK_8KB_SIZE;

		flash_ra_layout[1].pages_count = (FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_END -
						  FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_START) +
						 1;
		flash_ra_layout[1].pages_size = FLASH_HP_CF_BLOCK_32KB_SIZE;

		flash_ra_layout[2].pages_count = FLASH_RESERVED_AREA_NUM;
		flash_ra_layout[2].pages_size =
			(FLASH_HP_BANK2_OFFSET -
			(flash_ra_layout[0].pages_count * flash_ra_layout[0].pages_size) -
			(flash_ra_layout[1].pages_count * flash_ra_layout[1].pages_size))
				/ FLASH_RESERVED_AREA_NUM;

		flash_ra_layout[3].pages_count =
			(FLASH_HP_CF_BLOCK_8KB_HIGH_END - FLASH_HP_CF_BLOCK_8KB_HIGH_START) + 1;
		flash_ra_layout[3].pages_size = FLASH_HP_CF_BLOCK_8KB_SIZE;

		/* The final block is the dummy block */
		flash_ra_layout[4].pages_count = (FLASH_HP_CF_BLOCK_32KB_DUAL_HIGH_END + 1 -
						  FLASH_HP_CF_BLOCK_32KB_DUAL_HIGH_START) +
						 1;
		flash_ra_layout[4].pages_size = FLASH_HP_CF_BLOCK_32KB_SIZE;

		*layout_size = 5;
#else
		flash_ra_layout[0].pages_count =
			(FLASH_HP_CF_BLOCK_8KB_LOW_END - FLASH_HP_CF_BLOCK_8KB_LOW_START) + 1;
		flash_ra_layout[0].pages_size = FLASH_HP_CF_BLOCK_8KB_SIZE;
		flash_ra_layout[1].pages_count =
		(FLASH_HP_CF_BLOCK_32KB_LINEAR_END - FLASH_HP_CF_BLOCK_32KB_LINEAR_START) + 1;
		flash_ra_layout[1].pages_size = FLASH_HP_CF_BLOCK_32KB_SIZE;

		*layout_size = 2;
#endif
	*layout = flash_ra_layout;
}

static const struct flash_parameters *flash_ra_get_parameters(const struct device *dev)
{
	const struct flash_hp_ra_config *config = dev->config;

	return &config->flash_ra_parameters;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_ra_ex_op(const struct device *dev, uint16_t code, const uintptr_t in, void *out)
{
	int err = -ENOTSUP;

	switch (code) {
#if defined(CONFIG_FLASH_RA_WRITE_PROTECT)
	case FLASH_RA_EX_OP_WRITE_PROTECT:
		err = flash_ra_ex_op_write_protect(dev, in, out);
		break;
#endif /* CONFIG_FLASH_RA_WRITE_PROTECT */

	default:
		break;
	}

	return err;
}
#endif

DEVICE_DT_INST_DEFINE(0, flash_ra_init, NULL, &flash_hp_ra_data, &flash_hp_ra_config, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_ra_api);
