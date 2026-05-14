/*
 * Copyright (c) 2024-2026 Renesas Electronics Corporation
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
#include <r_flash_hp.h>
#include <r_flash_api.h>

#define DT_DRV_COMPAT renesas_ra_flash_hp_controller

LOG_MODULE_REGISTER(flash_renesas_ra_hp, CONFIG_FLASH_LOG_LEVEL);

/* Maximum number of page layout entries per flash region (code flash: 2, data flash: up to 2) */
#define FLASH_HP_CF_LAYOUT_SIZE 2U
#define FLASH_HP_DF_LAYOUT_SIZE 2U

enum flash_region {
	CODE_FLASH,
	DATA_FLASH,
};

#if defined(CONFIG_FLASH_RENESAS_RA_HP_BGO)
#define FLASH_FLAG_ERASE_COMPLETE BIT(0)
#define FLASH_FLAG_WRITE_COMPLETE BIT(1)
#define FLASH_FLAG_GET_ERROR      BIT(2)

#if defined(CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING)
#define FLASH_FLAG_BLANK     BIT(3)
#define FLASH_FLAG_NOT_BLANK BIT(4)
#endif /* CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING */

#endif /* CONFIG_FLASH_RENESAS_RA_HP_BGO */

struct flash_hp_ra_controller {
	struct st_flash_hp_instance_ctrl flash_ctrl;
	struct k_sem ctrl_sem;
	struct st_flash_cfg fsp_config;
	atomic_t flags;
};

struct flash_hp_ra_data {
	struct flash_hp_ra_controller *controller;
	enum flash_region flash_region;
	uint32_t area_address;
	uint32_t area_size;
};

struct flash_hp_ra_config {
	struct flash_parameters flash_ra_parameters;
};

static struct flash_pages_layout code_flash_ra_layout[FLASH_HP_CF_LAYOUT_SIZE];
static struct flash_pages_layout data_flash_ra_layout[FLASH_HP_DF_LAYOUT_SIZE];

#if defined(CONFIG_FLASH_RENESAS_RA_HP_BGO)
void fcu_frdyi_isr(void);
void fcu_fiferr_isr(void);

void flash_bgo_callback(flash_callback_args_t *p_args)
{
	atomic_t *event_flag = (atomic_t *)(p_args->p_context);

	if (FLASH_EVENT_ERASE_COMPLETE == p_args->event) {
		atomic_or(event_flag, FLASH_FLAG_ERASE_COMPLETE);
	} else if (FLASH_EVENT_WRITE_COMPLETE == p_args->event) {
		atomic_or(event_flag, FLASH_FLAG_WRITE_COMPLETE);
	}
#if defined(CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING)
	else if (FLASH_EVENT_BLANK == p_args->event) {
		atomic_or(event_flag, FLASH_FLAG_BLANK);
	} else if (FLASH_EVENT_NOT_BLANK == p_args->event) {
		atomic_or(event_flag, FLASH_FLAG_NOT_BLANK);
	}
#endif /* CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING */
	else {
		atomic_or(event_flag, FLASH_FLAG_GET_ERROR);
	}
}
#endif /* CONFIG_FLASH_RENESAS_RA_HP_BGO */

static bool flash_ra_valid_range(struct flash_hp_ra_data *flash_data, off_t offset, size_t len)
{
	if ((offset < 0) || (offset >= flash_data->area_size) ||
	    (flash_data->area_size - offset < len) || (len > UINT32_MAX - offset)) {
		return false;
	}

	return true;
}

#if defined(CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING)
/* This feature prevents erroneous reading. Values read from an
 * area of the data flash that has been erased but not programmed
 * are undefined.
 */
static int is_area_readable(const struct device *dev, off_t offset, size_t len)
{
	struct flash_hp_ra_data *flash_data = dev->data;
	struct flash_hp_ra_controller *dev_ctrl = flash_data->controller;
	int ret = 0;
	flash_result_t result = FLASH_RESULT_BGO_ACTIVE;
	fsp_err_t err;

	k_sem_take(&dev_ctrl->ctrl_sem, K_FOREVER);

	err = R_FLASH_HP_BlankCheck(&dev_ctrl->flash_ctrl,
				    (long)(flash_data->area_address + offset), len, &result);
	if (err != FSP_SUCCESS) {
		ret = -EIO;
		goto end;
	}

	/* Wait for the blank check result event if BGO is SET  */
	if (true == dev_ctrl->fsp_config.data_flash_bgo) {
		while (!(dev_ctrl->flags & (FLASH_FLAG_BLANK | FLASH_FLAG_NOT_BLANK))) {
			if (dev_ctrl->flags & FLASH_FLAG_GET_ERROR) {
				ret = -EIO;
				atomic_and(&dev_ctrl->flags, ~FLASH_FLAG_GET_ERROR);
				break;
			}
			k_sleep(K_USEC(10));
		}
		if (dev_ctrl->flags & FLASH_FLAG_BLANK) {
			LOG_DBG("read request on erased offset:0x%lx size:%d", offset, len);
			result = FLASH_RESULT_BLANK;
		}
		atomic_and(&dev_ctrl->flags, ~(FLASH_FLAG_BLANK | FLASH_FLAG_NOT_BLANK));
	}

end:
	k_sem_give(&dev_ctrl->ctrl_sem);

	if (result == FLASH_RESULT_BLANK) {
		return -ENODATA;
	}

	return ret;
}
#endif /* CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING */

