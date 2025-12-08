/*
 * Copyright (c) 2025 Endress+Hauser GmbH+Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_fmc_nand

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/memc/memc_stm32.h>
#include <zephyr/kernel.h>

#include "flash_stm32_fmc_nand.h"

#define STM32_FMC_NAND_USE_DMA DT_ANY_INST_HAS_PROP_STATUS_OKAY(dmas)

#if STM32_FMC_NAND_USE_DMA
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#endif /* STM32_FMC_NAND_USE_DMA */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32_fmc_nand, CONFIG_FLASH_LOG_LEVEL);

#define DMA_TIMEOUT_MS        1000
#define NAND_CMD_SET_FEATURES ((uint8_t)0xEF)
#define NAND_TIMEOUT_MS       2000
#define PAGE_BUFFER_ALIGNMENT 4

#define STM32_FMC_NAND_ARRAY_ADDRESS(address, data)                                                \
	((address)->page +                                                                         \
	 (((address)->block + ((address)->plane * (data)->plane_size)) * (data)->block_size))

#define STM32_FMC_NAND_COLUMN_ADDRESS(data) ((data)->page_size)

#if STM32_FMC_NAND_USE_DMA
struct stream {
	bool enabled;
	struct k_sem sync;
	DMA_TypeDef *reg;
	const struct device *dev;
	uint32_t channel;
	struct dma_config cfg;
	struct dma_block_config block_cfg;
};
#endif /* STM32_FMC_NAND_USE_DMA */

enum nand_state {
	NAND_STATE_RESET = 0, /* Not yet initialised or disabled */
	NAND_STATE_READY,     /* Initialised and ready */
	NAND_STATE_BUSY,      /* Busy */
	NAND_STATE_ERROR,     /* Failed */
};

#if STM32_FMC_NAND_USE_DMA
struct flash_stm32_fmc_nand_config {
	uint8_t *page_buffer;
};
#endif /* STM32_FMC_NAND_USE_DMA */

struct flash_stm32_fmc_nand_data {
	FMC_NAND_TypeDef *instance;
	FMC_NAND_InitTypeDef init;
	struct k_sem lock;
	enum nand_state state;
	size_t page_size;       /* Page size in bytes or words depending on addressing */
	size_t spare_area_size; /* Spare area size in bytes or words depending on addressing */
	size_t block_size;      /* Block size in number of pages */
	size_t plane_size;      /* Plane size in number of blocks */
	size_t total_pages;     /* Total number of pages */
#if STM32_FMC_NAND_USE_DMA
	struct stream dma;
#endif /* STM32_FMC_NAND_USE_DMA */
};

