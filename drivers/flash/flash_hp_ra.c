/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <string.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include "flash_hp_ra.h"

#define DT_DRV_COMPAT renesas_ra_flash_hp_controller

LOG_MODULE_REGISTER(flash_hp_ra, CONFIG_FLASH_LOG_LEVEL);

#define ERASE_BLOCK_SIZE_0 DT_PROP(DT_INST(0, renesas_ra_nv_flash), erase_block_size)
#define ERASE_BLOCK_SIZE_1 DT_PROP(DT_INST(1, renesas_ra_nv_flash), erase_block_size)

BUILD_ASSERT((ERASE_BLOCK_SIZE_0 % FLASH_HP_CF_BLOCK_8KB_SIZE) == 0,
	     "erase-block-size expected to be a multiple of a block size");
BUILD_ASSERT((ERASE_BLOCK_SIZE_1 % FLASH_HP_DF_BLOCK_SIZE) == 0,
	     "erase-block-size expected to be a multiple of a block size");

/* Flags, set from Callback function */
static volatile struct event_flash g_event_flash = {
	.erase_complete = false,
	.write_complete = false,
};

void fcu_frdyi_isr(void);
void fcu_fiferr_isr(void);

static int flash_controller_ra_init(const struct device *dev);
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

static struct flash_pages_layout flash_ra_layout[5];

void bgo_callback(flash_callback_args_t *p_args)
{
	if (FLASH_EVENT_ERASE_COMPLETE == p_args->event) {
		g_event_flash.erase_complete = true;
	} else {
		g_event_flash.write_complete = true;
	}
}

static bool flash_ra_valid_range(off_t area_size, off_t offset, size_t len)
{
	if ((offset < 0) || (offset >= area_size) || ((area_size - offset) < len)) {
		return false;
	}

	return true;
}

static int flash_ra_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_hp_ra_data *flash_data = dev->data;

	if (!flash_ra_valid_range(flash_data->area_size, offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("flash: read 0x%lx, len: %u", (long)(offset + flash_data->area_address), len);

	memcpy(data, (uint8_t *)(offset + flash_data->area_address), len);

	return 0;
}

static int flash_ra_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_hp_ra_data *flash_data = dev->data;
	struct flash_hp_ra_controller *dev_ctrl = flash_data->controller;
	static struct flash_pages_info page_info_off, page_info_len;
	fsp_err_t err = FSP_ERR_ASSERTION;
	uint32_t block_num;
	int rc, rc2;

	if (!flash_ra_valid_range(flash_data->area_size, offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("flash: erase 0x%lx, len: %u", (long)(offset + flash_data->area_address), len);

	rc = flash_get_page_info_by_offs(dev, offset, &page_info_off);

	if (rc != 0) {
		return -EINVAL;
	}

	if (offset != page_info_off.start_offset) {
		return -EINVAL;
	}

	rc2 = flash_get_page_info_by_offs(dev, (offset + len), &page_info_len);
	if (rc2 != 0) {
		return -EINVAL;
	}

#if defined(CONFIG_DUAL_BANK_MODE)
	/* Invalid offset in dual bank mode, this is reversed area. */
	if ((page_info_off.index > FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_END &&
	     page_info_off.index < FLASH_HP_CF_BLOCK_8KB_HIGH_START) ||
	    (page_info_len.index > FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_END &&
	     page_info_len.index < FLASH_HP_CF_BLOCK_8KB_HIGH_START)) {
		return -EIO;
	}
#endif

	if ((offset + len) != (page_info_len.start_offset)) {
		return -EIO;
	}

	block_num = (uint32_t)((page_info_len.index) - page_info_off.index);

	if (block_num > 0) {
		k_sem_take(&dev_ctrl->ctrl_sem, K_FOREVER);

		err = R_FLASH_HP_Erase(&dev_ctrl->flash_ctrl,
				       (long)(flash_data->area_address + offset), block_num);

		if (err != FSP_SUCCESS) {
			k_sem_give(&dev_ctrl->ctrl_sem);
			return -EIO;
		}

		if (flash_data->FlashRegion == DATA_FLASH) {
			/* Wait for the erase complete event flag, if BGO is SET  */
			if (true == dev_ctrl->fsp_config.data_flash_bgo) {
				while (!g_event_flash.erase_complete) {
					k_sleep(K_USEC(10));
				}
				g_event_flash.erase_complete = false;
			}
		}

		k_sem_give(&dev_ctrl->ctrl_sem);
	}

	return 0;
}

static int flash_ra_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	fsp_err_t err = FSP_ERR_ASSERTION;
	struct flash_hp_ra_data *flash_data = dev->data;
	struct flash_hp_ra_controller *dev_ctrl = flash_data->controller;

	if (!flash_ra_valid_range(flash_data->area_size, offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("flash: write 0x%lx, len: %u", (long)(offset + flash_data->area_address), len);

	k_sem_take(&dev_ctrl->ctrl_sem, K_FOREVER);

	err = R_FLASH_HP_Write(&dev_ctrl->flash_ctrl, (uint32_t)data,
			       (long)(offset + flash_data->area_address), len);

	if (err != FSP_SUCCESS) {
		k_sem_give(&dev_ctrl->ctrl_sem);
		return -EIO;
	}

	if (flash_data->FlashRegion == DATA_FLASH) {
		/* Wait for the write complete event flag, if BGO is SET  */
		if (true == dev_ctrl->fsp_config.data_flash_bgo) {
			while (!g_event_flash.write_complete) {
				k_sleep(K_USEC(10));
			}
			g_event_flash.write_complete = false;
		}
	}

	k_sem_give(&dev_ctrl->ctrl_sem);

	return 0;
}

void flash_ra_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			  size_t *layout_size)
{
	struct flash_hp_ra_data *flash_data = dev->data;

	if (flash_data->FlashRegion == DATA_FLASH) {
		flash_ra_layout[0].pages_count = flash_data->area_size / FLASH_HP_DF_BLOCK_SIZE;
		flash_ra_layout[0].pages_size = FLASH_HP_DF_BLOCK_SIZE;

		*layout_size = 1;
	} else {
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
			 (flash_ra_layout[1].pages_count * flash_ra_layout[1].pages_size)) /
			FLASH_RESERVED_AREA_NUM;

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
			(FLASH_HP_CF_BLOCK_32KB_LINEAR_END - FLASH_HP_CF_BLOCK_32KB_LINEAR_START) +
			1;
		flash_ra_layout[1].pages_size = FLASH_HP_CF_BLOCK_32KB_SIZE;

		*layout_size = 2;
#endif
	}

	*layout = flash_ra_layout;
}

static const struct flash_parameters *flash_ra_get_parameters(const struct device *dev)
{
	const struct flash_hp_ra_config *config = dev->config;

	return &config->flash_ra_parameters;
}

static struct flash_hp_ra_controller flash_hp_ra_controller = {
	.fsp_config = {
		.data_flash_bgo = true,
		.p_callback = bgo_callback,
		.p_context = NULL,
		.irq = (IRQn_Type)DT_INST_IRQ_BY_NAME(0, frdyi, irq),
		.err_irq = (IRQn_Type)DT_INST_IRQ_BY_NAME(0, fiferr, irq),
		.err_ipl = DT_INST_IRQ_BY_NAME(0, fiferr, priority),
		.ipl = DT_INST_IRQ_BY_NAME(0, frdyi, priority),
	}};

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

static int flash_ra_init(const struct device *dev)
{
	const struct device *dev_ctrl = DEVICE_DT_INST_GET(0);
	struct flash_hp_ra_data *flash_data = dev->data;

	if (!device_is_ready(dev_ctrl)) {
		return -ENODEV;
	}

	if (flash_data->area_address == FLASH_HP_DF_START) {
		flash_data->FlashRegion = DATA_FLASH;
	} else {
		flash_data->FlashRegion = CODE_FLASH;
	}

	flash_data->controller = dev_ctrl->data;

	return 0;
}

static void flash_controller_ra_irq_config_func(const struct device *dev)
{
	ARG_UNUSED(dev);

	R_ICU->IELSR[DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, irq)] = ELC_EVENT_FCU_FRDYI;
	R_ICU->IELSR[DT_IRQ_BY_NAME(DT_DRV_INST(0), fiferr, irq)] = ELC_EVENT_FCU_FIFERR;

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, priority), fcu_frdyi_isr,
		    DEVICE_DT_INST_GET(0), 0);
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), fiferr, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), fiferr, priority), fcu_fiferr_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQ_BY_NAME(0, frdyi, irq));
	irq_enable(DT_INST_IRQ_BY_NAME(0, fiferr, irq));
}

