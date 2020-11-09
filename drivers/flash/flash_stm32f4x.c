/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>

#include "flash_stm32.h"

#define STM32F4X_SECTOR_MASK		((uint32_t) 0xFFFFFF07)

bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	ARG_UNUSED(write);

#if (FLASH_SECTOR_TOTAL == 12) && defined(FLASH_OPTCR_DB1M)
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	/*
	 * RM0090, table 7.1: STM32F42xxx, STM32F43xxx
	 */
	if (regs->OPTCR & FLASH_OPTCR_DB1M) {
		/* Device configured in Dual Bank, but not supported for now */
		return false;
	}
#endif

	return flash_stm32_range_exists(dev, offset, len);
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

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	regs->CR &= CR_PSIZE_MASK;
	regs->CR |= FLASH_PSIZE_BYTE;
	regs->CR |= FLASH_CR_PG;

	/* flush the register write */
	tmp = regs->CR;

	*((uint8_t *) offset + CONFIG_FLASH_BASE_ADDRESS) = val;

	rc = flash_stm32_wait_flash_idle(dev);
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

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

#if FLASH_SECTOR_TOTAL == 24
	/*
	 * RM0090, §3.9.8: STM32F42xxx, STM32F43xxx
	 * RM0386, §3.7.5: STM32F469xx, STM32F479xx
	 */
	if (sector >= 12) {
		/* From sector 12, SNB is offset by 0b10000 */
		sector += 4U;
	}
#endif

	regs->CR &= STM32F4X_SECTOR_MASK;
	regs->CR |= FLASH_CR_SER | (sector << 3);
	regs->CR |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->CR;

	rc = flash_stm32_wait_flash_idle(dev);
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
 * Different SoC flash layouts are specified in across various
 * reference manuals, but the flash layout for a given number of
 * sectors is consistent across these manuals, with one "gotcha". The
 * number of sectors is given by the HAL as FLASH_SECTOR_TOTAL.
 *
 * The only "gotcha" is that when there are 24 sectors, they are split
 * across 2 "banks" of 12 sectors each, with another set of small
 * sectors (16 KB) in the second bank occurring after the large ones
 * (128 KB) in the first. We could consider supporting this as two
 * devices to make the layout cleaner, but this will do for now.
 */
#ifndef FLASH_SECTOR_TOTAL
#error "Unknown flash layout"
#else  /* defined(FLASH_SECTOR_TOTAL) */
#if FLASH_SECTOR_TOTAL == 5
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/* RM0401, table 5: STM32F410Tx, STM32F410Cx, STM32F410Rx */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
};
#elif FLASH_SECTOR_TOTAL == 6
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/* RM0368, table 5: STM32F401xC */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 1, .pages_size = KB(128)},
};
#elif FLASH_SECTOR_TOTAL == 8
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/*
	 * RM0368, table 5: STM32F401xE
	 * RM0383, table 4: STM32F411xE
	 * RM0390, table 4: STM32F446xx
	 */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 3, .pages_size = KB(128)},
};
#elif FLASH_SECTOR_TOTAL == 12
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/*
	 * RM0090, table 5: STM32F405xx, STM32F415xx, STM32F407xx, STM32F417xx
	 * RM0402, table 5: STM32F412Zx, STM32F412Vx, STM32F412Rx, STM32F412Cx
	 */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#elif FLASH_SECTOR_TOTAL == 16
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/* RM0430, table 5.: STM32F413xx, STM32F423xx */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 11, .pages_size = KB(128)},
};
#elif FLASH_SECTOR_TOTAL == 24
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/*
	 * RM0090, table 6: STM32F427xx, STM32F437xx, STM32F429xx, STM32F439xx
	 * RM0386, table 4: STM32F469xx, STM32F479xx
	 */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#else
#error "Unknown flash layout"
#endif /* FLASH_SECTOR_TOTAL == 5 */
#endif/* !defined(FLASH_SECTOR_TOTAL) */

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = stm32f4_flash_layout;
	*layout_size = ARRAY_SIZE(stm32f4_flash_layout);
}