static int flash_ra_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_hp_ra_data *flash_data = dev->data;
	int rc = 0;

	if (!flash_ra_valid_range(flash_data, offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("flash: read 0x%lx, len: %u", (long)(offset + flash_data->area_address), len);

#if defined(CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING)
	if (flash_data->flash_region == DATA_FLASH) {
		rc = is_area_readable(dev, offset, len);
	}
#endif /* CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING */

	if (!rc) {
		memcpy(data, (uint8_t *)(offset + flash_data->area_address), len);
#if defined(CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING)
	} else if (rc == -ENODATA) {
		/* Erased area, return dummy data as an erased page. */
		memset(data, 0xFF, len);
		rc = 0;
#endif /* CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING */
	}

	return rc;
}

static int flash_ra_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_hp_ra_data *flash_data = dev->data;
	struct flash_hp_ra_controller *dev_ctrl = flash_data->controller;
	static struct flash_pages_info page_info_off, page_info_len;
	fsp_err_t err;
	uint32_t block_num;
	int rc, rc2, ret = 0;
	int key = 0;
	bool is_contain_end_block = false;

	if (!flash_ra_valid_range(flash_data, offset, len)) {
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

	if ((offset + len) == flash_data->area_size) {
		flash_info_t info;
		flash_regions_t *regions;
		uint32_t total_blocks = 0;

		err = R_FLASH_HP_InfoGet(&dev_ctrl->flash_ctrl, &info);
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

	block_num = (uint32_t)(page_info_len.index - page_info_off.index);

	if (block_num > 0) {
		if (flash_data->flash_region == CODE_FLASH) {
			/* Disable interrupts during code flash operations */
			key = irq_lock();
		} else {
			k_sem_take(&dev_ctrl->ctrl_sem, K_FOREVER);
		}

		err = R_FLASH_HP_Erase(&dev_ctrl->flash_ctrl,
				       (long)(flash_data->area_address + offset), block_num);
		if (err != FSP_SUCCESS) {
			ret = -EIO;
			goto end;
		}

#if defined(CONFIG_FLASH_RENESAS_RA_HP_BGO)
		if (flash_data->flash_region == DATA_FLASH) {
			/* Wait for the erase complete event flag, if BGO is SET  */
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
#endif /* CONFIG_FLASH_RENESAS_RA_HP_BGO */

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
	struct flash_hp_ra_data *flash_data = dev->data;
	struct flash_hp_ra_controller *dev_ctrl = flash_data->controller;
	int key = 0;
	int ret = 0;

	if (!flash_ra_valid_range(flash_data, offset, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("flash: write 0x%lx, len: %u", (long)(offset + flash_data->area_address), len);

	if (flash_data->flash_region == CODE_FLASH) {
		/* Disable interrupts during code flash operations */
		key = irq_lock();
	} else {
		k_sem_take(&dev_ctrl->ctrl_sem, K_FOREVER);
	}

	err = R_FLASH_HP_Write(&dev_ctrl->flash_ctrl, (uint32_t)data,
			       (long)(offset + flash_data->area_address), len);
	if (err != FSP_SUCCESS) {
		ret = -EIO;
		goto end;
	}

#if defined(CONFIG_FLASH_RENESAS_RA_HP_BGO)
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
#endif /* CONFIG_FLASH_RENESAS_RA_HP_BGO */

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
	struct flash_hp_ra_data *flash_data = dev->data;
	*size = (uint64_t)flash_data->area_size;

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_ra_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			  size_t *layout_size)
{
	struct flash_hp_ra_data *flash_data = dev->data;
	struct flash_hp_ra_controller *dev_ctrl = flash_data->controller;
	flash_info_t info;
	flash_regions_t *regions;
	struct flash_pages_layout *ret_layout;
	fsp_err_t err;

	err = R_FLASH_HP_InfoGet(&dev_ctrl->flash_ctrl, &info);
	if (err != FSP_SUCCESS) {
		*layout_size = 0;
		return;
	}

	if (flash_data->flash_region == DATA_FLASH) {
		regions = &info.data_flash;
		ret_layout = data_flash_ra_layout;
	} else {
		regions = &info.code_flash;
		ret_layout = code_flash_ra_layout;
	}

	for (uint32_t i = 0; i < regions->num_regions; i++) {
		ret_layout[i].pages_size = regions->p_block_array[i].block_size;
		ret_layout[i].pages_count = (regions->p_block_array[i].block_section_end_addr -
					     regions->p_block_array[i].block_section_st_addr + 1) /
					    regions->p_block_array[i].block_size;
	}

	*layout = ret_layout;
	*layout_size = regions->num_regions;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_ra_get_parameters(const struct device *dev)
{
	const struct flash_hp_ra_config *config = dev->config;

	return &config->flash_ra_parameters;
}

static struct flash_hp_ra_controller flash_hp_ra_controller = {
	.fsp_config = {
		.data_flash_bgo = IS_ENABLED(CONFIG_FLASH_RENESAS_RA_HP_BGO),
#if defined(CONFIG_FLASH_RENESAS_RA_HP_BGO)
		.p_callback = flash_bgo_callback,
		.p_context = NULL,
		.irq = (IRQn_Type)DT_INST_IRQ_BY_NAME(0, frdyi, irq),
		.err_irq = (IRQn_Type)DT_INST_IRQ_BY_NAME(0, fiferr, irq),
		.err_ipl = DT_INST_IRQ_BY_NAME(0, fiferr, priority),
		.ipl = DT_INST_IRQ_BY_NAME(0, frdyi, priority),
#endif /* CONFIG_FLASH_RENESAS_RA_HP_BGO */
	}};

static int flash_ra_init(const struct device *dev)
{
	const struct device *dev_ctrl = DEVICE_DT_INST_GET(0);
	struct flash_hp_ra_data *flash_data = dev->data;
	flash_info_t info;
	fsp_err_t err;

	if (!device_is_ready(dev_ctrl)) {
		return -ENODEV;
	}

	flash_data->controller = dev_ctrl->data;

	err = R_FLASH_HP_InfoGet(&flash_data->controller->flash_ctrl, &info);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	flash_data->flash_region =
		(flash_data->area_address == info.data_flash.p_block_array[0].block_section_st_addr)
			? DATA_FLASH
			: CODE_FLASH;

	return 0;
}

#if defined(CONFIG_FLASH_RENESAS_RA_HP_BGO)
#define FLASH_CONTROLLER_RA_IRQ_INIT                                                               \
	{                                                                                          \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, irq)] =                         \
			BSP_PRV_IELS_ENUM(EVENT_FCU_FRDYI);                                        \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_DRV_INST(0), fiferr, irq)] =                        \
			BSP_PRV_IELS_ENUM(EVENT_FCU_FIFERR);                                       \
                                                                                                   \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, irq),                            \
			    DT_IRQ_BY_NAME(DT_DRV_INST(0), frdyi, priority), fcu_frdyi_isr,        \
			    DEVICE_DT_INST_GET(0), 0);                                             \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), fiferr, irq),                           \
			    DT_IRQ_BY_NAME(DT_DRV_INST(0), fiferr, priority), fcu_fiferr_isr,      \
			    DEVICE_DT_INST_GET(0), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(0, frdyi, irq));                                    \
		irq_enable(DT_INST_IRQ_BY_NAME(0, fiferr, irq));                                   \
	}
