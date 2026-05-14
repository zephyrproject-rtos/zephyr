/*
 * Copyright (c) 2025-2026 Renesas Electronics Corporation
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
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/atomic.h>
#include <r_flash_lp.h>
#include <r_flash_api.h>

LOG_MODULE_REGISTER(flash_renesas_ra_lp, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_ra_flash_lp_controller

/* Maximum number of page layout entries per flash region */
#define FLASH_LP_CF_LAYOUT_SIZE 1U
#define FLASH_LP_DF_LAYOUT_SIZE 1U

enum flash_region {
	CODE_FLASH,
	DATA_FLASH,
};

#if defined(CONFIG_FLASH_RENESAS_RA_LP_BGO)
#define FLASH_FLAG_ERASE_COMPLETE BIT(0)
#define FLASH_FLAG_WRITE_COMPLETE BIT(1)
#define FLASH_FLAG_GET_ERROR      BIT(2)
#endif /* CONFIG_FLASH_RENESAS_RA_LP_BGO */

struct flash_lp_ra_controller {
	struct st_flash_lp_instance_ctrl flash_ctrl;
	struct k_sem ctrl_sem;
	struct st_flash_cfg fsp_config;
	atomic_t flags;
};

struct flash_lp_ra_data {
	struct flash_lp_ra_controller *controller;
	enum flash_region flash_region;
	uint32_t area_address;
	uint32_t area_size;
};

struct flash_lp_ra_config {
	struct flash_parameters flash_ra_parameters;
};

static struct flash_pages_layout code_flash_ra_layout[1];
static struct flash_pages_layout data_flash_ra_layout[1];

#if defined(CONFIG_FLASH_RENESAS_RA_LP_BGO)
void fcu_frdyi_isr(void);

void flash_bgo_callback(flash_callback_args_t *p_args)
{
	atomic_t *event_flag = (atomic_t *)(p_args->p_context);

	if (FLASH_EVENT_ERASE_COMPLETE == p_args->event) {
		atomic_or(event_flag, FLASH_FLAG_ERASE_COMPLETE);
	} else if (FLASH_EVENT_WRITE_COMPLETE == p_args->event) {
		atomic_or(event_flag, FLASH_FLAG_WRITE_COMPLETE);
	} else {
		atomic_or(event_flag, FLASH_FLAG_GET_ERROR);
	}
}
#endif /* CONFIG_FLASH_RENESAS_RA_LP_BGO */

static bool flash_ra_valid_range(off_t area_size, off_t offset, size_t len)
{
	if ((offset < 0) || offset >= area_size || (area_size - offset) < len ||
	    (len > UINT32_MAX - offset)) {
		return false;
	}

	return true;
}

