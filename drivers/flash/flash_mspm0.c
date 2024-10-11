#include <zephyr/kernel.h>
#include <zephyr/device.h>
#define DT_DRV_COMPAT ti_mspm0_flash_controller
#include <zephyr/drivers/flash.h>
#include <soc.h>
#include <zephyr/logging/log.h>
#include <driverlib/dl_flashctl.h>
#include "flash_mspm0.h"

struct flash_mspm0_config {
	FLASHCTL_Regs *regs;
};

struct flash_mspm0_data {
	struct k_sem lock;
	size_t num_banks;
	size_t bank_size;
	size_t flash_size;
};

#define MSPM0_BANK_COUNT		1
#define MSPM0_FLASH_SIZE		(FLASH_SIZE)
#define MSPM0_FLASH_PAGE_SIZE		(FLASH_PAGE_SIZE)
#define MSPM0_PAGES_PER_BANK		\
	((MSPM0_FLASH_SIZE / MSPM0_FLASH_PAGE_SIZE) / MSPM0_BANK_COUNT)

LOG_MODULE_REGISTER(flash_mspm0, CONFIG_FLASH_LOG_LEVEL); 

static const struct flash_parameters flash_mspm0_parameters = {
	.write_block_size	= FLASH_MSPM0_WRITE_BLOCK_SIZE,
	.erase_value		= 0xff
};

static inline void flash_mspm0_lock(const struct device *dev)
{
#if defined(CONFIG_MULTITHREADING)
	struct flash_mspm0_data *mdata = dev->data;

	k_sem_take(&mdata->lock, K_FOREVER);
#endif
}

static inline void flash_mspm0_unlock(const struct device *dev)
{
#if defined(CONFIG_MULTITHREADING)
	struct flash_mspm0_data *mdata = dev->data;

	k_sem_give(&mdata->lock);
#endif
}

static int flash_mspm0_init(const struct device *dev)
{
	struct flash_mspm0_data *data = dev->data;

	data->num_banks		= DL_FactoryRegion_getNumBanks();
	data->flash_size	= DL_FactoryRegion_getMAINFlashSize();
	data->bank_size		= ((data->flash_size / data->num_banks) * 1024U);

	k_sem_init(&data->lock, 1, 1);
#if ((CONFIG_FLASH_LOG_LEVEL >= LOG_LEVEL_DBG) && CONFIG_FLASH_PAGE_LAYOUT)
	const struct flash_pages_layout *layout;
	size_t layout_size;

	flash_mspm0_page_layout(dev, &layout, &layout_size);
	for (size_t i = 0; i < layout_size; i++) {
		LOG_DBG("Block %zu: bs: %zu count: %zu", i,
				layout[i].pages_size, layout[i].pages_count);
	}
#endif
	return 0;
}

static inline int flash_mspm0_valid_range(off_t offset, size_t len)
{
	if (offset < 0) {
		return -EINVAL;
	}

	if ((offset + len) > CONFIG_FLASH_SIZE * 1024) {
		return -EINVAL;
	}

	return 0;
}

static inline void  flash_mspm0_select_bank(const struct device *dev,
					    size_t offset, size_t len)
{
	DL_FLASHCTL_BANK_SELECT bank_select;
	FLASHCTL_Regs *regs = FLASH_MSPM0_REGS(dev);
	struct flash_mspm0_data *data = dev->data;

	if (data->num_banks == 1) {
		return;
	}

	if ((offset + len) > data->bank_size) {
		bank_select = DL_FLASHCTL_BANK_SELECT_1;
	} else {
		bank_select = DL_FLASHCTL_BANK_SELECT_0;
	}

	DL_FlashCTL_setBankSelect(regs, bank_select);
	return;
}

