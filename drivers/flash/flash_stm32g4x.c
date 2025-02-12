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

#if defined(FLASH_STM32_DBANK) && (CONFIG_FLASH_SIZE < STM32G4_SERIES_MAX_FLASH)
	/*
	 * In case of bank1/2 discontinuity, the range should not
	 * start before bank2 and end beyond bank1 at the same time.
	 * Locations beyond bank2 are caught by flash_stm32_range_exists.
	 */
	if ((offset < BANK2_OFFSET) && (offset + len > FLASH_SIZE / 2)) {
		return 0;
	}
#endif

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
#if defined(FLASH_STM32_DBANK)
	bool dcache_enabled = false;
#endif /* FLASH_STM32_DBANK */
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

	/* Check if this double word is erased and value isn't 0.
	 *
	 * It is allowed to write only zeros over an already written dword
	 * See 3.3.7 in reference manual.
	 */
	if ((flash[0] != 0xFFFFFFFFUL ||
	    flash[1] != 0xFFFFFFFFUL) && val != 0UL) {
		LOG_ERR("Word at offs %ld not erased", (long)offset);
		return -EIO;
	}

#if defined(FLASH_STM32_DBANK)
	/*
	 * Disable the data cache to avoid the silicon errata ES0430 Rev 7 2.2.2:
	 * "Data cache might be corrupted during Flash memory read-while-write operation"
	 */
	if (regs->ACR & FLASH_ACR_DCEN) {
		dcache_enabled = true;
		regs->ACR &= (~FLASH_ACR_DCEN);
	}
#endif /* FLASH_STM32_DBANK */

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

#if defined(FLASH_STM32_DBANK)
	/* Reset/enable the data cache if previously enabled */
	if (dcache_enabled) {
		regs->ACR |= FLASH_ACR_DCRST;
		regs->ACR &= (~FLASH_ACR_DCRST);
		regs->ACR |= FLASH_ACR_DCEN;
	}
#endif /* FLASH_STM32_DBANK */

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

#if defined(FLASH_STM32_DBANK)
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

	flush_cache(regs);

#ifdef FLASH_STM32_DBANK
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
		rc = write_dword(dev, offset, ((const uint64_t *) data)[i>>3]);
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

static __unused int write_optb(const struct device *dev, uint32_t mask,
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

	return 0;
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
int flash_stm32_update_rdp(const struct device *dev, bool enable,
			   bool permanent)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint8_t current_level, target_level;

	current_level =
		(regs->OPTR & FLASH_OPTR_RDP_Msk) >> FLASH_OPTR_RDP_Pos;
	target_level = current_level;

	/*
	 * 0xAA = RDP level 0 (no protection)
	 * 0xCC = RDP level 2 (permanent protection)
	 * others = RDP level 1 (protection active)
	 */
	switch (current_level) {
	case FLASH_STM32_RDP2:
		if (!enable || !permanent) {
			LOG_ERR("RDP level 2 is permanent and can't be changed!");
			return -ENOTSUP;
		}
		break;
	case FLASH_STM32_RDP0:
		if (enable) {
			target_level = FLASH_STM32_RDP1;
			if (permanent) {
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION_PERMANENT_ALLOW)
				target_level = FLASH_STM32_RDP2;
#else
				LOG_ERR("Permanent readout protection (RDP "
					"level 0 -> 2) not allowed");
				return -ENOTSUP;
#endif
			}
		}
		break;
	default: /* FLASH_STM32_RDP1 */
		if (enable && permanent) {
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION_PERMANENT_ALLOW)
			target_level = FLASH_STM32_RDP2;
#else
			LOG_ERR("Permanent readout protection (RDP "
				"level 1 -> 2) not allowed");
			return -ENOTSUP;
#endif
		}
		if (!enable) {
#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION_DISABLE_ALLOW)
			target_level = FLASH_STM32_RDP0;
#else
			LOG_ERR("Disabling readout protection (RDP "
				"level 1 -> 0) not allowed");
			return -EACCES;
#endif
		}
	}

	/* Update RDP level if needed */
	if (current_level != target_level) {
		LOG_INF("RDP changed from 0x%02x to 0x%02x", current_level,
			target_level);

		write_optb(dev, FLASH_OPTR_RDP_Msk,
			   (uint32_t)target_level << FLASH_OPTR_RDP_Pos);
	}
	return 0;
}

int flash_stm32_get_rdp(const struct device *dev, bool *enabled,
			bool *permanent)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint8_t current_level;

	current_level =
		(regs->OPTR & FLASH_OPTR_RDP_Msk) >> FLASH_OPTR_RDP_Pos;

	/*
	 * 0xAA = RDP level 0 (no protection)
	 * 0xCC = RDP level 2 (permanent protection)
	 * others = RDP level 1 (protection active)
	 */
	switch (current_level) {
	case FLASH_STM32_RDP2:
		*enabled = true;
		*permanent = true;
		break;
	case FLASH_STM32_RDP0:
		*enabled = false;
		*permanent = false;
		break;
	default: /* FLASH_STM32_RDP1 */
		*enabled = true;
		*permanent = false;
	}
	return 0;
}
#endif /* CONFIG_FLASH_STM32_READOUT_PROTECTION */

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

#if defined(FLASH_STM32_DBANK) && (CONFIG_FLASH_SIZE < STM32G4_SERIES_MAX_FLASH)
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
#if defined(FLASH_STM32_DBANK)
	if (READ_BIT(FLASH->OPTR, FLASH_STM32_DBANK) == 0U) {
		/* Single bank not supported when dualbank is possible */
		LOG_ERR("Single bank configuration not supported");
		return -ENOTSUP;
	}
#endif
	return 0;
}
