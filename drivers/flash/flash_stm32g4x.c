/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32g4
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>

#include "flash_stm32.h"

#define STM32G4X_PAGE_SHIFT	11

/*
 * offset and len must be aligned on 8 for write,
 * positive and not beyond end of flash
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	return (!write || (offset % 8 == 0 && len % 8 == 0U)) &&
		flash_stm32_range_exists(dev, offset, len);
}

/*
 * STM32G4xx devices can have up to 64 2K pages in a single banks
 */
static unsigned int get_page(off_t offset)
{
	return offset >> STM32G4X_PAGE_SHIFT;
}

static int write_dword(const struct device *dev, off_t offset, uint64_t val)
{
	volatile uint32_t *flash = (uint32_t *)(offset + CONFIG_FLASH_BASE_ADDRESS);
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		LOG_ERR("CR locked");
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check if this double word is erased */
	if (flash[0] != 0xFFFFFFFFUL ||
	    flash[1] != 0xFFFFFFFFUL) {
		LOG_ERR("Word at offs %d not erased", offset);
		return -EIO;
	}

	/* Set the PG bit */
	regs->CR |= FLASH_CR_PG;

	/* Flush the register write */
	tmp = regs->CR;

	/* Perform the data write operation at the desired memory address */
	flash[0] = (uint32_t)val;
	flash[1] = (uint32_t)(val >> 32);

	/* Wait until the BSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

	/* Clear the PG bit */
	regs->CR &= (~FLASH_CR_PG);

	return rc;
}

static int erase_page(const struct device *dev, unsigned int page)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		LOG_ERR("CR locked");
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

#ifdef FLASH_OPTR_DBANK
	if (page > 127) {
		if (!(regs->OPTR & FLASH_OPTR_DBANK)) {
			LOG_ERR("Page %d does not exist when DBANK=0", page);
			return -EINVAL;
		}

		/* The pages to be erased is in bank 2*/
		regs->CR |= FLASH_CR_BKER;
		page = page - 128;
		LOG_DBG("Erase page %d on bank 2", page);
	} else {
		LOG_DBG("Erase page %d on bank 1", page);
	}


	__ASSERT(page <= 127, "There are only 127 pages, but page is %d", page);
#endif

	/* Set the PER bit and select the page you wish to erase */
	regs->CR |= FLASH_CR_PER;
	regs->CR &= ~FLASH_CR_PNB_Msk;
	regs->CR |= (page << FLASH_CR_PNB_Pos);

	/* Set the STRT bit */
	regs->CR |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->CR;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

#ifdef FLASH_OPTR_DBANK
	regs->CR &= ~(FLASH_CR_PER | FLASH_CR_BKER);
#else
	regs->CR &= ~(FLASH_CR_PER);
#endif

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
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

int flash_stm32_write_range(const struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;

	for (i = 0; i < len; i += 8, offset += 8) {
		rc = write_dword(dev, offset, ((const uint64_t *) data)[i>>3]);
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32g4_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32g4_flash_layout.pages_count == 0) {
		stm32g4_flash_layout.pages_count = FLASH_SIZE / FLASH_PAGE_SIZE;
		stm32g4_flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &stm32g4_flash_layout;
	*layout_size = 1;
}