/* Reads status until NAND is ready or reports an error */
static int flash_stm32_fmc_nand_wait(void)
{
	uint8_t status;
	k_timepoint_t deadline;

	deadline = sys_timepoint_calc(SYS_TIMEOUT_MS(NAND_TIMEOUT_MS));
	while (true) {
		/* Send read status operation command */
		sys_write8(NAND_CMD_STATUS, NAND_DEVICE | CMD_AREA);

		/* Read status register data */
		status = sys_read8(NAND_DEVICE);

		if ((status & NAND_READY) == NAND_READY) {
			break;
		} else if ((status & NAND_ERROR) == NAND_ERROR) {
			return -EIO;
		} else if (sys_timepoint_expired(deadline)) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

int flash_stm32_fmc_nand_read_page_chunk(const struct device *dev,
					 const struct nand_flash_address *address,
					 off_t page_offset, size_t chunk, uint8_t *data)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;
	uint32_t nand_address;
	int ret;

	k_sem_take(&dev_data->lock, K_FOREVER);

	if (dev_data->state == NAND_STATE_BUSY) {
		k_sem_give(&dev_data->lock);
		return -EBUSY;
	} else if (dev_data->state != NAND_STATE_READY) {
		k_sem_give(&dev_data->lock);
		return -EIO;
	}

	dev_data->state = NAND_STATE_BUSY;
	nand_address = STM32_FMC_NAND_ARRAY_ADDRESS(address, dev_data); /* Raw NAND address */

	/* Send read page command sequence */
	sys_write8(NAND_CMD_AREA_A, NAND_DEVICE | CMD_AREA);

	if (dev_data->page_size <= 512) {
		if (dev_data->total_pages <= 65535) {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		} else {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		}
	} else {
		if (dev_data->total_pages <= 65535) {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		} else {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		}
	}

	sys_write8(NAND_CMD_AREA_TRUE1, NAND_DEVICE | CMD_AREA);

	/* Read status until NAND is ready or reports an error */
	ret = flash_stm32_fmc_nand_wait();
	if (ret == -EIO) {
		LOG_ERR("Uncorrectable ECC error detected");
		dev_data->state = NAND_STATE_READY;
		k_sem_give(&dev_data->lock);
		return ret;
	} else if (ret == -ETIMEDOUT) {
		dev_data->state = NAND_STATE_ERROR;
		k_sem_give(&dev_data->lock);
		return ret;
	}

	/* Go back to read mode */
	sys_write8(NAND_CMD_AREA_A, NAND_DEVICE | CMD_AREA);

#if STM32_FMC_NAND_USE_DMA
	if (dev_data->dma.enabled) {
		const struct flash_stm32_fmc_nand_config *config = dev->config;

		/* Get page data into buffer */
		k_sem_reset(&dev_data->dma.sync);
		if (dma_reload(dev_data->dma.dev, dev_data->dma.channel, NAND_DEVICE,
			       (uint32_t)config->page_buffer, dev_data->page_size) != 0) {
			dev_data->state = NAND_STATE_ERROR;
			k_sem_give(&dev_data->lock);
			return -EIO;
		}
		if (k_sem_take(&dev_data->dma.sync, K_MSEC(DMA_TIMEOUT_MS)) != 0) {
			dev_data->state = NAND_STATE_ERROR;
			k_sem_give(&dev_data->lock);
			return -EIO;
		}

		/* Get chunk into output buffer */
		if (dma_reload(dev_data->dma.dev, dev_data->dma.channel,
			       (uint32_t)(config->page_buffer + page_offset), (uint32_t)data,
			       chunk) != 0) {
			dev_data->state = NAND_STATE_ERROR;
			k_sem_give(&dev_data->lock);
			return -EIO;
		}
	} else {
#endif /* STM32_FMC_NAND_USE_DMA */

		/* Read chunk of page data directly into output buffer */
		for (size_t index = 0; index < dev_data->page_size; index++) {
			if (index >= page_offset && index < (page_offset + chunk)) {
				*data = sys_read8(NAND_DEVICE);
				data++;
			} else {
				(void)sys_read8(NAND_DEVICE);
			}
		}

#if STM32_FMC_NAND_USE_DMA
	}
#endif /* STM32_FMC_NAND_USE_DMA */

	dev_data->state = NAND_STATE_READY;
	k_sem_give(&dev_data->lock);

	return 0;
}

int flash_stm32_fmc_nand_read_spare_area(const struct device *dev,
					 const struct nand_flash_address *address, uint8_t *data)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;
	uint32_t nand_address, column_address;
	uint8_t *buffer = data;
	int ret;

	k_sem_take(&dev_data->lock, K_FOREVER);

	if (dev_data->state == NAND_STATE_BUSY) {
		k_sem_give(&dev_data->lock);
		return -EBUSY;
	} else if (dev_data->state != NAND_STATE_READY) {
		k_sem_give(&dev_data->lock);
		return -EIO;
	}

	dev_data->state = NAND_STATE_BUSY;
	nand_address = STM32_FMC_NAND_ARRAY_ADDRESS(address, dev_data); /* Raw NAND address */
	column_address = STM32_FMC_NAND_COLUMN_ADDRESS(dev_data);

	/* Send read spare area command sequence */
	if (dev_data->page_size <= 512) {
		sys_write8(NAND_CMD_AREA_C, NAND_DEVICE | CMD_AREA);

		if (dev_data->total_pages <= 65535) {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		} else {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		}
	} else {
		sys_write8(NAND_CMD_AREA_A, NAND_DEVICE | CMD_AREA);

		if (dev_data->total_pages <= 65535) {
			sys_write8(COLUMN_1ST_CYCLE(column_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(COLUMN_2ND_CYCLE(column_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		} else {
			sys_write8(COLUMN_1ST_CYCLE(column_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(COLUMN_2ND_CYCLE(column_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		}
	}

	sys_write8(NAND_CMD_AREA_TRUE1, NAND_DEVICE | CMD_AREA);

	/* Read status until NAND is ready or reports an error */
	ret = flash_stm32_fmc_nand_wait();
	if (ret == -EIO) {
		LOG_ERR("Uncorrectable ECC error detected");
		dev_data->state = NAND_STATE_READY;
		k_sem_give(&dev_data->lock);
		return ret;
	} else if (ret == -ETIMEDOUT) {
		dev_data->state = NAND_STATE_ERROR;
		k_sem_give(&dev_data->lock);
		return ret;
	}

	/* Go back to read mode */
	sys_write8(NAND_CMD_AREA_A, NAND_DEVICE | CMD_AREA);

	/* Get spare area data into output buffer */
	for (size_t index = 0; index < dev_data->spare_area_size; index++) {
		*buffer = sys_read8(NAND_DEVICE);
		buffer++;
	}

	dev_data->state = NAND_STATE_READY;
	k_sem_give(&dev_data->lock);

	return 0;
}

int flash_stm32_fmc_nand_write_page(const struct device *dev,
				    const struct nand_flash_address *address, const uint8_t *data)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;
	uint32_t nand_address;
	const uint8_t *buffer = data;
	int ret;

	k_sem_take(&dev_data->lock, K_FOREVER);

	if (dev_data->state == NAND_STATE_BUSY) {
		k_sem_give(&dev_data->lock);
		return -EBUSY;
	} else if (dev_data->state != NAND_STATE_READY) {
		k_sem_give(&dev_data->lock);
		return -EIO;
	}

	dev_data->state = NAND_STATE_BUSY;
	nand_address = STM32_FMC_NAND_ARRAY_ADDRESS(address, dev_data); /* Raw NAND address */

	/* Send write page command sequence */
	sys_write8(NAND_CMD_AREA_A, NAND_DEVICE | CMD_AREA);
	sys_write8(NAND_CMD_WRITE0, NAND_DEVICE | CMD_AREA);

	if (dev_data->page_size <= 512) {
		if (dev_data->total_pages <= 65535) {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		} else {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		}
	} else {
		if (dev_data->total_pages <= 65535) {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		} else {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		}
	}

	/* Write data to memory */
	for (size_t index = 0; index < dev_data->page_size; index++) {
		sys_write8(*buffer, NAND_DEVICE);
		buffer++;
	}

	sys_write8(NAND_CMD_WRITE_TRUE1, NAND_DEVICE | CMD_AREA);

	/* Read status until NAND is ready or reports an error */
	ret = flash_stm32_fmc_nand_wait();
	if (ret != 0) {
		dev_data->state = NAND_STATE_ERROR;
		k_sem_give(&dev_data->lock);
		return ret;
	}

	dev_data->state = NAND_STATE_READY;
	k_sem_give(&dev_data->lock);

	return 0;
}

int flash_stm32_fmc_nand_write_spare_area(const struct device *dev,
					  const struct nand_flash_address *address,
					  const uint8_t *data)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;
	uint32_t nand_address, column_address;
	const uint8_t *buffer = data;
	int ret;

	k_sem_take(&dev_data->lock, K_FOREVER);

	if (dev_data->state == NAND_STATE_BUSY) {
		k_sem_give(&dev_data->lock);
		return -EBUSY;
	} else if (dev_data->state != NAND_STATE_READY) {
		k_sem_give(&dev_data->lock);
		return -EIO;
	}

	dev_data->state = NAND_STATE_BUSY;
	nand_address = STM32_FMC_NAND_ARRAY_ADDRESS(address, dev_data); /* Raw NAND address */
	column_address = STM32_FMC_NAND_COLUMN_ADDRESS(dev_data);

	/* Send write spare area command sequence */
	if (dev_data->page_size <= 512) {
		sys_write8(NAND_CMD_AREA_C, NAND_DEVICE | CMD_AREA);
		sys_write8(NAND_CMD_WRITE0, NAND_DEVICE | CMD_AREA);

		if (dev_data->total_pages <= 65535) {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		} else {
			sys_write8(0x00, NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		}
	} else {
		sys_write8(NAND_CMD_AREA_A, NAND_DEVICE | CMD_AREA);
		sys_write8(NAND_CMD_WRITE0, NAND_DEVICE | CMD_AREA);

		if (dev_data->total_pages <= 65535) {
			sys_write8(COLUMN_1ST_CYCLE(column_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(COLUMN_2ND_CYCLE(column_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		} else {
			sys_write8(COLUMN_1ST_CYCLE(column_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(COLUMN_2ND_CYCLE(column_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
			sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
		}
	}

	/* Write data to memory */
	for (size_t index = 0; index < dev_data->spare_area_size; index++) {
		sys_write8(*buffer, NAND_DEVICE);
		buffer++;
	}

	sys_write8(NAND_CMD_WRITE_TRUE1, NAND_DEVICE | CMD_AREA);

	/* Read status until NAND is ready or reports an error */
	ret = flash_stm32_fmc_nand_wait();
	if (ret != 0) {
		dev_data->state = NAND_STATE_ERROR;
		k_sem_give(&dev_data->lock);
		return ret;
	}

	dev_data->state = NAND_STATE_READY;
	k_sem_give(&dev_data->lock);

	return 0;
}

int flash_stm32_fmc_nand_erase_block(const struct device *dev,
				     const struct nand_flash_address *address)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;
	uint32_t nand_address;
	int ret;

	k_sem_take(&dev_data->lock, K_FOREVER);

	if (dev_data->state == NAND_STATE_BUSY) {
		k_sem_give(&dev_data->lock);
		return -EBUSY;
	} else if (dev_data->state != NAND_STATE_READY) {
		k_sem_give(&dev_data->lock);
		return -EIO;
	}

	dev_data->state = NAND_STATE_BUSY;
	nand_address = STM32_FMC_NAND_ARRAY_ADDRESS(address, dev_data); /* Raw NAND address */

	/* Send erase block command sequence */
	sys_write8(NAND_CMD_ERASE0, NAND_DEVICE | CMD_AREA);

	sys_write8(ADDR_1ST_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
	sys_write8(ADDR_2ND_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);
	sys_write8(ADDR_3RD_CYCLE(nand_address), NAND_DEVICE | ADDR_AREA);

	sys_write8(NAND_CMD_ERASE1, NAND_DEVICE | CMD_AREA);

	/* Read status until NAND is ready or reports an error */
	ret = flash_stm32_fmc_nand_wait();
	if (ret == -EIO) {
		LOG_ERR("Bad block detected");
		dev_data->state = NAND_STATE_READY;
		k_sem_give(&dev_data->lock);
		return ret;
	} else if (ret == -ETIMEDOUT) {
		dev_data->state = NAND_STATE_ERROR;
		k_sem_give(&dev_data->lock);
		return ret;
	}

	dev_data->state = NAND_STATE_READY;
	k_sem_give(&dev_data->lock);

	return 0;
}

int flash_stm32_fmc_nand_init_bank(const struct device *dev,
				   const struct flash_stm32_fmc_nand_init *init)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;

	k_sem_take(&dev_data->lock, K_FOREVER);

	dev_data->instance = FMC_NAND_DEVICE;

	if (init->bank == 3) {
		dev_data->init.NandBank = FMC_NAND_BANK3;
	} else {
		LOG_ERR("Unsupported FMC NAND bank %d", init->bank);
		k_sem_give(&dev_data->lock);
		return -EINVAL;
	}

	dev_data->init.Waitfeature = FMC_NAND_WAIT_FEATURE_ENABLE;
	dev_data->init.MemoryDataWidth = FMC_NAND_MEM_BUS_WIDTH_8;
	dev_data->init.EccComputation = FMC_NAND_ECC_DISABLE;
	dev_data->init.ECCPageSize = FMC_NAND_ECC_PAGE_SIZE_2048BYTE;
	dev_data->init.TCLRSetupTime = 0;
	dev_data->init.TARSetupTime = 0;

	dev_data->page_size = init->page_size;
	dev_data->spare_area_size = init->spare_area_size;
	dev_data->block_size = init->block_size / init->page_size;
	dev_data->plane_size = init->plane_size / init->block_size;
	dev_data->total_pages = init->flash_size / init->page_size;

	(void)FMC_NAND_Init(dev_data->instance, &dev_data->init);

	FMC_NAND_PCC_TimingTypeDef timing = {
		.SetupTime = init->setup_time,
		.WaitSetupTime = init->wait_setup_time,
		.HoldSetupTime = init->hold_setup_time,
		.HiZSetupTime = init->hiz_setup_time,
	};

	(void)FMC_NAND_CommonSpace_Timing_Init(dev_data->instance, &timing,
					       dev_data->init.NandBank);
	(void)FMC_NAND_AttributeSpace_Timing_Init(dev_data->instance, &timing,
						  dev_data->init.NandBank);

	__FMC_NAND_ENABLE(dev_data->instance);
	__FMC_ENABLE();

	dev_data->state = NAND_STATE_READY;
	k_sem_give(&dev_data->lock);

	return 0;
}

int flash_stm32_fmc_nand_reset(const struct device *dev)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;

	k_sem_take(&dev_data->lock, K_FOREVER);

	if (dev_data->state == NAND_STATE_BUSY) {
		k_sem_give(&dev_data->lock);
		return -EBUSY;
	} else if (dev_data->state != NAND_STATE_READY) {
		k_sem_give(&dev_data->lock);
		return -EIO;
	}

	dev_data->state = NAND_STATE_BUSY;

	/* Send NAND reset command */
	sys_write8(NAND_CMD_RESET, NAND_DEVICE | CMD_AREA);

	dev_data->state = NAND_STATE_READY;
	k_sem_give(&dev_data->lock);

	return 0;
}

int flash_stm32_fmc_nand_set_feature(const struct device *dev,
				     const struct nand_flash_feature *feature)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;
	int ret;

	k_sem_take(&dev_data->lock, K_FOREVER);

	if (dev_data->state == NAND_STATE_BUSY) {
		k_sem_give(&dev_data->lock);
		return -EBUSY;
	} else if (dev_data->state != NAND_STATE_READY) {
		k_sem_give(&dev_data->lock);
		return -EIO;
	}

	dev_data->state = NAND_STATE_BUSY;

	/* Send feature setting command sequence */
	sys_write8(NAND_CMD_SET_FEATURES, NAND_DEVICE | CMD_AREA);
	sys_write8(feature->feature_addr, NAND_DEVICE | ADDR_AREA);
	sys_write8(feature->feature_data[0], NAND_DEVICE);
	sys_write8(feature->feature_data[1], NAND_DEVICE);
	sys_write8(feature->feature_data[2], NAND_DEVICE);
	sys_write8(feature->feature_data[3], NAND_DEVICE);

	/* Read status until NAND is ready or reports an error */
	ret = flash_stm32_fmc_nand_wait();
	if (ret != 0) {
		dev_data->state = NAND_STATE_ERROR;
		k_sem_give(&dev_data->lock);
		return ret;
	}

	dev_data->state = NAND_STATE_READY;
	k_sem_give(&dev_data->lock);

	return 0;
}

static int flash_stm32_fmc_nand_init(const struct device *dev)
{
	struct flash_stm32_fmc_nand_data *dev_data = dev->data;
	uint32_t __maybe_unused fmc_freq;

	dev_data->state = NAND_STATE_RESET;
	k_sem_init(&dev_data->lock, 1, 1);

	memc_stm32_fmc_clock_rate(&fmc_freq);
	LOG_DBG("FMC clock rate: %d Hz", fmc_freq);

#if STM32_FMC_NAND_USE_DMA
	if (dev_data->dma.enabled) {
		const struct flash_stm32_fmc_nand_config *config = dev->config;
		int ret;

		if (!device_is_ready(dev_data->dma.dev)) {
			LOG_ERR("DMA %s device is not ready", dev_data->dma.dev->name);
			return -ENODEV;
		}

		/*
		 * Dummy address configuration to avoid warnings in dma_config(). The correct
		 * addresses are set with dma_reload().
		 */
		dev_data->dma.block_cfg.source_address = (uint32_t)config->page_buffer;
		dev_data->dma.block_cfg.dest_address = (uint32_t)config->page_buffer;

		dev_data->dma.cfg.head_block = &dev_data->dma.block_cfg;
		k_sem_init(&dev_data->dma.sync, 0, 1);
		dev_data->dma.cfg.user_data = &dev_data->dma.sync;

		ret = dma_config(dev_data->dma.dev, dev_data->dma.channel, &dev_data->dma.cfg);
		if (ret != 0) {
			LOG_ERR("Failed to configure DMA channel %d with error %d",
				dev_data->dma.channel, ret);
			return -EIO;
		}

		LOG_INF("FMC NAND with DMA transfer");
	}
#endif /* STM32_FMC_NAND_USE_DMA */

	return 0;
}

/* This function is executed in the interrupt context */
#if STM32_FMC_NAND_USE_DMA
static void fmc_nand_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				  int status)
{
	ARG_UNUSED(dev);

	k_sem_give((struct k_sem *)user_data);

	if (status < 0) {
		LOG_ERR("DMA callback error %d with channel %d", status, channel);
	}
}

#define DMA_CHANNEL_CONFIG(node, dir) DT_DMAS_CELL_BY_NAME(node, dir, channel_config)

#define FMC_NAND_DMA_CHANNEL_INIT(node, dir)                                                       \
	.enabled = true, .reg = (DMA_TypeDef *)DT_REG_ADDR(DT_PHANDLE_BY_NAME(node, dmas, dir)),   \
	.dev = DEVICE_DT_GET(DT_DMAS_CTLR(node)),                                                  \
	.channel = DT_DMAS_CELL_BY_NAME(node, dir, channel),                                       \
	.cfg = {                                                                                   \
		.channel_direction = MEMORY_TO_MEMORY,                                             \
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(DMA_CHANNEL_CONFIG(node, dir)),      \
		.source_data_size =                                                                \
			STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(DMA_CHANNEL_CONFIG(node, dir)),      \
		.dest_data_size =                                                                  \
			STM32_DMA_CONFIG_MEMORY_DATA_SIZE(DMA_CHANNEL_CONFIG(node, dir)),          \
		.source_burst_length = 64,                                                         \
		.dest_burst_length = 64,                                                           \
		.block_count = 1,                                                                  \
		.dma_callback = fmc_nand_dma_callback,                                             \
	}

#define FMC_NAND_DMA_CHANNEL(node, dir)                                                            \
	.dma = {COND_CODE_1(DT_DMAS_HAS_NAME(node, dir),                                           \
			    (FMC_NAND_DMA_CHANNEL_INIT(node, dir)),                                \
			    (NULL))},

#define FMC_NAND_PAGE_BUFFER(node)                                                                 \
	static uint8_t __nocache __aligned(PAGE_BUFFER_ALIGNMENT)                                  \
	flash_stm32_fmc_nand_page_buffer_##node[DT_INST_PROP(node, page_buffer_size)];             \
                                                                                                   \
	static const struct flash_stm32_fmc_nand_config flash_stm32_fmc_nand_config_##node = {     \
		.page_buffer = flash_stm32_fmc_nand_page_buffer_##node,                            \
	};
#else
#define FMC_NAND_DMA_CHANNEL(node, dir)
#define FMC_NAND_PAGE_BUFFER(node)
#endif /* STM32_FMC_NAND_USE_DMA */

#define FLASH_STM32_FMC_NAND_INIT(n)                                                               \
	FMC_NAND_PAGE_BUFFER(n)                                                                    \
	static struct flash_stm32_fmc_nand_data flash_stm32_fmc_nand_data_##n = {                  \
		FMC_NAND_DMA_CHANNEL(DT_DRV_INST(n), tx_rx)                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, flash_stm32_fmc_nand_init, NULL, &flash_stm32_fmc_nand_data_##n,  \
			      COND_CODE_1(STM32_FMC_NAND_USE_DMA,                                  \
					  (&flash_stm32_fmc_nand_config_##n), (NULL)),             \
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FLASH_STM32_FMC_NAND_INIT)
