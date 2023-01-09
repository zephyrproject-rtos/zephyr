/*
 * Copyright (c) 2017 BayLibre, SAS
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32generic, CONFIG_FLASH_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <soc.h>

#include "flash_stm32.h"

#if FLASH_STM32_WRITE_BLOCK_SIZE == 8
typedef uint64_t flash_prg_t;
#elif FLASH_STM32_WRITE_BLOCK_SIZE == 4
typedef uint32_t flash_prg_t;
#elif FLASH_STM32_WRITE_BLOCK_SIZE == 2
typedef uint16_t flash_prg_t;
#else
#error Unknown write block size
#endif

#if defined(FLASH_CR_PER)
#define FLASH_ERASED_VALUE ((flash_prg_t)-1)
#elif defined(FLASH_PECR_ERASE)
#define FLASH_ERASED_VALUE 0
#else
#error Unknown erase value
#endif

static unsigned int get_page(off_t offset)
{
	return offset / FLASH_PAGE_SIZE;
}

#if defined(FLASH_CR_PER)
static int is_flash_locked(FLASH_TypeDef *regs)
{
	return !!(regs->CR & FLASH_CR_LOCK);
}

static void write_enable(FLASH_TypeDef *regs)
{
	regs->CR |= FLASH_CR_PG;
}

static void write_disable(FLASH_TypeDef *regs)
{
	regs->CR &= (~FLASH_CR_PG);
}

static void erase_page_begin(FLASH_TypeDef *regs, unsigned int page)
{
	/* Set the PER bit and select the page you wish to erase */
	regs->CR |= FLASH_CR_PER;
	regs->AR = CONFIG_FLASH_BASE_ADDRESS + page * FLASH_PAGE_SIZE;

	__DSB();

	/* Set the STRT bit */
	regs->CR |= FLASH_CR_STRT;
}

static void erase_page_end(FLASH_TypeDef *regs)
{
	regs->CR &= ~FLASH_CR_PER;
}

#else

static int is_flash_locked(FLASH_TypeDef *regs)
{
	return !!(regs->PECR & FLASH_PECR_PRGLOCK);
}

static void write_enable(FLASH_TypeDef *regs)
{
	/* Only used for half-page programming on L1x */
#if !defined(CONFIG_SOC_SERIES_STM32L1X)
	regs->PECR |= FLASH_PECR_PROG;
#endif
}

static void write_disable(FLASH_TypeDef *regs)
{
	/* Clear the PG bit */
	regs->PECR &= ~FLASH_PECR_PROG;
}

static void erase_page_begin(FLASH_TypeDef *regs, unsigned int page)
{
	volatile flash_prg_t *page_base = (flash_prg_t *)(
		CONFIG_FLASH_BASE_ADDRESS + page * FLASH_PAGE_SIZE);
	/* Enable programming in erase mode. An erase is triggered by
	 * writing 0 to the first word of a page.
	 */
	regs->PECR |= FLASH_PECR_ERASE;
	regs->PECR |= FLASH_PECR_PROG;

	__DSB();

	*page_base = 0;
}

static void erase_page_end(FLASH_TypeDef *regs)
{
	/* Disable programming */
	regs->PECR &= ~FLASH_PECR_PROG;
	regs->PECR &= ~FLASH_PECR_ERASE;
}
#endif

static int write_value(const struct device *dev, off_t offset,
		       flash_prg_t val)
{
	volatile flash_prg_t *flash = (flash_prg_t *)(
		offset + CONFIG_FLASH_BASE_ADDRESS);
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	int rc;

	/* if the control register is locked, do not fail silently */
	if (is_flash_locked(regs)) {
		LOG_ERR("Flash is locked");
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check if this half word is erased */
	if (*flash != FLASH_ERASED_VALUE) {
		LOG_ERR("Flash location not erased");
		return -EIO;
	}

	/* Enable writing */
	write_enable(regs);

	/* Make sure the register write has taken effect */
	__DSB();

	/* Perform the data write operation at the desired memory address */
	*flash = val;

	/* Wait until the BSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

	/* Disable writing */
	write_disable(regs);

	return rc;
}

/* offset and len must be aligned on 2 for write
 * positive and not beyond end of flash
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	return (!write || (offset % 2 == 0 && len % 2 == 0U)) &&
		flash_stm32_range_exists(dev, offset, len);
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	int i, rc = 0;

	/* if the control register is locked, do not fail silently */
	if (is_flash_locked(regs)) {
		LOG_ERR("Flash is locked");
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	for (i = get_page(offset); i <= get_page(offset + len - 1); ++i) {
		erase_page_begin(regs, i);
		__DSB();
		rc = flash_stm32_wait_flash_idle(dev);
		erase_page_end(regs);

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
	flash_prg_t value;

	for (i = 0; i < len / sizeof(flash_prg_t); i++) {
		memcpy(&value,
		       (const uint8_t *)data + i * sizeof(flash_prg_t),
		       sizeof(flash_prg_t));
		rc = write_value(dev, offset + i * sizeof(flash_prg_t), value);
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
	static struct flash_pages_layout flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (flash_layout.pages_count == 0) {
#if defined(CONFIG_SOC_SERIES_STM32F3X)
		flash_layout.pages_count =
			DT_REG_SIZE(DT_INST(0, soc_nv_flash)) / FLASH_PAGE_SIZE;
#else
		flash_layout.pages_count = (CONFIG_FLASH_SIZE * 1024) /
			FLASH_PAGE_SIZE;
#endif
		flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &flash_layout;
	*layout_size = 1;
}
