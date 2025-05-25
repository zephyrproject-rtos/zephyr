/*
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32l5
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/cache.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <soc.h>
#include <stm32_ll_icache.h>
#include <stm32_ll_system.h>

#include "flash_stm32.h"

#if defined(CONFIG_SOC_SERIES_STM32H5X) || defined(CONFIG_SOC_SERIES_STM32U5X)
/*
 * It is used to handle the 2 banks discontinuity case,
 * so define it to flash size to avoid the unexpected check.
 */
#define STM32_SERIES_MAX_FLASH	(CONFIG_FLASH_SIZE)
#elif defined(CONFIG_SOC_SERIES_STM32L5X)
#define STM32_SERIES_MAX_FLASH	512
#endif

#define PAGES_PER_BANK ((FLASH_SIZE / FLASH_PAGE_SIZE) / 2)

#define BANK2_OFFSET	(KB(STM32_SERIES_MAX_FLASH) / 2)

/* Macro to check if the flash is Dual bank or not */
#if defined(CONFIG_SOC_SERIES_STM32H5X)
#define stm32_flash_has_2_banks(flash_device) true
#else
#define stm32_flash_has_2_banks(flash_device) \
	(((FLASH_STM32_REGS(flash_device)->OPTR & FLASH_STM32_DBANK) \
	== FLASH_STM32_DBANK) \
	? (true) : (false))
#endif /* CONFIG_SOC_SERIES_STM32H5X */

/*
 * offset and len must be aligned on write-block-size for write,
 * positive and not beyond end of flash
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	if (stm32_flash_has_2_banks(dev) &&
			(CONFIG_FLASH_SIZE < STM32_SERIES_MAX_FLASH)) {
		/*
		 * In case of bank1/2 discontinuity, the range should not
		 * start before bank2 and end beyond bank1 at the same time.
		 * Locations beyond bank2 are caught by
		 * flash_stm32_range_exists.
		 */
		if ((offset < BANK2_OFFSET) &&
					(offset + len > FLASH_SIZE / 2)) {
			return 0;
		}
	}

	if (write && !flash_stm32_valid_write(offset, len)) {
		return false;
	}
	return flash_stm32_range_exists(dev, offset, len);
}

static int write_nwords(const struct device *dev, off_t offset, const uint32_t *buff, size_t n)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	volatile uint32_t *flash = (uint32_t *)(offset
						+ FLASH_STM32_BASE_ADDRESS);
	bool full_zero = true;
	uint32_t tmp;
	int rc;
	int i;

	/* if the non-secure control register is locked,do not fail silently */
	if (regs->NSCR & FLASH_STM32_NSLOCK) {
		LOG_ERR("NSCR locked\n");
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check if this double/quad word is erased and value isn't 0.
	 *
	 * It is allowed to write only zeros over an already written dword / qword
	 * See 6.3.7 in STM32L5 reference manual.
	 * See 7.3.7 in STM32U5 reference manual.
	 * See 7.3.5 in STM32H5 reference manual.
	 */
	for (i = 0; i < n; i++) {
		if (buff[i] != 0) {
			full_zero = false;
			break;
		}
	}
	if (!full_zero) {
		for (i = 0; i < n; i++) {
			if (flash[i] != 0xFFFFFFFFUL) {
				LOG_ERR("Word at offs %ld not erased", (long)(offset + i));
				return -EIO;
			}
		}
	}

	/* Set the NSPG bit */
	regs->NSCR |= FLASH_STM32_NSPG;

	/* Flush the register write */
	tmp = regs->NSCR;

	/* Perform the data write operation at the desired memory address */
	for (i = 0; i < n; i++) {
		flash[i] = buff[i];
	}

	/* Wait until the NSBSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

	/* Clear the NSPG bit */
	regs->NSCR &= (~FLASH_STM32_NSPG);

	return rc;
}

