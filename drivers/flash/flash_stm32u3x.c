/*
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32u3
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <soc.h>
#include <stm32_ll_icache.h>
#include <stm32_ll_system.h>
#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include "flash_stm32.h"

#define BANK2_OFFSET	(KB(CONFIG_FLASH_SIZE) / 2)

/* Macro to check if the flash is Dual bank or not */
#define stm32_flash_has_2_banks(flash_device) \
	((FLASH_STM32_REGS(flash_device)->OPTR & FLASH_STM32_DBANK) == FLASH_STM32_DBANK)

/*
 * offset and len must be aligned on write-block-size for write,
 * positive and not beyond end of flash
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
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
	if (regs->CR & FLASH_STM32_NSLOCK) {
		LOG_ERR("NSCR locked");
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
	regs->CR |= FLASH_STM32_NSPG;

	/* Flush the register write */
	tmp = regs->CR;

	/* Perform the data write operation at the desired memory address */
	for (i = 0; i < n; i++) {
		flash[i] = buff[i];
	}

	/* Wait until the NSBSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

	/* Clear the NSPG bit */
	regs->CR &= ~FLASH_STM32_NSPG;

	return rc;
}

static int erase_page(const struct device *dev, unsigned int offset)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;
	int page;

	/* if the non-secure control register is locked,do not fail silently */
	if (regs->CR & FLASH_STM32_NSLOCK) {
		LOG_ERR("NSCR locked");
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
		bank_swap = ((regs->OPTR & FLASH_OPTR_SWAP_BANK) == FLASH_OPTR_SWAP_BANK);

		if ((offset < (FLASH_SIZE / 2)) && !bank_swap) {
			/* The pages to be erased is in bank 1 */
			regs->CR &= ~FLASH_STM32_NSBKER_MSK;
			page = offset / FLASH_PAGE_SIZE;
			LOG_DBG("Erase page %d on bank 1", page);
		} else if ((offset >= BANK2_OFFSET) && bank_swap) {
			/* The pages to be erased is in bank 1 */
			regs->CR &= ~FLASH_STM32_NSBKER_MSK;
			page = (offset - BANK2_OFFSET) / FLASH_PAGE_SIZE;
			LOG_DBG("Erase page %d on bank 1", page);
		} else if ((offset < (FLASH_SIZE / 2)) && bank_swap) {
			/* The pages to be erased is in bank 2 */
			regs->CR |= FLASH_STM32_NSBKER;
			page = offset / FLASH_PAGE_SIZE;
			LOG_DBG("Erase page %d on bank 2", page);
		} else if ((offset >= BANK2_OFFSET) && !bank_swap) {
			/* The pages to be erased is in bank 2 */
			regs->CR |= FLASH_STM32_NSBKER;
			page = (offset - BANK2_OFFSET) / FLASH_PAGE_SIZE;
			LOG_DBG("Erase page %d on bank 2", page);
		} else {
			LOG_ERR("Offset %d does not exist", offset);
			return -EINVAL;
		}
	} else {
		page = offset / FLASH_PAGE_SIZE_128_BITS;
		LOG_DBG("Erase page %d", page);
	}

	/* Set the NSPER bit and select the page you wish to erase */
	regs->CR |= FLASH_STM32_NSPER;
	regs->CR &= ~FLASH_STM32_NSPNB_MSK;
	regs->CR |= page << FLASH_STM32_NSPNB_POS;

	/* Set the NSSTRT bit */
	regs->CR |= FLASH_STM32_NSSTRT;

	/* flush the register write */
	tmp = regs->CR;

	/* Wait for the NSBSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	if (stm32_flash_has_2_banks(dev)) {
		regs->CR &= ~(FLASH_STM32_NSPER | FLASH_STM32_NSBKER);
	} else {
		regs->CR &= ~(FLASH_STM32_NSPER);
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

	for (address = offset; address <= offset + len - 1; address += FLASH_PAGE_SIZE) {
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
		rc = write_nwords(dev, offset + i, ((const uint32_t *)data + (i >> 2)),
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
	static struct flash_pages_layout stm32_flash_layout[1];

	if (stm32_flash_layout[0].pages_count == 0) {
		if (stm32_flash_has_2_banks(dev)) {
			stm32_flash_layout[0].pages_count = FLASH_PAGE_NB * 2;
		} else {
			stm32_flash_layout[0].pages_count = FLASH_PAGE_NB;
		}
		stm32_flash_layout[0].pages_size = FLASH_PAGE_SIZE;
	}

	*layout = stm32_flash_layout;
	*layout_size = 1;
}
