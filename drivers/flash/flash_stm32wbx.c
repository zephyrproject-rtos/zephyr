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
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>
#include <sys/__assert.h>

#include "flash_stm32.h"

#define STM32WBX_PAGE_SHIFT	12

/* offset and len must be aligned on 8 for write,
 * positive and not beyond end of flash
 */
bool flash_stm32_valid_range(struct device *dev, off_t offset, u32_t len,
			     bool write)
{
	return (!write || (offset % 8 == 0 && len % 8 == 0U)) &&
	       flash_stm32_range_exists(dev, offset, len);
}

/*
 * Up to 255 4K pages
 */
static u32_t get_page(off_t offset)
{
	return offset >> STM32WBX_PAGE_SHIFT;
}

static int write_dword(struct device *dev, off_t offset, u64_t val)
{
	volatile u32_t *flash = (u32_t *)(offset + CONFIG_FLASH_BASE_ADDRESS);
	struct stm32wbx_flash *regs = FLASH_STM32_REGS(dev);
	u32_t tmp;
	int ret, rc;

	/* if the control register is locked, do not fail silently */
	if (regs->cr & FLASH_CR_LOCK) {
		rc = -EIO;
	}

	/* Check if this double word is erased */
	if (flash[0] != 0xFFFFFFFFUL ||
	    flash[1] != 0xFFFFFFFFUL) {
		return -EIO;
	}

	ret = flash_stm32_check_status(dev);
	if (ret < 0) {
		return -EIO;
	}

	/* Set the PG bit */
	regs->cr |= FLASH_CR_PG;

	/* Flush the register write */
	tmp = regs->cr;

	/* Perform the data write operation at the desired memory address */
	flash[0] = (u32_t)val;
	flash[1] = (u32_t)(val >> 32);

	/* Wait until the BSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

	/* Clear the PG bit */
	regs->cr &= (~FLASH_CR_PG);

	return 0;
}

static int erase_page(struct device *dev, u32_t page)
{
	struct stm32wbx_flash *regs = FLASH_STM32_REGS(dev);
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->cr & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check erase operation allowed */
	if (regs->cr & FLASH_SR_PESD) {
		return -EBUSY;
	}

	/* Proceed to erase the page */
	regs->cr |= FLASH_CR_PER;
	regs->cr &= ~FLASH_CR_PNB_Msk;
	regs->cr |= page << FLASH_CR_PNB_Pos;

	regs->cr |= FLASH_CR_STRT;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	regs->cr &= (~FLASH_TYPEERASE_PAGES);

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

	for (i = 0; i < len; i += 8, offset += 8U) {
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
	struct stm32wbx_flash *regs = FLASH_STM32_REGS(dev);
	u32_t error = 0;

	/* Save Flash errors */
	error = (regs->sr & FLASH_FLAG_SR_ERROR);
	error |= (regs->eccr & FLASH_FLAG_ECCC);

	/* Clear systematic Option and Enginneering bits validity error */
	if (error & FLASH_FLAG_OPTVERR) {
		regs->sr |= FLASH_FLAG_SR_ERROR;
		return 0;
	}

	if (error) {
		return -EIO;
	}

	return 0;
}
