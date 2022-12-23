/*
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Copyright (c) 2022 Nuvoton Technology Corporation.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <errno.h>
#include <zephyr/init.h>
#include <soc.h>
#include "flash_priv.h"
#include "NuMicro.h"

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_numaker);

#define DT_DRV_COMPAT nuvoton_numaker_fmc

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)


struct flash_numaker_data {
	FMC_T* fmc;
	struct k_sem write_lock;
	uint32_t flash_block_base;
};

static const struct flash_parameters flash_numaker_parameters = {
#if DT_NODE_HAS_PROP(SOC_NV_FLASH_NODE, write_block_size)
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
#else
	.write_block_size = 0x04,
#endif
	.erase_value = 0xff,
};


/* Validate offset and length */
static bool flash_numaker_is_range_valid(off_t offset, size_t len)
{
	/* check for min value */
	if ((offset < 0) || (len < 1)) {
		return false;
	}

	/* check for max value */
	if ((offset + len) > (FMC_APROM_END - FMC_APROM_BASE)) {
		return false;
	}

	return true;
}

/*
 * @brief Erase a flash memory area.
 *
 * @param dev       Device struct
 * @param offset    The address's offset
 * @param len       The size of the buffer
 * @return 	0       on success,
 * @return 	-EINVAL erroneous code
 */
 
static int flash_numaker_erase(const struct device *dev, off_t offset,
			    size_t len)
{
	struct flash_numaker_data *dev_data = dev->data;
	uint32_t rc = 0;
	unsigned int key;
	int page_nums = (len / FMC_FLASH_PAGE_SIZE);
	uint32_t addr = dev_data->flash_block_base + offset;

	/* return SUCCESS for len == 0 (required by tests/drivers/flash) */
	if (!len) {
		return 0;
	}

	/* Validate range */
	if (!flash_numaker_is_range_valid(offset, len)) {
		return -EINVAL;
	}
    
	/* check alignment and erase only by pages */
	if (((addr % FMC_FLASH_PAGE_SIZE) != 0) || ((len % FMC_FLASH_PAGE_SIZE) != 0)) {
		return -EINVAL;
	}

	/* take semaphore */
	if (k_sem_take(&dev_data->write_lock, K_NO_WAIT)) {
		return -EACCES;
	}

    SYS_UnlockReg();
    key = irq_lock();
	while (page_nums) {
        if(((len >= FMC_BANK_SIZE)) && ((addr % FMC_BANK_SIZE) == 0)) {
            if(FMC_EraseBank(addr)) {
                rc = -EIO;
                goto move_exit;
            }
            page_nums -= (FMC_BANK_SIZE/FMC_FLASH_PAGE_SIZE);
            addr += FMC_BANK_SIZE;                
		} else {
			/* erase page */
			if(FMC_Erase(addr)) {
                rc = -EIO;
                goto move_exit;                
            }
			page_nums--;
			addr += FMC_FLASH_PAGE_SIZE;
		}
	}

move_exit:
    SYS_LockReg();
	irq_unlock(key);
	/* release semaphore */
	k_sem_give(&dev_data->write_lock);

	return rc;
}    
    

/*
 * @brief Read a flash memory area.
 *
 * @param dev       Device struct
 * @param offset    The address's offset
 * @param data      The buffer to store or read the value
 * @param length    The size of the buffer
 * @return 	0       on success,
 * @return -EIO     erroneous code
 */
static int flash_numaker_read(const struct device *dev, off_t offset,
				void *data, size_t len)
{
	struct flash_numaker_data *dev_data = dev->data;
	uint32_t addr = dev_data->flash_block_base + offset;

	/* return SUCCESS for len == 0 (required by tests/drivers/flash) */
	if (!len) {
		return 0;
	}

	/* Validate range */
	if (!flash_numaker_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	/* read flash */
	memcpy(data, (void *) addr, len);

	return 0;
}

static int32_t flash_numaker_block_write(uint32_t u32Addr, uint8_t *pu8Data, int blockSize)
{
    int32_t retval;
    uint32_t *pu32Data = (uint32_t *)pu8Data;

    SYS_UnlockReg();    
    if(blockSize == 4) {
        retval = FMC_Write(u32Addr, *pu32Data);
    } else if(blockSize == 8) {
        retval = FMC_Write8Bytes(u32Addr, *pu32Data, *(pu32Data + 1));
    } else {
        retval = -1;
    }
    SYS_LockReg();
    
    return retval;
}

static int flash_numaker_write(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	struct flash_numaker_data *dev_data = dev->data;
	uint32_t rc = 0;
	unsigned int key;    
	uint32_t addr = dev_data->flash_block_base + offset;
    int blockSize = flash_numaker_parameters.write_block_size;
    int blocks = (len / blockSize);
    uint8_t *pu8Data = (uint8_t *)data;

	/* return SUCCESS for len == 0 (required by tests/drivers/flash) */
	if (!len) {
		return 0;
	}

	/* Validate range */
	if (!flash_numaker_is_range_valid(offset, len)) {
		return -EINVAL;
	}

    /* Validate address alignment */
    if((addr % flash_numaker_parameters.write_block_size) != 0) {
		return -EINVAL;
	}
    
	if (k_sem_take(&dev_data->write_lock, K_FOREVER)) {
		return -EACCES;
	}

	key = irq_lock();

    while(blocks) {
        if(flash_numaker_block_write(addr, pu8Data, blockSize)) {
            rc = -EIO;
            goto move_exit;
        }
        pu8Data += blockSize;
        addr += blockSize;
        blocks --;
    }
    if((len % blockSize) !=0 ) {
        uint8_t buffer[blockSize];
        memcpy(buffer, (void *) addr, blockSize);
        memcpy(buffer, (void *) pu8Data, len / blockSize);
        if(flash_numaker_block_write(addr, buffer, blockSize)) {
            rc = -EIO;
            goto move_exit;
        }
    }

move_exit:
	irq_unlock(key);

	k_sem_give(&dev_data->write_lock);

	return rc;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
				DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_numaker_pages_layout(const struct device *dev,
				    const struct flash_pages_layout **layout,
				    size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_numaker_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_numaker_parameters;
}

static struct flash_numaker_data flash_data;

static const struct flash_driver_api flash_numaker_api = {
	.erase = flash_numaker_erase,
	.write = flash_numaker_write,
	.read = flash_numaker_read,
	.get_parameters = flash_numaker_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_numaker_pages_layout,
#endif
};

static int flash_numaker_init(const struct device *dev)
{
	struct flash_numaker_data *dev_data = dev->data;

	k_sem_init(&dev_data->write_lock, 1, 1);
    
    /* Enable FMC ISP function */
    SYS_UnlockReg();
    FMC_Open();
    /* Enable APROM update. */
    FMC_ENABLE_AP_UPDATE();
    SYS_LockReg();
	dev_data->flash_block_base = (uint32_t) FMC_APROM_BASE;
    dev_data->fmc = (FMC_T *)DT_REG_ADDR(DT_NODELABEL(fmc));
    
	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_numaker_init, NULL,
			&flash_data, NULL, POST_KERNEL,
			CONFIG_FLASH_INIT_PRIORITY, &flash_numaker_api);