#endif /* CONFIG_FLASH_RENESAS_RA_HP_BGO */

static int flash_controller_ra_init(const struct device *dev)
{
	fsp_err_t err;
	struct flash_hp_ra_controller *data = dev->data;

#if defined(CONFIG_FLASH_RENESAS_RA_HP_BGO)
	FLASH_CONTROLLER_RA_IRQ_INIT
#endif /* CONFIG_FLASH_RENESAS_RA_HP_BGO */

	k_sem_init(&data->ctrl_sem, 1, 1);

	data->fsp_config.p_context = &data->flags;

	err = R_FLASH_HP_Open(&data->flash_ctrl, &data->fsp_config);

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
	struct flash_hp_ra_data flash_hp_ra_data_##ctrl_node = {                                   \
		.area_address = DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(nv_node, 0),                   \
		.area_size = DT_RANGES_LENGTH_BY_IDX(nv_node, 0),                                  \
	};                                                                                         \
	static struct flash_hp_ra_config flash_hp_ra_config_##ctrl_node = {                        \
		.flash_ra_parameters =                                                             \
			{                                                                          \
				.write_block_size = DT_PROP(nv_node, write_block_size),            \
				.erase_value = 0xff,                                               \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_DEFINE(ctrl_node, flash_ra_init, NULL, &flash_hp_ra_data_##ctrl_node,            \
			 &flash_hp_ra_config_##ctrl_node, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, \
			 &flash_ra_api);

#define RA_FLASH_INIT(index) DT_FOREACH_CHILD_STATUS_OKAY_VARGS(index, RA_FLASH_NV_INIT, index)

DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(0), RA_FLASH_INIT);

/* define the flash controller device just to run the init. */
DEVICE_DT_DEFINE(DT_DRV_INST(0), flash_controller_ra_init, NULL, &flash_hp_ra_controller, NULL,
		 PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);