static int flash_ra_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_lp_ra_data *flash_data = dev->data;

	if (!len) {
		return 0;
	}

	if (!flash_ra_valid_range(flash_data->area_size, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("flash: read 0x%lx, len: %u", (long)(offset + flash_data->area_address), len);

	memcpy(data, (uint8_t *)(offset + flash_data->area_address), len);

	return 0;
}

static int flash_ra_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_lp_ra_data *flash_data = dev->data;
	struct flash_lp_ra_controller *dev_ctrl = flash_data->controller;
	static struct flash_pages_info page_info_off, page_info_len;
	fsp_err_t err;
	uint32_t block_num;
	int rc, rc2, ret = 0;
	int key = 0;
	bool is_contain_end_block = false;

	if (!len) {
		return 0;
	}

	if (!flash_ra_valid_range(flash_data->area_size, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("flash: erase 0x%lx, len: %u", (long)(offset + flash_data->area_address), len);

	rc = flash_get_page_info_by_offs(dev, offset, &page_info_off);

	if (rc != 0) {
		return -EINVAL;
	}

	if (offset != page_info_off.start_offset) {
		return -EINVAL;
	}

	if ((offset + len) == flash_data->area_size) {
		flash_info_t info;
		flash_regions_t *regions;
		uint32_t total_blocks = 0;

		err = R_FLASH_LP_InfoGet(&dev_ctrl->flash_ctrl, &info);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
		regions = (flash_data->flash_region == CODE_FLASH) ? &info.code_flash
								   : &info.data_flash;

		for (uint32_t i = 0; i < regions->num_regions; i++) {
			total_blocks += (regions->p_block_array[i].block_section_end_addr -
					 regions->p_block_array[i].block_section_st_addr + 1) /
					regions->p_block_array[i].block_size;
		}
		page_info_len.index = total_blocks;
		is_contain_end_block = true;
	}

	if (!is_contain_end_block) {
		rc2 = flash_get_page_info_by_offs(dev, (offset + len), &page_info_len);
		if (rc2 != 0) {
			return -EINVAL;
		}
		if ((offset + len) != (page_info_len.start_offset)) {
			return -EIO;
		}
	}

	block_num = (uint32_t)((page_info_len.index) - page_info_off.index);

	if (block_num > 0) {
		if (flash_data->flash_region == CODE_FLASH) {
			/* Disable interrupts during code flash operations */
			key = irq_lock();
		} else {
			k_sem_take(&dev_ctrl->ctrl_sem, K_FOREVER);
		}

		err = R_FLASH_LP_Erase(&dev_ctrl->flash_ctrl,
				       (long)(flash_data->area_address + offset), block_num);

		if (err != FSP_SUCCESS) {
			ret = -EIO;
			goto end;
		}

#if defined(CONFIG_FLASH_RENESAS_RA_LP_BGO)
		if (flash_data->flash_region == DATA_FLASH) {
			/* Wait for the erase complete event flag, if BGO is SET */
			while (!(dev_ctrl->flags & FLASH_FLAG_ERASE_COMPLETE)) {
				if (dev_ctrl->flags & FLASH_FLAG_GET_ERROR) {
					ret = -EIO;
					atomic_and(&dev_ctrl->flags, ~FLASH_FLAG_GET_ERROR);
					break;
				}
				k_sleep(K_USEC(10));
			}
			atomic_and(&dev_ctrl->flags, ~FLASH_FLAG_ERASE_COMPLETE);
		}
#endif /* CONFIG_FLASH_RENESAS_RA_LP_BGO */

end:
		if (flash_data->flash_region == CODE_FLASH) {
			irq_unlock(key);
		} else {
			k_sem_give(&dev_ctrl->ctrl_sem);
		}
	}

	return ret;
}

static int flash_ra_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	fsp_err_t err;
	struct flash_lp_ra_data *flash_data = dev->data;
	struct flash_lp_ra_controller *dev_ctrl = flash_data->controller;
	int key = 0;
	int ret = 0;

	if (!len) {
		return 0;
	}

	if (!flash_ra_valid_range(flash_data->area_size, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("flash: write 0x%lx, len: %u", (long)(offset + flash_data->area_address), len);

	if (flash_data->flash_region == CODE_FLASH) {
		/* Disable interrupts during code flash operations */
		key = irq_lock();
	} else {
		k_sem_take(&dev_ctrl->ctrl_sem, K_FOREVER);
	}

	err = R_FLASH_LP_Write(&dev_ctrl->flash_ctrl, (uint32_t)data,
			       (long)(offset + flash_data->area_address), len);

	if (err != FSP_SUCCESS) {
		ret = -EIO;
		goto end;
	}

#if defined(CONFIG_FLASH_RENESAS_RA_LP_BGO)
	if (flash_data->flash_region == DATA_FLASH) {
		/* Wait for the write complete event flag, if BGO is SET  */
		while (!(dev_ctrl->flags & FLASH_FLAG_WRITE_COMPLETE)) {
			if (dev_ctrl->flags & FLASH_FLAG_GET_ERROR) {
				ret = -EIO;
				atomic_and(&dev_ctrl->flags, ~FLASH_FLAG_GET_ERROR);
				break;
			}
			k_sleep(K_USEC(10));
		}
		atomic_and(&dev_ctrl->flags, ~FLASH_FLAG_WRITE_COMPLETE);
	}
#endif /* CONFIG_FLASH_RENESAS_RA_LP_BGO */

end:
	if (flash_data->flash_region == CODE_FLASH) {
		irq_unlock(key);
	} else {
		k_sem_give(&dev_ctrl->ctrl_sem);
	}

	return ret;
}

static int flash_ra_get_size(const struct device *dev, uint64_t *size)
{
	struct flash_lp_ra_data *flash_data = dev->data;
	*size = (uint64_t)flash_data->area_size;

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_ra_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			  size_t *layout_size)
{
	struct flash_lp_ra_data *flash_data = dev->data;
	struct flash_lp_ra_controller *dev_ctrl = flash_data->controller;
	flash_info_t info;
	flash_regions_t *regions;
	struct flash_pages_layout *out_layout;
	fsp_err_t err;

	err = R_FLASH_LP_InfoGet(&dev_ctrl->flash_ctrl, &info);
	if (err != FSP_SUCCESS) {
		*layout_size = 0;
		return;
	}

	if (flash_data->flash_region == DATA_FLASH) {
		regions = &info.data_flash;
		out_layout = data_flash_ra_layout;
	} else {
		regions = &info.code_flash;
		out_layout = code_flash_ra_layout;
	}

	for (uint32_t i = 0; i < regions->num_regions; i++) {
		out_layout[i].pages_size = regions->p_block_array[i].block_size;
		out_layout[i].pages_count = (regions->p_block_array[i].block_section_end_addr -
					     regions->p_block_array[i].block_section_st_addr + 1) /
					    regions->p_block_array[i].block_size;
	}

	*layout = out_layout;
	*layout_size = regions->num_regions;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_ra_get_parameters(const struct device *dev)
{
	const struct flash_lp_ra_config *config = dev->config;

	return &config->flash_ra_parameters;
}

static struct flash_lp_ra_controller flash_lp_ra_controller = {
	.fsp_config = {
		.data_flash_bgo = IS_ENABLED(CONFIG_FLASH_RENESAS_RA_LP_BGO),
#if defined(CONFIG_FLASH_RENESAS_RA_LP_BGO)
		.p_callback = flash_bgo_callback,
		.p_context = NULL,
		.irq = (IRQn_Type)DT_INST_IRQ_BY_NAME(0, frdyi, irq),
		.ipl = DT_INST_IRQ_BY_NAME(0, frdyi, priority),
#endif /* CONFIG_FLASH_RENESAS_RA_LP_BGO */
	}};

static int flash_ra_init(const struct device *dev)
{
	const struct device *dev_ctrl = DEVICE_DT_INST_GET(0);
	struct flash_lp_ra_data *flash_data = dev->data;
	flash_info_t info;
	fsp_err_t err;

	if (!device_is_ready(dev_ctrl)) {
		return -ENODEV;
	}

	flash_data->controller = dev_ctrl->data;

	err = R_FLASH_LP_InfoGet(&flash_data->controller->flash_ctrl, &info);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	flash_data->flash_region =
		(flash_data->area_address == info.data_flash.p_block_array[0].block_section_st_addr)
			? DATA_FLASH
			: CODE_FLASH;

	return 0;
}

#ifdef CONFIG_SOC_RA_DYNAMIC_INTERRUPT_NUMBER
#define FLASH_ASSIGN_DYNAMIC_INTERRUPT_NUMBER                                                      \
	R_ICU->IELSR[DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, irq)] =                                 \
		BSP_PRV_IELS_ENUM(EVENT_FCU_FRDYI);
#else
#define FLASH_ASSIGN_DYNAMIC_INTERRUPT_NUMBER
#endif /* CONFIG_SOC_RA_DYNAMIC_INTERRUPT_NUMBER */

#if defined(CONFIG_FLASH_RENESAS_RA_LP_BGO)
#define FLASH_CONTROLLER_RA_IRQ_INIT                                                               \
	{                                                                                          \
		FLASH_ASSIGN_DYNAMIC_INTERRUPT_NUMBER                                              \
                                                                                                   \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, irq),                            \
			    DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, priority), fcu_frdyi_isr,        \
			    DEVICE_DT_INST_GET(0), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(0, frdyi, irq));                                    \
	}
