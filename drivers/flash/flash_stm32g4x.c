/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32g4
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/init.h>
#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_ll_system.h>

#include "flash_stm32.h"

#define STM32G4_SERIES_MAX_FLASH 512
#define BANK2_OFFSET             (KB(STM32G4_SERIES_MAX_FLASH) / 2)
#define DBANK_PAGES_PER_BANK     ((FLASH_SIZE / FLASH_PAGE_SIZE) / 2)

#if defined(FLASH_STM32_DBANK)
#define IS_DUAL_BANK(flash_device)                                                                 \
	((FLASH_STM32_REGS(flash_device)->OPTR & FLASH_OPTR_DBANK) == FLASH_OPTR_DBANK)
#else
/* If FLASH_STM32_DBANK is not defined this chip doesn't support dual bank operation */
#define IS_DUAL_BANK(flash_device) false
#endif

/*
 * offset and len must be aligned on 8 for write,
 * positive and not beyond end of flash
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset, uint32_t len, bool write)
{
	if (IS_DUAL_BANK(dev) && (CONFIG_FLASH_SIZE < STM32G4_SERIES_MAX_FLASH)) {
		/*
		 * In case of bank1/2 discontinuity, the range should not
		 * start before bank2 and end beyond bank1 at the same time.
		 * Locations beyond bank2 are caught by flash_stm32_range_exists.
		 */
		if ((offset < BANK2_OFFSET) && (offset + len > FLASH_SIZE / 2)) {
			return false;
		}
	}

	if (write && !flash_stm32_valid_write(offset, len)) {
		return false;
	}
	return flash_stm32_range_exists(dev, offset, len);
}

static inline void flush_cache(FLASH_TypeDef *regs)
{
	if (regs->ACR & FLASH_ACR_DCEN) {
		regs->ACR &= ~FLASH_ACR_DCEN;
		/* Datasheet: DCRST: Data cache reset
		 * This bit can be written only when the data cache is disabled
		 */
		regs->ACR |= FLASH_ACR_DCRST;
		regs->ACR &= ~FLASH_ACR_DCRST;
		regs->ACR |= FLASH_ACR_DCEN;
	}

	if (regs->ACR & FLASH_ACR_ICEN) {
		regs->ACR &= ~FLASH_ACR_ICEN;
		/* Datasheet: ICRST: Instruction cache reset :
		 * This bit can be written only when the instruction cache
		 * is disabled
		 */
		regs->ACR |= FLASH_ACR_ICRST;
		regs->ACR &= ~FLASH_ACR_ICRST;
		regs->ACR |= FLASH_ACR_ICEN;
	}
}

