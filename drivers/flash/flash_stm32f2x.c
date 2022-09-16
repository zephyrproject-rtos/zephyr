/*
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <soc.h>

#include "flash_stm32.h"

bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	ARG_UNUSED(write);

	return flash_stm32_range_exists(dev, offset, len);
}

static inline void flush_cache(FLASH_TypeDef *regs)
{
	/* If Data cache is enabled, disable Data cache, reset Data cache
	 * and then re-enable Data cache.
	 */
	if (regs->ACR & FLASH_ACR_DCEN) {
		regs->ACR &= ~FLASH_ACR_DCEN;
		/* Datasheet: DCRST: Data cache reset
		 * This bit can be written only when the Data cache is disabled
		 */
		regs->ACR |= FLASH_ACR_DCRST;
		regs->ACR &= ~FLASH_ACR_DCRST;
		regs->ACR |= FLASH_ACR_DCEN;
	}

	/* If Instruction cache is enabled, disable Instruction cache, reset
	 * Instruction cache and then re-enable Instruction cache.
	 */
	if (regs->ACR & FLASH_ACR_ICEN) {
		regs->ACR &= ~FLASH_ACR_ICEN;
		/* Datasheet: ICRST: Instruction cache reset
		 * This bit can be written only when the Instruction cache
		 * is disabled
		 */
		regs->ACR |= FLASH_ACR_ICRST;
		regs->ACR &= ~FLASH_ACR_ICRST;
		regs->ACR |= FLASH_ACR_ICEN;
	}
}

static int write_byte(const struct device *dev, off_t offset, uint8_t val)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	regs->CR &= ~FLASH_CR_PSIZE;
	regs->CR |= FLASH_PSIZE_BYTE;
	regs->CR |= FLASH_CR_PG;

	/* flush the register write */
	tmp = regs->CR;

	*((uint8_t *) offset + CONFIG_FLASH_BASE_ADDRESS) = val;

	/* Wait until the BSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

	/* Clear the PG bit */
	regs->CR &= (~FLASH_CR_PG);

	return rc;
}

static int erase_sector(const struct device *dev, uint32_t sector)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	regs->CR &= ~FLASH_CR_SNB;
	regs->CR |= FLASH_CR_SER | (sector << 3);
	regs->CR |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->CR;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	flush_cache(regs);

	regs->CR &= ~(FLASH_CR_SER | FLASH_CR_SNB);

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	struct flash_pages_info info;
	uint32_t start_sector, end_sector;
	uint32_t i;
	int rc = 0;

	rc = flash_get_page_info_by_offs(dev, offset, &info);
	if (rc) {
		return rc;
	}
	start_sector = info.index;
	rc = flash_get_page_info_by_offs(dev, offset + len - 1, &info);
	if (rc) {
		return rc;
	}
	end_sector = info.index;

	for (i = start_sector; i <= end_sector; i++) {
		rc = erase_sector(dev, i);
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

	for (i = 0; i < len; i++, offset++) {
		rc = write_byte(dev, offset, ((const uint8_t *) data)[i]);
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

/*
 * The flash memory in stm32f2 series has bank 1 only with 12 sectors,
 * they are split as 4 sectors of 16 Kbytes, 1 sector of 64 Kbytes,
 * and 7 sectors of 128 Kbytes.
 */
#ifndef FLASH_SECTOR_TOTAL
#error "Unknown flash layout"
#else  /* defined(FLASH_SECTOR_TOTAL) */
#if FLASH_SECTOR_TOTAL == 12
static const struct flash_pages_layout stm32f2_flash_layout[] = {
	/*
	 * PM0059, table 10: STM32F207xx
	 */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#else
#error "Unknown flash layout"
#endif /* FLASH_SECTOR_TOTAL == 12 */
#endif/* !defined(FLASH_SECTOR_TOTAL) */

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = stm32f2_flash_layout;
	*layout_size = ARRAY_SIZE(stm32f2_flash_layout);
}