#endif /* CONFIG_FLASH_RENESAS_RA_LP_BGO */

static int flash_controller_ra_init(const struct device *dev)
{
	fsp_err_t err;
	struct flash_lp_ra_controller *data = dev->data;

#if defined(CONFIG_FLASH_RENESAS_RA_LP_BGO)
	FLASH_CONTROLLER_RA_IRQ_INIT
#endif /* CONFIG_FLASH_RENESAS_RA_LP_BGO */

	k_sem_init(&data->ctrl_sem, 1, 1);

	data->fsp_config.p_context = &data->flags;

	err = R_FLASH_LP_Open(&data->flash_ctrl, &data->fsp_config);
	if (err != FSP_SUCCESS) {
		LOG_DBG("flash: open error=%d", (int)err);
		return -EIO;
	}

	return 0;
}

static DEVICE_API(flash, flash_ra_api) = {
	.erase = flash_ra_erase,
	.write = flash_ra_write,
	.read = flash_ra_read,
	.get_parameters = flash_ra_get_parameters,
	.get_size = flash_ra_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_ra_page_layout,
#endif
};

#define RA_FLASH_NV_INIT(nv_node, ctrl_node)                                                       \
	struct flash_lp_ra_data flash_lp_ra_data_##ctrl_node = {                                   \
		.area_address = DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(nv_node, 0),                   \
		.area_size = DT_RANGES_LENGTH_BY_IDX(nv_node, 0),                                  \
	};                                                                                         \
	static struct flash_lp_ra_config flash_lp_ra_config_##ctrl_node = {                        \
		.flash_ra_parameters = {                                                           \
			.write_block_size = DT_PROP(nv_node, write_block_size),                    \
			.erase_value = 0xff,                                                       \
		}};                                                                                \
	DEVICE_DT_DEFINE(ctrl_node, flash_ra_init, NULL, &flash_lp_ra_data_##ctrl_node,            \
			 &flash_lp_ra_config_##ctrl_node, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, \
			 &flash_ra_api);

#define RA_FLASH_INIT(index) DT_FOREACH_CHILD_STATUS_OKAY_VARGS(index, RA_FLASH_NV_INIT, index)

DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), RA_FLASH_INIT);

/* define the flash controller device just to run the init. */
DEVICE_DT_DEFINE(DT_DRV_INST(0), flash_controller_ra_init, NULL, &flash_lp_ra_controller, NULL,
		 PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);