static int flash_controller_ra_init(const struct device *dev)
{
	fsp_err_t err = FSP_SUCCESS;
	const struct flash_hp_ra_controller_config *cfg = dev->config;
	struct flash_hp_ra_controller *data = dev->data;

	cfg->irq_config(dev);

	err = R_FLASH_HP_Open(&data->flash_ctrl, &data->fsp_config);

	if (err != FSP_SUCCESS) {
		LOG_DBG("flash: open error=%d", (int)err);
		return -EIO;
	}

	k_sem_init(&data->ctrl_sem, 1, 1);

	return 0;
}

static struct flash_hp_ra_controller_config flash_hp_ra_controller_config = {
	.irq_config = flash_controller_ra_irq_config_func,
};

#define RA_FLASH_INIT(index)                                                                       \
	struct flash_hp_ra_data flash_hp_ra_data_##index = {.area_address = DT_REG_ADDR(index),    \
							    .area_size = DT_REG_SIZE(index)};      \
	static struct flash_hp_ra_config flash_hp_ra_config_##index = {                            \
		.flash_ra_parameters = {                                                           \
			.write_block_size = GET_SIZE(                                              \
				(CHECK_EQ(DT_REG_ADDR(index), FLASH_HP_DF_START)), 4, 128),        \
			.erase_value = 0xff,                                                       \
		}};                                                                                \
                                                                                                   \
	DEVICE_DT_DEFINE(index, flash_ra_init, NULL, &flash_hp_ra_data_##index,                    \
			 &flash_hp_ra_config_##index, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,     \
			 &flash_ra_api);

DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), RA_FLASH_INIT);

/* define the flash controller device just to run the init. */
DEVICE_DT_DEFINE(DT_DRV_INST(0), flash_controller_ra_init, NULL, &flash_hp_ra_controller,
		 &flash_hp_ra_controller_config, PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);
