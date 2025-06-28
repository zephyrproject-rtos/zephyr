/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <driverlib/dl_flashctl.h>

LOG_MODULE_REGISTER(flash_ti_mspm0, CONFIG_FLASH_LOG_LEVEL);

#define FLASH_SIZE		(CONFIG_FLASH_SIZE * 1024)
#define FLASH_PAGE_SIZE		(CONFIG_SOC_FLASH_TI_MSPM0_LAYOUT_PAGE_SIZE)
#define FLASH_MSPM0_BASE_ADDRESS DT_REG_ADDR(DT_INST(0, soc_nv_flash))

#if DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#define FLASH_MSPM0_WRITE_BLOCK_SIZE \
	DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#else
#error Flash write block size not available
#endif

#define MSPM0_FLASH_SIZE		(FLASH_SIZE)
#define MSPM0_FLASH_PAGE_SIZE		(FLASH_PAGE_SIZE)

struct flash_ti_mspm0_config {
	FLASHCTL_Regs *regs;
	const struct flash_parameters parameters;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout flash_layout;
#endif
};

struct flash_ti_mspm0_data {
#if defined(CONFIG_MULTITHREADING)
	struct k_sem lock;
#endif
};

static inline void flash_ti_mspm0_lock(const struct device *dev)
{
#if defined(CONFIG_MULTITHREADING)
	struct flash_ti_mspm0_data *mdata = dev->data;

	k_sem_take(&mdata->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void flash_ti_mspm0_unlock(const struct device *dev)
{
#if defined(CONFIG_MULTITHREADING)
	struct flash_ti_mspm0_data *mdata = dev->data;

	k_sem_give(&mdata->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static inline int flash_ti_mspm0_valid_range(off_t offset, size_t len)
{
	if (offset < 0) {
		return -EINVAL;
	}

	if ((offset + len) > CONFIG_FLASH_SIZE * 1024) {
		return -EINVAL;
	}

	return 0;
}

static int flash_ti_mspm0_erase(const struct device *dev,
				off_t offset, size_t len)
{
	int ret = 0;
	size_t iter;
	const struct flash_ti_mspm0_config *cfg = dev->config;

	if (flash_ti_mspm0_valid_range(offset, len)) {
		LOG_ERR("Erase range invalid. Offset %ld, len: %zu",
				(long int) offset, len);
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	if (len < FLASH_PAGE_SIZE) {
		LOG_ERR("Erase must be done in page length manner");
		return -EINVAL;
	}

	iter = len / FLASH_PAGE_SIZE;
	flash_ti_mspm0_lock(dev);

	do {
		DL_FlashCTL_unprotectSector(cfg->regs, offset,
					    DL_FLASHCTL_REGION_SELECT_MAIN);
		DL_FlashCTL_eraseMemory(cfg->regs, offset,
					DL_FLASHCTL_COMMAND_SIZE_SECTOR);

		if (!DL_FlashCTL_waitForCmdDone(cfg->regs)) {
			ret = -EIO;
			goto out;
		}

		offset = offset + FLASH_PAGE_SIZE;
		iter--;
	} while (iter != 0);

out:
	flash_ti_mspm0_unlock(dev);

	return ret;
}

static int flash_ti_mspm0_write(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	int ret = 0;
	size_t iter;
	size_t remain;
	uint32_t *write_data = (uint32_t *)data;
	const struct flash_ti_mspm0_config *cfg = dev->config;

	if (flash_ti_mspm0_valid_range(offset, len)) {
		LOG_ERR("Erase range invalid. Offset %ld, len: %zu",
				(long int) offset, len);
		return -EINVAL;
	}

	if (offset % FLASH_MSPM0_WRITE_BLOCK_SIZE) {
		LOG_DBG("offset must be 8-byte aligned");
		return -EINVAL;
	}

	flash_ti_mspm0_lock(dev);
	iter = len / FLASH_MSPM0_WRITE_BLOCK_SIZE;
	remain = len % FLASH_MSPM0_WRITE_BLOCK_SIZE;

	for (int i = 0; i < iter; i++) {
		DL_FlashCTL_unprotectSector(cfg->regs, offset,
					    DL_FLASHCTL_REGION_SELECT_MAIN);
		DL_FlashCTL_programMemory64WithECCGenerated(cfg->regs, offset,
							    &write_data[i * 2]);

		if (!DL_FlashCTL_waitForCmdDone(cfg->regs)) {
			ret = -EIO;
			goto out;
		}

		offset = offset + FLASH_MSPM0_WRITE_BLOCK_SIZE;
	}

	if (remain) {
		DL_FlashCTL_unprotectSector(cfg->regs, offset,
					    DL_FLASHCTL_REGION_SELECT_MAIN);
		DL_FlashCTL_programMemory64WithECCGenerated(cfg->regs, offset,
							    &write_data[iter * 2]);

		if (!DL_FlashCTL_waitForCmdDone(cfg->regs)) {
			ret = -EIO;
		}
	}

out:
	flash_ti_mspm0_unlock(dev);

	return ret;
}

static int flash_ti_mspm0_read(const struct device *dev, off_t offset,
			       void *data, size_t len)
{
	ARG_UNUSED(dev);
	if (flash_ti_mspm0_valid_range(offset, len)) {
		LOG_ERR("Read range invalid. Offset %ld, len %zu",
				(long int) offset, len);
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	LOG_DBG("Read offset: %ld, len %zu", (long int) offset, len);
	memcpy((uint8_t *) data, (uint8_t *) (FLASH_MSPM0_BASE_ADDRESS + offset), len);

	return 0;
}

static const struct flash_parameters
		*flash_ti_mspm0_get_parameters(const struct device *dev)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;

	return &cfg->parameters;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_ti_mspm0_page_layout(const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;

	*layout = &cfg->flash_layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, flash_ti_mspm0_driver_api) = {
	.erase		= flash_ti_mspm0_erase,
	.write		= flash_ti_mspm0_write,
	.read		= flash_ti_mspm0_read,
	.get_parameters	= flash_ti_mspm0_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout	= flash_ti_mspm0_page_layout,
#endif
};

static int flash_ti_mspm0_init(const struct device *dev)
{
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	const struct flash_ti_mspm0_config *cfg = dev->config;
	const struct flash_pages_layout *layout = &cfg->flash_layout;

	LOG_DBG("Block %zu: bs: %zu count: %zu", 1,
			layout->pages_size, layout->pages_count);
#else
	ARG_UNUSED(dev);
#endif

	return 0;
}

#define FLASH_TI_MSPM0_DEVICE_INIT(inst)					    \
	static const struct flash_ti_mspm0_config flash_ti_mspm0_cfg##inst = {	    \
		.regs = (FLASHCTL_Regs *)DT_INST_REG_ADDR(inst),		    \
		.parameters = {							    \
			.write_block_size = FLASH_MSPM0_WRITE_BLOCK_SIZE,	    \
			.erase_value = 0xFF,					    \
		},								    \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,				    \
		(.flash_layout =  {						    \
			.pages_count = MSPM0_FLASH_SIZE / MSPM0_FLASH_PAGE_SIZE,    \
			.pages_size = MSPM0_FLASH_PAGE_SIZE,			    \
		},))								    \
	};									    \
										    \
	static struct flash_ti_mspm0_data flash_ti_mspm0_data##inst = {		    \
	IF_ENABLED(CONFIG_MULTITHREADING,					    \
		(.lock = Z_SEM_INITIALIZER(flash_ti_mspm0_data##inst.lock, 1, 1),)) \
	};									    \
										    \
DEVICE_DT_INST_DEFINE(inst, flash_ti_mspm0_init, NULL,				    \
		      &flash_ti_mspm0_data##inst, &flash_ti_mspm0_cfg##inst,	    \
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,			    \
		      &flash_ti_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_TI_MSPM0_DEVICE_INIT);
