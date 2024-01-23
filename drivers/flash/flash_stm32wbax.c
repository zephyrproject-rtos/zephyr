/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32wba
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <soc.h>
#include <stm32_ll_icache.h>
#include <stm32_ll_system.h>

#include "flash_stm32.h"

#define STM32_SERIES_MAX_FLASH	1024

#define ICACHE_DISABLE_TIMEOUT_VALUE           1U   /* 1ms */
#define ICACHE_INVALIDATE_TIMEOUT_VALUE        1U   /* 1ms */

static int stm32_icache_disable(void)
{
	int status = 0;
	uint32_t tickstart;

	LOG_DBG("I-cache Disable");
	/* Clear BSYENDF flag first and then disable the instruction cache
	 * that starts a cache invalidation procedure
	 */
	CLEAR_BIT(ICACHE->FCR, ICACHE_FCR_CBSYENDF);

	LL_ICACHE_Disable();

	/* Get tick */
	tickstart = k_uptime_get_32();

	/* Wait for instruction cache to get disabled */
	while (LL_ICACHE_IsEnabled()) {
		if ((k_uptime_get_32() - tickstart) >
						ICACHE_DISABLE_TIMEOUT_VALUE) {
			/* New check to avoid false timeout detection in case
			 * of preemption.
			 */
			if (LL_ICACHE_IsEnabled()) {
				status = -ETIMEDOUT;
				break;
			}
		}
	}

	return status;
}

static void stm32_icache_enable(void)
{
	LOG_DBG("I-cache Enable");
	LL_ICACHE_Enable();
}

static int icache_wait_for_invalidate_complete(void)
{
	int status = -EIO;
	uint32_t tickstart;

	/* Check if ongoing invalidation operation */
	if (LL_ICACHE_IsActiveFlag_BUSY()) {
		/* Get tick */
		tickstart = k_uptime_get_32();

		/* Wait for end of cache invalidation */
		while (!LL_ICACHE_IsActiveFlag_BSYEND()) {
			if ((k_uptime_get_32() - tickstart) >
					ICACHE_INVALIDATE_TIMEOUT_VALUE) {
				break;
			}
		}
	}

	/* Clear any pending flags */
	if (LL_ICACHE_IsActiveFlag_BSYEND()) {
		LOG_DBG("I-cache Invalidation complete");

		LL_ICACHE_ClearFlag_BSYEND();
		status = 0;
	} else {
		LOG_ERR("I-cache Invalidation timeout");

		status = -ETIMEDOUT;
	}

	if (LL_ICACHE_IsActiveFlag_ERR()) {
		LOG_ERR("I-cache error");

		LL_ICACHE_ClearFlag_ERR();
		status = -EIO;
	}

	return status;
}

static int write_qword(const struct device *dev, off_t offset, const uint32_t *buff)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	volatile uint32_t *flash = (uint32_t *)(offset
						+ FLASH_STM32_BASE_ADDRESS);
	uint32_t tmp;
	int rc;

	/* if the non-secure control register is locked, do not fail silently */
	if (regs->NSCR & FLASH_STM32_NSLOCK) {
		LOG_ERR("NSCR locked\n");
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check if this double word is erased */
	if ((flash[0] != 0xFFFFFFFFUL) || (flash[1] != 0xFFFFFFFFUL) ||
		(flash[2] != 0xFFFFFFFFUL) || (flash[3] != 0xFFFFFFFFUL)) {
		LOG_ERR("Word at offs %ld not erased", (long)offset);
		return -EIO;
	}

	/* Set the NSPG bit */
	regs->NSCR |= FLASH_STM32_NSPG;

	/* Flush the register write */
	tmp = regs->NSCR;

	/* Perform the data write operation at the desired memory address */
	flash[0] = buff[0];
	flash[1] = buff[1];
	flash[2] = buff[2];
	flash[3] = buff[3];

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

	page = offset / FLASH_PAGE_SIZE;
	LOG_DBG("Erase page %d\n", page);

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

	regs->NSCR &= ~(FLASH_STM32_NSPER);

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	unsigned int address = offset;
	int rc = 0;
	bool icache_enabled = LL_ICACHE_IsEnabled();

	if (icache_enabled) {
		/* Disable icache, this will start the invalidation procedure.
		 * All changes(erase/write) to flash memory should happen when
		 * i-cache is disabled. A write to flash performed without
		 * disabling i-cache will set ERRF error flag in SR register.
		 */
		rc = stm32_icache_disable();
		if (rc != 0) {
			return rc;
		}
	}

	for (; address <= offset + len - 1 ; address += FLASH_PAGE_SIZE) {
		rc = erase_page(dev, address);
		if (rc < 0) {
			break;
		}
	}

	if (icache_enabled) {
		/* Since i-cache was disabled, this would start the
		 * invalidation procedure, so wait for completion.
		 */
		rc = icache_wait_for_invalidate_complete();

		/* I-cache should be enabled only after the
		 * invalidation is complete.
		 */
		stm32_icache_enable();
	}

	return rc;
}

int flash_stm32_write_range(const struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;
	bool icache_enabled = LL_ICACHE_IsEnabled();

	if (icache_enabled) {
		/* Disable icache, this will start the invalidation procedure.
		 * All changes(erase/write) to flash memory should happen when
		 * i-cache is disabled. A write to flash performed without
		 * disabling i-cache will set ERRF error flag in SR register.
		 */
		rc = stm32_icache_disable();
		if (rc != 0) {
			return rc;
		}
	}

	for (i = 0; i < len; i += 16) {
		rc = write_qword(dev, offset + i, ((const uint32_t *) data + (i>>2)));
		if (rc < 0) {
			break;
		}
	}

	if (icache_enabled) {
		/* Since i-cache was disabled, this would start the
		 * invalidation procedure, so wait for completion.
		 */
		rc = icache_wait_for_invalidate_complete();

		/* I-cache should be enabled only after the
		 * invalidation is complete.
		 */
		stm32_icache_enable();
	}

	return rc;
}

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32wba_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32wba_flash_layout.pages_count == 0) {
		stm32wba_flash_layout.pages_count = FLASH_SIZE / FLASH_PAGE_SIZE;
		stm32wba_flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &stm32wba_flash_layout;
	*layout_size = 1;
}
