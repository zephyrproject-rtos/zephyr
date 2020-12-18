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
#include <stm32_ll_system.h>

#include "flash_stm32.h"

#define STM32G4_SERIES_MAX_FLASH	512
#define BANK2_OFFSET	(KB(STM32G4_SERIES_MAX_FLASH) / 2)

/*
 * offset and len must be aligned on 8 for write,
 * positive and not beyond end of flash
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{

#if defined(FLASH_OPTR_DBANK) && (CONFIG_FLASH_SIZE < STM32G4_SERIES_MAX_FLASH)
	/*
	 * In case of bank1/2 discontinuity, the range should not
	 * start before bank2 and end beyond bank1 at the same time.
	 * Locations beyond bank2 are caught by flash_stm32_range_exists.
	 */
	if ((offset < BANK2_OFFSET) && (offset + len > FLASH_SIZE / 2)) {
		return 0;
	}
#endif

	return (!write || (offset % 8 == 0 && len % 8 == 0U)) &&
		flash_stm32_range_exists(dev, offset, len);
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
		LOG_ERR("Word at offs %ld not erased", (long)offset);
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

static int erase_page(const struct device *dev, unsigned int offset)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;
	int page;

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

#if defined(FLASH_OPTR_DBANK)
	bool bank_swap;
	/* Check whether bank1/2 are swapped */
	bank_swap = (LL_SYSCFG_GetFlashBankMode() == LL_SYSCFG_BANKMODE_BANK2);

	if ((offset < (FLASH_SIZE / 2)) && !bank_swap) {
		/* The pages to be erased is in bank 1 */
		regs->CR &= ~FLASH_CR_BKER_Msk;
		page = offset / FLASH_PAGE_SIZE;
		LOG_DBG("Erase page %d on bank 1", page);
	} else if ((offset >= BANK2_OFFSET) && bank_swap) {
		/* The pages to be erased is in bank 1 */
		regs->CR &= ~FLASH_CR_BKER_Msk;
		page = (offset - BANK2_OFFSET) / FLASH_PAGE_SIZE;
		LOG_DBG("Erase page %d on bank 1", page);
	} else if ((offset < (FLASH_SIZE / 2)) && bank_swap) {
		/* The pages to be erased is in bank 2 */
		regs->CR |= FLASH_CR_BKER;
		page = offset / FLASH_PAGE_SIZE;
		LOG_DBG("Erase page %d on bank 2", page);
	} else if ((offset >= BANK2_OFFSET) && !bank_swap) {
		/* The pages to be erased is in bank 2 */
		regs->CR |= FLASH_CR_BKER;
		page = (offset - BANK2_OFFSET) / FLASH_PAGE_SIZE;
		LOG_DBG("Erase page %d on bank 2", page);
	} else {
		LOG_ERR("Offset %d does not exist", offset);
		return -EINVAL;
	}
#else
	page = offset / FLASH_PAGE_SIZE;
		LOG_DBG("Erase page %d", page);
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
	unsigned int address = offset, rc = 0;

	for (; address <= offset + len - 1 ; address += FLASH_PAGE_SIZE) {
		rc = erase_page(dev, address);
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
	ARG_UNUSED(dev);

#if defined(FLASH_OPTR_DBANK) && (CONFIG_FLASH_SIZE < STM32G4_SERIES_MAX_FLASH)
#define PAGES_PER_BANK  ((FLASH_SIZE / FLASH_PAGE_SIZE) / 2)
	static struct flash_pages_layout stm32g4_flash_layout[3];

	if (stm32g4_flash_layout[0].pages_count == 0) {
		/* Bank1 */
		stm32g4_flash_layout[0].pages_count = PAGES_PER_BANK;
		stm32g4_flash_layout[0].pages_size = FLASH_PAGE_SIZE;
		/* Dummy page corresponding to discontinuity between bank1/2 */
		stm32g4_flash_layout[1].pages_count = 1;
		stm32g4_flash_layout[1].pages_size = BANK2_OFFSET
					- (PAGES_PER_BANK * FLASH_PAGE_SIZE);
		/* Bank2 */
		stm32g4_flash_layout[2].pages_count = PAGES_PER_BANK;
		stm32g4_flash_layout[2].pages_size = FLASH_PAGE_SIZE;
	}
#else
	static struct flash_pages_layout stm32g4_flash_layout[1];

	if (stm32g4_flash_layout[0].pages_count == 0) {
		stm32g4_flash_layout[0].pages_count = FLASH_SIZE
						/ FLASH_PAGE_SIZE;
		stm32g4_flash_layout[0].pages_size = FLASH_PAGE_SIZE;
	}
#endif

	*layout = stm32g4_flash_layout;
	*layout_size = ARRAY_SIZE(stm32g4_flash_layout);
}

/* Override weak function */
int  flash_stm32_check_configuration(void)
{
#if defined(FLASH_OPTR_DBANK)
	if (READ_BIT(FLASH->OPTR, FLASH_OPTR_DBANK) == 0U) {
		/* Single bank not supported when dualbank is possible */
		LOG_ERR("Single bank configuration not supported");
		return -ENOTSUP;
	}
#endif
	return 0;
}
