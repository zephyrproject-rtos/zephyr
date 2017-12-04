/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "flash_stm32l4"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_ERROR
#include <logging/sys_log.h>

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <init.h>
#include <soc.h>

#include "flash_stm32.h"

#define STM32L4X_PAGE_SHIFT	11

/* offset and len must be aligned on 8 for write
 * , positive and not beyond end of flash */
bool flash_stm32_valid_range(struct device *dev, off_t offset, u32_t len,
			     bool write)
{
	return (!write || (offset % 8 == 0 && len % 8 == 0)) &&
		flash_stm32_range_exists(dev, offset, len);
}

/* STM32L4xx devices can have up to 512 2K pages on two 256x2K pages banks */
static unsigned int get_page(off_t offset)
{
	return offset >> STM32L4X_PAGE_SHIFT;
}

static int write_dword(struct device *dev, off_t offset, u64_t val)
{
	volatile u32_t *flash = (u32_t *)(offset + CONFIG_FLASH_BASE_ADDRESS);
	struct stm32l4x_flash *regs = FLASH_STM32_REGS(dev);
	u32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->cr & FLASH_CR_LOCK) {
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

	return rc;
}

static int erase_page(struct device *dev, unsigned int page)
{
	struct stm32l4x_flash *regs = FLASH_STM32_REGS(dev);
	u32_t tmp;
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

	/* Set the PER bit and select the page you wish to erase */
	regs->cr |= FLASH_CR_PER;
#ifdef FLASH_CR_BKER
	regs->cr &= ~FLASH_CR_BKER_Msk;
	/* Select bank, only for DUALBANK devices */
	if (page >= 256)
		regs->cr |= FLASH_CR_BKER;
#endif
	regs->cr &= ~FLASH_CR_PNB_Msk;
	regs->cr |= ((page % 256) << 3);

	/* Set the STRT bit */
	regs->cr |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->cr;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	regs->cr &= ~FLASH_CR_PER;

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
		rc = write_dword(dev, offset, ((const u64_t *) data)[i>>3]);
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
	static struct flash_pages_layout stm32l4_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32l4_flash_layout.pages_count == 0) {
		stm32l4_flash_layout.pages_count = FLASH_SIZE / FLASH_PAGE_SIZE;
		stm32l4_flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &stm32l4_flash_layout;
	*layout_size = 1;
}
