/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32c5
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
	volatile uint32_t *flash = (uint32_t *)(offset + FLASH_STM32_BASE_ADDRESS);
	int rc;
	int i;

	/* if the non-secure control register is locked,do not fail silently */
	if (LL_FLASH_IsLocked(regs)) {
		LOG_ERR("NSCR locked");
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check if this double/quad word is erased.
	 * From RM0522 6.3.5: "Nothing prevents overwriting a nonvirgin FLASH word but this is not
	 * recommended. The result may lead to invalid data and inconsistent ECC code."
	 * To prevent errors, only write if erased.
	 */
	for (i = 0; i < n; i++) {
		if (flash[i] != 0xFFFFFFFFU) {
			return -EIO;
		}
	}

	/* Set the NSPG bit */
	LL_FLASH_EnableProgramming(regs);
	if (LL_FLASH_IsEnabledProgramming(regs) == 0) {
		LOG_ERR("Error enabling flash programming");
		rc = -EIO;
		goto end;
	}

	/* Perform the data write operation at the desired memory address */
	for (i = 0; i < n; i++) {
		flash[i] = buff[i];
	}

	/* Wait until the NSBSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

end:
	/* Clear the NSPG bit */
	LL_FLASH_DisableProgramming(regs);

	return rc;
}

static int erase_page(const struct device *dev, unsigned int offset)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t bank_swap = LL_FLASH_OB_IsBankSwapped(regs);
	uint32_t bank;
	uint32_t tmp;
	int rc;
	int page;

	/* if the non-secure control register is locked,do not fail silently */
	if (LL_FLASH_IsLocked(regs)) {
		LOG_ERR("NSCR locked");
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	if (offset < (FLASH_SIZE / 2)) {
		bank = bank_swap ? LL_FLASH_ERASE_BANK_2 : LL_FLASH_ERASE_BANK_1;
		page = offset / FLASH_PAGE_SIZE;
	} else if (offset < FLASH_SIZE) {
		bank = bank_swap ? LL_FLASH_ERASE_BANK_1 : LL_FLASH_ERASE_BANK_2;
		page = (offset - FLASH_SIZE / 2) / FLASH_PAGE_SIZE;
	} else {
		LOG_ERR("Offset %d does not exist", offset);
		return -EINVAL;
	}
	LOG_DBG("Erase page %d on bank %d", page, bank == LL_FLASH_ERASE_BANK_1 ? 1 : 2);

	/* Start erase */
	LL_FLASH_StartErasePage(regs, bank, LL_FLASH_ERASE_USER_AREA, page);

	/* Flush the register write */
	tmp = regs->CR;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	LL_FLASH_DisablePageErase(regs);

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

	bool cache_enabled = LL_ICACHE_IsEnabled(ICACHE);

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

	bool cache_enabled = LL_ICACHE_IsEnabled(ICACHE);

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
		bool cache_enabled = LL_ICACHE_IsEnabled(ICACHE);

		sys_cache_instr_disable();
		stm32_flash_layout[0].pages_count = FLASH_PAGE_NB * FLASH_BANK_NB;
		stm32_flash_layout[0].pages_size = FLASH_PAGE_SIZE;
		if (cache_enabled) {
			sys_cache_instr_enable();
		}
	}

	*layout = stm32_flash_layout;
	*layout_size = 1;
}
