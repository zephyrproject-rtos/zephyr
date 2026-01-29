/*
 * Copyright (c) 2026 Texas Instruments
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <driverlib/dl_flashctl.h>

LOG_MODULE_REGISTER(flash_ti_mspm0, CONFIG_FLASH_LOG_LEVEL);

#define MSPM0_FLASH_PAGE_SIZE	1024
#define MSPM0_FLASH_SIZE	(CONFIG_FLASH_SIZE * MSPM0_FLASH_PAGE_SIZE)
#define FLASH_MSPM0_BASE_ADDRESS DT_REG_ADDR(DT_INST(0, soc_nv_flash))
#define FLASH_CMDWAIT_TIMEOUT	500

#if DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#define FLASH_MSPM0_WRITE_BLOCK_SIZE \
	DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#else
#error Flash write block size not available
#endif

struct flash_ti_mspm0_config {
	FLASHCTL_Regs *regs;
	void (*irq_config_func)(void);
	const struct flash_parameters parameters;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout flash_layout;
#endif
};

struct flash_ti_mspm0_data {
	struct k_sem wait_sem;
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

static inline bool flash_ti_mspm0_valid_range(off_t offset, size_t len)
{
	return ((offset >= 0) && (offset < MSPM0_FLASH_SIZE) &&
			((MSPM0_FLASH_SIZE - offset) >= len));
}

static int flash_ti_mspm0_erase(const struct device *dev,
				off_t offset, size_t len)
{
	int ret = 0;
	const struct flash_ti_mspm0_config *cfg = dev->config;
	struct flash_ti_mspm0_data *data = dev->data;

	if (len == 0) {
		return 0;
	}

	if (flash_ti_mspm0_valid_range(offset, len) == 0) {
		LOG_ERR("Erase range invalid. Offset %ld, len: %zu",
						offset, len);
		return -EINVAL;
	}

	if (!IS_ALIGNED(offset, MSPM0_FLASH_PAGE_SIZE)) {
		LOG_ERR("Offset must be aligned to flash page size");
		return -EINVAL;
	}

	if (!IS_ALIGNED(len, MSPM0_FLASH_PAGE_SIZE)) {
		LOG_ERR("Erase length must be aligned to flash page size");
		return -EINVAL;
	}

	flash_ti_mspm0_lock(dev);
	k_sem_reset(&data->wait_sem);

	do {
		DL_FlashCTL_unprotectSector(cfg->regs, offset,
					    DL_FLASHCTL_REGION_SELECT_MAIN);
		DL_FlashCTL_eraseMemory(cfg->regs, offset,
					DL_FLASHCTL_COMMAND_SIZE_SECTOR);

		if (k_sem_take(&data->wait_sem, K_MSEC(FLASH_CMDWAIT_TIMEOUT)) < 0) {
			ret = -ETIMEDOUT;
			goto out;
		}

		offset += MSPM0_FLASH_PAGE_SIZE;
		len -= MSPM0_FLASH_PAGE_SIZE;
	} while (len != 0);

out:
	flash_ti_mspm0_unlock(dev);

	return ret;
}

static int flash_ti_mspm0_write(const struct device *dev, off_t offset,
				const void *buf, size_t len)
{
	int ret = 0;
	uint32_t *write_data = (uint32_t *)buf;
	const struct flash_ti_mspm0_config *cfg = dev->config;
	struct flash_ti_mspm0_data *data = dev->data;

	if (len == 0) {
		return 0;
	}

	if (flash_ti_mspm0_valid_range(offset, len) == 0) {
		LOG_ERR("Erase range invalid. Offset %ld, len: %zu",
						offset, len);
		return -EINVAL;
	}

	if (!IS_ALIGNED(offset, FLASH_MSPM0_WRITE_BLOCK_SIZE)) {
		LOG_ERR("Offset must be aligned to write block");
		return -EINVAL;
	}

	if (!IS_ALIGNED(len, FLASH_MSPM0_WRITE_BLOCK_SIZE)) {
		LOG_ERR("Length must be aligned to write block");
		return -EINVAL;
	}

	flash_ti_mspm0_lock(dev);
	k_sem_reset(&data->wait_sem);

	while (len != 0) {
		DL_FlashCTL_unprotectSector(cfg->regs, offset,
					    DL_FLASHCTL_REGION_SELECT_MAIN);

		DL_FlashCTL_programMemory64WithECCGenerated(cfg->regs, offset, write_data);

		if (k_sem_take(&data->wait_sem, K_MSEC(FLASH_CMDWAIT_TIMEOUT)) < 0) {
			ret = -ETIMEDOUT;
			goto out;
		}

		offset += FLASH_MSPM0_WRITE_BLOCK_SIZE;
		len -= FLASH_MSPM0_WRITE_BLOCK_SIZE;
		write_data += 2;
	}

out:
	flash_ti_mspm0_unlock(dev);

	return ret;
}

static int flash_ti_mspm0_read(const struct device *dev, off_t offset,
			       void *data, size_t len)
{
	ARG_UNUSED(dev);

	if (len == 0) {
		return 0;
	}

	if (flash_ti_mspm0_valid_range(offset, len) == 0) {
		LOG_ERR("Read range invalid. Offset %ld, len %zu",
						offset, len);
		return -EINVAL;
	}

	LOG_DBG("Read offset: %ld, len %zu", offset, len);
	memcpy((uint8_t *) data, (uint8_t *) (FLASH_MSPM0_BASE_ADDRESS + offset), len);

	return 0;
}

static const struct flash_parameters
		*flash_ti_mspm0_get_parameters(const struct device *dev)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;

	return &cfg->parameters;
}

static int flash_ti_mspm0_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	if (size == NULL) {
		return -EINVAL;
	}

	*size = (uint64_t)MSPM0_FLASH_SIZE;

	return 0;
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
	.get_size	= flash_ti_mspm0_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout	= flash_ti_mspm0_page_layout,
#endif
};

static void flash_ti_mspm0_isr(const struct device *dev)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;
	struct flash_ti_mspm0_data *data = dev->data;

	if (DL_FlashCTL_getPendingInterrupt(cfg->regs) == DL_FLASHCTL_IIDX_DONE) {
		k_sem_give(&data->wait_sem);
	}
}

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

	cfg->irq_config_func();
	DL_FlashCTL_enableInterrupt(cfg->regs);

	return 0;
}

#define FLASH_TI_MSPM0_DEVICE_INIT(inst)					    \
										    \
	static void flash_ti_mspm0_config_irq_##inst(void)			    \
	{									    \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),	    \
			    flash_ti_mspm0_isr, DEVICE_DT_INST_GET(inst), 0);	    \
		irq_enable(DT_INST_IRQN(inst));					    \
	}									    \
										    \
	static const struct flash_ti_mspm0_config flash_ti_mspm0_cfg##inst = {	    \
		.regs = (FLASHCTL_Regs *)DT_INST_REG_ADDR(inst),		    \
		.parameters = {							    \
			.write_block_size = FLASH_MSPM0_WRITE_BLOCK_SIZE,	    \
			.erase_value = 0xFF,					    \
		},								    \
		.irq_config_func = flash_ti_mspm0_config_irq_##inst,		    \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,				    \
		(.flash_layout =  {						    \
			.pages_count = MSPM0_FLASH_SIZE / MSPM0_FLASH_PAGE_SIZE,    \
			.pages_size = MSPM0_FLASH_PAGE_SIZE,			    \
		},))								    \
	};									    \
										    \
	static struct flash_ti_mspm0_data flash_ti_mspm0_data##inst = {		    \
	.wait_sem = Z_SEM_INITIALIZER(flash_ti_mspm0_data##inst.wait_sem, 0, 1),    \
	IF_ENABLED(CONFIG_MULTITHREADING,					    \
		(.lock = Z_SEM_INITIALIZER(flash_ti_mspm0_data##inst.lock, 1, 1),)) \
	};									    \
										    \
DEVICE_DT_INST_DEFINE(inst, flash_ti_mspm0_init, NULL,				    \
		      &flash_ti_mspm0_data##inst, &flash_ti_mspm0_cfg##inst,	    \
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,			    \
		      &flash_ti_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_TI_MSPM0_DEVICE_INIT);
