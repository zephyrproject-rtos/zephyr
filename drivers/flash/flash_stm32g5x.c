/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_flash.h>
#include <stm32_ll_icache.h>
#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32g5, CONFIG_FLASH_LOG_LEVEL);

#include "flash_stm32.h"

#define G5_FLASH_SIZE DT_REG_SIZE(DT_INST(0, st_stm32_nv_flash))
#define G5_FLASH_BANK_SIZE (G5_FLASH_SIZE / FLASH_BANK_NB)

static int flash_stm32g5_check_status(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

	/* Report controller errors, including write protection, and clear the flags. */
	if (LL_FLASH_IsActiveFlag(regs, LL_FLASH_FLAG_ERRORS_ALL) != 0U) {
		LL_FLASH_ClearFlag(regs, LL_FLASH_FLAG_ERRORS_ALL);
		return -EIO;
	}

	return 0;
}

static int flash_stm32g5_wait_idle(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(STM32_FLASH_TIMEOUT));
	bool expired = false;

	/* Wait for BSY to clear and for both buffers (WBNE and DBNE) to empty. */
	while (LL_FLASH_IsActiveFlag(regs, LL_FLASH_FLAG_STATUS_ALL) != 0U) {
		if (expired) {
			LOG_ERR("Flash operation timeout");
			return -EIO;
		}

		expired = sys_timepoint_expired(timeout);
	}

	return flash_stm32g5_check_status(dev);
}

/*
 * The range must fit in main flash. Writes must also align the offset and
 * length to the write block size.
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	if (write && !flash_stm32_valid_write(offset, len)) {
		return false;
	}

	return offset >= 0 && (uint64_t)offset <= G5_FLASH_SIZE &&
	       len <= G5_FLASH_SIZE - (uint64_t)offset;
}

static int write_quadword(const struct device *dev, off_t offset, const void *data)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	volatile uint32_t *flash = (uint32_t *)(offset + FLASH_STM32_BASE_ADDRESS);
	uint32_t words[4];
	int rc;

	BUILD_ASSERT(sizeof(words) == FLASH_STM32_WRITE_BLOCK_SIZE);

	if (LL_FLASH_IsLocked(regs)) {
		LOG_ERR("CR locked");
		return -EIO;
	}

	/* Check controller status before setting PG (RM0514 section 7.3.5). */
	rc = flash_stm32g5_wait_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Only program an erased flash word. */
	for (size_t index = 0U; index < ARRAY_SIZE(words); index++) {
		if (flash[index] != UINT32_MAX) {
			return -EIO;
		}
	}

	/* Stage the full word before setting PG */
	memcpy(words, data, sizeof(words));
	LL_FLASH_EnableProgramming(regs);
	if (LL_FLASH_IsEnabledProgramming(regs) == 0U) {
		LOG_ERR("Error enabling flash programming");
		rc = -EIO;
		goto end;
	}

	/* Fill the 128-bit write buffer with four consecutive 32-bit stores. */
	for (size_t index = 0U; index < ARRAY_SIZE(words); index++) {
		flash[index] = words[index];
	}
	/* Complete the stores before polling the controller. */
	barrier_dsync_fence_full();

	rc = flash_stm32g5_wait_idle(dev);

end:
	/* Clear PG only when the control register is writable. */
	if (LL_FLASH_IsActiveFlag(regs, LL_FLASH_FLAG_BSY) == 0U) {
		LL_FLASH_DisableProgramming(regs);
	}

	return rc;
}

static int erase_page(const struct device *dev, unsigned int offset)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	/* Bank swapping is not handled. Page numbers are relative to the physical bank. */
	uint32_t bank = (offset >= G5_FLASH_BANK_SIZE) ? LL_FLASH_ERASE_BANK_2
						       : LL_FLASH_ERASE_BANK_1;
	uint32_t page = (offset % G5_FLASH_BANK_SIZE) / FLASH_PAGE_SIZE;
	int rc;

	if (LL_FLASH_IsLocked(regs)) {
		LOG_ERR("CR locked");
		return -EIO;
	}

	rc = flash_stm32g5_wait_idle(dev);
	if (rc < 0) {
		return rc;
	}

	LOG_DBG("Erase page %u", page);

	/* Set BKSEL, PNB and PER, then start the erase (RM0514 section 7.3.6). */
	LL_FLASH_StartErasePage(regs, bank, page);
	/* Complete the register write before polling for erase completion. */
	barrier_dsync_fence_full();

	rc = flash_stm32g5_wait_idle(dev);

	/* Hardware clears STRT; clear PER only after BSY drops. */
	if (LL_FLASH_IsActiveFlag(regs, LL_FLASH_FLAG_BSY) == 0U) {
		LL_FLASH_DisablePageErase(regs);
	}

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	unsigned int address;
	int rc = 0;
	bool cache_enabled;

	/* Reject partial pages before starting any erase. */
	if ((offset % FLASH_PAGE_SIZE) != 0U || (len == 0U) ||
	    (len % FLASH_PAGE_SIZE) != 0U || !flash_stm32_valid_range(dev, offset, len, false)) {
		return -EINVAL;
	}

	/* Keep ICACHE disabled for the erase loop and restore its previous state. */
	cache_enabled = LL_ICACHE_IsEnabled(ICACHE);

	sys_cache_instr_disable();

	for (address = offset; address < offset + len; address += FLASH_PAGE_SIZE) {
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
	const uint8_t *source = data;
	/* Keep ICACHE disabled while programming and restore its previous state. */
	bool cache_enabled = LL_ICACHE_IsEnabled(ICACHE);
	int rc = 0;

	sys_cache_instr_disable();

	for (unsigned int index = 0U; index < len; index += FLASH_STM32_WRITE_BLOCK_SIZE) {
		rc = write_quadword(dev, offset + index, source + index);
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
	/* Both main flash banks use the same erase page size. */
	static const struct flash_pages_layout stm32_flash_layout = {
		.pages_count = G5_FLASH_SIZE / FLASH_PAGE_SIZE,
		.pages_size = FLASH_PAGE_SIZE,
	};

	*layout = &stm32_flash_layout;
	*layout_size = 1;
}