static int erase_page(const struct device *dev, unsigned int offset)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;
	int page;

	/* if the non-secure control register is locked,do not fail silently */
	if (regs->NSCR & FLASH_STM32_NSLOCK) {
		LOG_ERR("NSCR locked\n");
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	if (stm32_flash_has_2_banks(dev)) {
		bool bank_swap;
		/* Check whether bank1/2 are swapped */
		bank_swap =
		((regs->OPTR & FLASH_OPTR_SWAP_BANK) == FLASH_OPTR_SWAP_BANK);

		if ((offset < (FLASH_SIZE / 2)) && !bank_swap) {
			/* The pages to be erased is in bank 1 */
			regs->NSCR &= ~FLASH_STM32_NSBKER_MSK;
			page = offset / FLASH_PAGE_SIZE;
			LOG_DBG("Erase page %d on bank 1", page);
		} else if ((offset >= BANK2_OFFSET) && bank_swap) {
			/* The pages to be erased is in bank 1 */
			regs->NSCR &= ~FLASH_STM32_NSBKER_MSK;
			page = (offset - BANK2_OFFSET) / FLASH_PAGE_SIZE;
			LOG_DBG("Erase page %d on bank 1", page);
		} else if ((offset < (FLASH_SIZE / 2)) && bank_swap) {
			/* The pages to be erased is in bank 2 */
			regs->NSCR |= FLASH_STM32_NSBKER;
			page = offset / FLASH_PAGE_SIZE;
			LOG_DBG("Erase page %d on bank 2", page);
		} else if ((offset >= BANK2_OFFSET) && !bank_swap) {
			/* The pages to be erased is in bank 2 */
			regs->NSCR |= FLASH_STM32_NSBKER;
			page = (offset - BANK2_OFFSET) / FLASH_PAGE_SIZE;
			LOG_DBG("Erase page %d on bank 2", page);
		} else {
			LOG_ERR("Offset %d does not exist", offset);
			return -EINVAL;
		}
	} else {
		page = offset / FLASH_PAGE_SIZE_128_BITS;
		LOG_DBG("Erase page %d\n", page);
	}

	/* Set the NSPER bit and select the page you wish to erase */
	regs->NSCR |= FLASH_STM32_NSPER;
	regs->NSCR &= ~FLASH_STM32_NSPNB_MSK;
	regs->NSCR |= (page << FLASH_STM32_NSPNB_POS);

	/* Set the NSSTRT bit */
	regs->NSCR |= FLASH_STM32_NSSTRT;

	/* flush the register write */
	tmp = regs->NSCR;

	/* Wait for the NSBSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	if (stm32_flash_has_2_banks(dev)) {
		regs->NSCR &= ~(FLASH_STM32_NSPER | FLASH_STM32_NSBKER);
	} else {
		regs->NSCR &= ~(FLASH_STM32_NSPER);
	}

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	unsigned int address = offset;
	int rc = 0;

	/* Disable icache, this will start the invalidation procedure.
	 * All changes(erase/write) to flash memory should happen when
	 * i-cache is disabled. A write to flash performed without
	 * disabling i-cache will set ERRF error flag in SR register.
	 */

	bool cache_enabled = LL_ICACHE_IsEnabled();

	sys_cache_instr_disable();

	for (; address <= offset + len - 1 ; address += FLASH_PAGE_SIZE) {
		rc = erase_page(dev, address);
		if (rc < 0) {
			break;
		}
	}

	if (cache_enabled) {
		sys_cache_instr_enable();
	}

	return rc;
}

int flash_stm32_write_range(const struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;

	/* Disable icache, this will start the invalidation procedure.
	 * All changes(erase/write) to flash memory should happen when
	 * i-cache is disabled. A write to flash performed without
	 * disabling i-cache will set ERRF error flag in SR register.
	 */

	bool cache_enabled = LL_ICACHE_IsEnabled();

	sys_cache_instr_disable();

	for (i = 0; i < len; i += FLASH_STM32_WRITE_BLOCK_SIZE) {
		rc = write_nwords(dev, offset + i, ((const uint32_t *) data + (i>>2)),
				  FLASH_STM32_WRITE_BLOCK_SIZE / 4);
		if (rc < 0) {
			break;
		}
	}

	if (cache_enabled) {
		sys_cache_instr_enable();
	}

	return rc;
}

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32_flash_layout[3];
	static size_t stm32_flash_layout_size;

	*layout = stm32_flash_layout;

	if (stm32_flash_layout[0].pages_count != 0) {
		/* Short circuit calculation logic if already performed (size is known) */
		*layout_size = stm32_flash_layout_size;
		return;
	}

	if (stm32_flash_has_2_banks(dev) &&
			(CONFIG_FLASH_SIZE < STM32_SERIES_MAX_FLASH)) {
		/*
		 * For stm32l552xx with 256 kB flash
		 * which have space between banks 1 and 2.
		 */

		/* Bank1 */
		stm32_flash_layout[0].pages_count = PAGES_PER_BANK;
		stm32_flash_layout[0].pages_size = FLASH_PAGE_SIZE;

		/* Dummy page corresponding to space between banks 1 and 2 */
		stm32_flash_layout[1].pages_count = 1;
		stm32_flash_layout[1].pages_size = BANK2_OFFSET
				- (PAGES_PER_BANK * FLASH_PAGE_SIZE);

		/* Bank2 */
		stm32_flash_layout[2].pages_count = PAGES_PER_BANK;
		stm32_flash_layout[2].pages_size = FLASH_PAGE_SIZE;

		stm32_flash_layout_size = ARRAY_SIZE(stm32_flash_layout);
	} else {
		/*
		 * For stm32l562xx & stm32l552xx with 512 flash or stm32u5x,
		 * which has no space between banks 1 and 2.
		 */

		if (stm32_flash_has_2_banks(dev)) {
			/* L5 flash with dualbank has 2k pages */
			/* U5 flash pages are always 8 kB in size */
			/* H5 flash pages are always 8 kB in size */
			/* Considering one layout of full flash size, even with 2 banks */
			stm32_flash_layout[0].pages_count = FLASH_SIZE / FLASH_PAGE_SIZE;
			stm32_flash_layout[0].pages_size = FLASH_PAGE_SIZE;
#if defined(CONFIG_SOC_SERIES_STM32L5X)
		} else {
			/* L5 flash without dualbank has 4k pages */
			stm32_flash_layout[0].pages_count = FLASH_PAGE_NB_128_BITS;
			stm32_flash_layout[0].pages_size = FLASH_PAGE_SIZE_128_BITS;
#endif /* CONFIG_SOC_SERIES_STM32L5X */
		}

		/*
		 * In this case the stm32_flash_layout table has one single element
		 * when read by the flash_get_page_info()
		 */
		stm32_flash_layout_size = 1;
	}

	*layout_size = stm32_flash_layout_size;
}