static int flash_mspm0_erase(const struct device *dev, off_t offset, size_t len)
{
	size_t iter;
	FLASHCTL_Regs *regs = FLASH_MSPM0_REGS(dev);

	if (flash_mspm0_valid_range(offset, len)) {
		LOG_ERR("Erase range invalid. Offset %ld, len: %zu",
			(long int) offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	if (len < FLASH_PAGE_SIZE) {
		LOG_ERR("Erase must be done in page length manner");
		return -EINVAL;
	}

	iter = len / FLASH_PAGE_SIZE;
	flash_mspm0_lock(dev);
	do {
		DL_FlashCTL_unprotectSector(regs, offset, DL_FLASHCTL_REGION_SELECT_MAIN);
		DL_FlashCTL_eraseMemory(regs, offset, DL_FLASHCTL_COMMAND_SIZE_SECTOR);
		if (!DL_FlashCTL_waitForCmdDone(regs)) {
			flash_mspm0_unlock(dev);
			return -EINVAL;
		}

		offset = offset + FLASH_PAGE_SIZE;
		iter--;
	} while (iter != 0);

	flash_mspm0_unlock(dev);

	return 0;
}

static int flash_mspm0_write(const struct device *dev, off_t offset,
			     const void *data, size_t len)
{
	int ret = 0;
	size_t iter;
	size_t remain;
	uint32_t *write_data = (uint32_t *)data;
	FLASHCTL_Regs *regs = FLASH_MSPM0_REGS(dev);

	if (flash_mspm0_valid_range(offset, len)) {
		LOG_ERR("Erase range invalid. Offset %ld, len: %zu",
			(long int) offset, len);
		return -EINVAL;
	}

	if (offset % FLASH_MSPM0_FLASH_WRITE_SIZE) {
		LOG_DBG("offset must be 8-byte aligned");
		return -EINVAL;
	}

	flash_mspm0_lock(dev);
	iter = len / FLASH_MSPM0_FLASH_WRITE_SIZE;
	remain = len % FLASH_MSPM0_FLASH_WRITE_SIZE;

	for (int i = 0; i < iter; i++) {
		DL_FlashCTL_unprotectSector(regs, offset, DL_FLASHCTL_REGION_SELECT_MAIN);
		DL_FlashCTL_programMemory64WithECCGenerated(regs, offset, &write_data[i * 2]);
		if (!DL_FlashCTL_waitForCmdDone(regs)) {
			ret = -EINVAL;
			break;
		}
		offset = offset + FLASH_MSPM0_FLASH_WRITE_SIZE;
	}

	if (remain) {
		DL_FlashCTL_unprotectSector(regs, offset, DL_FLASHCTL_REGION_SELECT_MAIN);
		DL_FlashCTL_programMemory64WithECCGenerated(regs, offset, &write_data[iter * 2]);
		if (!DL_FlashCTL_waitForCmdDone(regs)) {
			ret = -EINVAL;
		}
	}

	flash_mspm0_unlock(dev);

	return ret;
}

static int flash_mspm0_read(const struct device *dev, off_t offset,
			    void *data, size_t len)
{
	if (flash_mspm0_valid_range(offset, len)) {
		LOG_ERR("Read range invalid. Offset %ld, len %zu",
			(long int) offset, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	LOG_DBG("Read offset: %ld, len %zu", (long int) offset, len);
	memcpy(data, (uint8_t*) FLASH_MSPM0_BASE_ADDRESS + offset, len);

	return 0;
}

static const struct flash_parameters *flash_mspm0_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);
	return &flash_mspm0_parameters;
}

void flash_mspm0_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout mspm0_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (mspm0_flash_layout.pages_count == 0) {
		mspm0_flash_layout.pages_count = MSPM0_FLASH_SIZE / MSPM0_FLASH_PAGE_SIZE;
		mspm0_flash_layout.pages_size = MSPM0_FLASH_PAGE_SIZE;
	}

	*layout = &mspm0_flash_layout;
	*layout_size = 1;
}

static const struct flash_mspm0_config flash_mspm0_cfg = {	
	.regs = (FLASHCTL_Regs *)DT_INST_REG_ADDR(0),
};

static struct flash_mspm0_data flash_mspm0;

static const struct flash_driver_api flash_mspm0_driver_api = {
	.erase = flash_mspm0_erase,
	.write = flash_mspm0_write,
	.read = flash_mspm0_read,
	.get_parameters = flash_mspm0_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_mspm0_page_layout,
#endif
};

DEVICE_DT_INST_DEFINE(0, flash_mspm0_init, NULL,
		      &flash_mspm0, &flash_mspm0_cfg, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_mspm0_driver_api);