static int write_dword(const struct device *dev, off_t offset, uint64_t val)
{
	volatile uint32_t *flash = (uint32_t *)(offset + FLASH_STM32_BASE_ADDRESS);
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	bool dcache_enabled = false;
	uint32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		LOG_ERROR("CR locked");
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check if this double word is erased and value isn't 0.
	 *
	 * It is allowed to write only zeros over an already written dword
	 * See 3.3.7 in reference manual.
	 */
	if ((flash[0] != 0xFFFFFFFFUL ||
	    flash[1] != 0xFFFFFFFFUL) && val != 0UL) {
		LOG_ERROR("Word at offs %ld not erased", (long)offset);
		return -EIO;
	}

	if (IS_DUAL_BANK(dev)) {
		/*
		 * Disable the data cache to avoid the silicon errata ES0430 Rev 7 2.2.2:
		 * "Data cache might be corrupted during Flash memory read-while-write operation"
		 */
		if (regs->ACR & FLASH_ACR_DCEN) {
			dcache_enabled = true;
			regs->ACR &= (~FLASH_ACR_DCEN);
		}
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

	/* Reset/enable the data cache if previously enabled */
	if (dcache_enabled) {
		regs->ACR |= FLASH_ACR_DCRST;
		regs->ACR &= (~FLASH_ACR_DCRST);
		regs->ACR |= FLASH_ACR_DCEN;
	}

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
		LOG_ERROR("CR locked");
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

#if defined(FLASH_STM32_DBANK)
	if (IS_DUAL_BANK(dev)) {
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
			LOG_ERROR("Offset %d does not exist", offset);
			return -EINVAL;
		}
	} else {
		page = offset / FLASH_PAGE_SIZE_128_BITS;
		LOG_DBG("Erase page %d", page);
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

	flush_cache(regs);

#ifdef FLASH_STM32_DBANK
	if (IS_DUAL_BANK(dev)) {
		regs->CR &= ~(FLASH_CR_PER | FLASH_CR_BKER);
	} else {
		regs->CR &= ~(FLASH_CR_PER);
	}
#else
	regs->CR &= ~(FLASH_CR_PER);
#endif

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	unsigned int address = offset;
	int rc = 0;

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
		rc = write_dword(dev, offset, UNALIGNED_GET((const uint64_t *) data + (i>>3)));
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

int flash_stm32_option_bytes_write(const struct device *dev, uint32_t mask,
				   uint32_t value)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	int rc;

	if (regs->CR & FLASH_CR_OPTLOCK) {
		return -EIO;
	}

	if ((regs->OPTR & mask) == value) {
		return 0;
	}

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	regs->OPTR = (regs->OPTR & ~mask) | value;
	regs->CR |= FLASH_CR_OPTSTRT;

	/* Make sure previous write is completed. */
	barrier_dsync_fence_full();

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Force the option byte loading */
	regs->CR |= FLASH_CR_OBL_LAUNCH;

	return flash_stm32_wait_flash_idle(dev);
}

uint32_t flash_stm32_option_bytes_read(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

	return regs->OPTR;
}

#if defined(CONFIG_FLASH_STM32_WRITE_PROTECT)

/*
 * Remark for future development implementing Write Protection for the L4 parts:
 *
 * STM32L4 allows for 2 write protected memory areas, c.f. FLASH_WEP1AR, FLASH_WRP1BR
 * which are defined by their start and end pages.
 *
 * Other STM32 parts (i.e. F4 series) uses bitmask to select sectors.
 *
 * To implement Write Protection for L4 one should thus add a new EX_OP like
 * FLASH_STM32_EX_OP_SECTOR_WP_RANGED in stm32_flash_api_extensions.h
 */

#endif /* CONFIG_FLASH_STM32_WRITE_PROTECT */

#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION)
uint8_t flash_stm32_get_rdp_level(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

	return (regs->OPTR & FLASH_OPTR_RDP_Msk) >> FLASH_OPTR_RDP_Pos;
}

void flash_stm32_set_rdp_level(const struct device *dev, uint8_t level)
{
	flash_stm32_option_bytes_write(dev, FLASH_OPTR_RDP_Msk,
				       (uint32_t)level << FLASH_OPTR_RDP_Pos);
}
#endif /* CONFIG_FLASH_STM32_READOUT_PROTECTION */

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32g4_flash_layout[3];

	if (IS_DUAL_BANK(dev) && (CONFIG_FLASH_SIZE < STM32G4_SERIES_MAX_FLASH)) {

		/*
		 * For category 3 devices with less than 512 kB flash
		 * which have space between banks 1 and 2.
		 */

		if (stm32g4_flash_layout[0].pages_count == 0) {
			/* Bank1 */
			stm32g4_flash_layout[0].pages_count = DBANK_PAGES_PER_BANK;
			stm32g4_flash_layout[0].pages_size = FLASH_PAGE_SIZE;
			/* Dummy page corresponding to discontinuity between bank1/2 */
			stm32g4_flash_layout[1].pages_count = 1;
			stm32g4_flash_layout[1].pages_size =
				BANK2_OFFSET - (DBANK_PAGES_PER_BANK * FLASH_PAGE_SIZE);
			/* Bank2 */
			stm32g4_flash_layout[2].pages_count = DBANK_PAGES_PER_BANK;
			stm32g4_flash_layout[2].pages_size = FLASH_PAGE_SIZE;
		}
	} else {
		/*
		 * For category 3 devices with 512 kB flash or category 2/4
		 * devices, which have no space between banks 1 and 2.
		 */

		if (stm32g4_flash_layout[0].pages_count == 0) {
#if defined(FLASH_STM32_DBANK)
			/* Dual or single bank with 2kB or 4kB  page size respectively */
			stm32g4_flash_layout[0].pages_size =
				(IS_DUAL_BANK(dev) ? FLASH_PAGE_SIZE : FLASH_PAGE_SIZE_128_BITS);
#else
			/* Single bank (non category 3) with 2k page size */
			stm32g4_flash_layout[0].pages_size = FLASH_PAGE_SIZE;
#endif
			stm32g4_flash_layout[0].pages_count =
				FLASH_SIZE / stm32g4_flash_layout[0].pages_size;
		}
	}

	*layout = stm32g4_flash_layout;
	*layout_size = IS_DUAL_BANK(dev) && (CONFIG_FLASH_SIZE != STM32G4_SERIES_MAX_FLASH)
			       ? ARRAY_SIZE(stm32g4_flash_layout)
			       : 1;
}
