/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32wb
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <init.h>
#include <soc.h>
#include <misc/__assert.h>

#include "flash_stm32.h"

#define STM32WBX_PAGE_SHIFT	12

/* offset and len must be aligned on 8 for write
 * , positive and not beyond end of flash */
bool flash_stm32_valid_range(struct device *dev, off_t offset, u32_t len,
			     bool write)
{
	return (!write || (offset % 8 == 0 && len % 8 == 0)) &&
	       flash_stm32_range_exists(dev, offset, len);
}

/*
 * Up to 255 4K pages
 */
static u32_t get_page(off_t offset)
{
	return offset >> STM32WBX_PAGE_SHIFT;
}

static int check_flash_unlock()
{
	int rc = 0;

	/* verify Flash is unlocked */
	if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) == 0u) {
		rc = -EIO;
	}

	return rc;
}

static int write_dword(struct device *dev, off_t offset, u64_t val)
{
	u32_t address = (u32_t)(offset + CONFIG_FLASH_BASE_ADDRESS);
	HAL_StatusTypeDef rc;

	/* if the control register is locked, do not fail silently */
	if (!check_flash_unlock()) {
		return -EIO;
	}

	flash_stm32_check_status(dev);
	/* Perform the data write operation at the desired memory address */
	rc = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, val);

	if (rc != HAL_OK) {
		return -EIO;
	}

	return 0;
}

static int erase_page(struct device *dev, u32_t page)
{
	int rc;

	/* if the control register is locked, do not fail silently */
	if (!check_flash_unlock()) {
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check erase operation allowed */
	if (HAL_FLASHEx_IsOperationSuspended()) {
		return -EBUSY;
	}

	/* Proceed to erase the page */
	MODIFY_REG(FLASH->CR, (FLASH_CR_PNB | FLASH_CR_PER), \
		   ((page << FLASH_CR_PNB_Pos) | FLASH_CR_PER));

	SET_BIT(FLASH->CR, FLASH_CR_STRT);

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	CLEAR_BIT(FLASH->CR, FLASH_TYPEERASE_PAGES);

	return rc;
}

int flash_stm32_block_erase_loop(struct device *dev, unsigned int offset,
				 unsigned int len)
{
	int i, rc = 0;

	i = get_page(offset);
	for (; i <= get_page(offset + len - 1) ; ++i) {
		rc = erase_page(dev, i);
		if (rc < 0) {
			break;
		}
	}

	return rc;
}

int flash_stm32_write_range(struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;

	for (i = 0; i < len; i += 8, offset += 8) {
		rc = write_dword(dev, offset,
				UNALIGNED_GET((const u64_t *) data + (i >> 3)));
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

void flash_stm32_page_layout(struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32wb_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32wb_flash_layout.pages_count == 0) {
		stm32wb_flash_layout.pages_count = FLASH_SIZE / FLASH_PAGE_SIZE;
		stm32wb_flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &stm32wb_flash_layout;
	*layout_size = 1;
}

int flash_stm32_check_status(struct device *dev)
{
	u32_t error = 0;

	/* Save Flash errors */
	error = (FLASH->SR & FLASH_FLAG_SR_ERROR);
	error |= (FLASH->ECCR & FLASH_FLAG_ECCC);

	/* TODO: Clean this */
	if (error & FLASH_FLAG_OPTVERR) {
		FLASH->SR |= FLASH_FLAG_SR_ERROR;
		return 0;
	}

	if (error) {
		return -EIO;
	}

	return 0;
}
