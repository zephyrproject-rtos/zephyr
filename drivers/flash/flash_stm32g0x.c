/*
 * Copyright (c) 2019 Philippe Retornaz <philippe@shapescale.com>
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32g0
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <soc.h>

#include "flash_stm32.h"


/* FLASH_DBANK_SUPPORT is defined in the HAL for all G0Bx and G0C1 SoCs,
 * while only those with 256KiB and 512KiB Flash have two banks.
 */
#if defined(FLASH_DBANK_SUPPORT) && (CONFIG_FLASH_SIZE > (128))
#define STM32G0_DBANK_SUPPORT
#endif

#if defined(STM32G0_DBANK_SUPPORT)
#define STM32G0_BANK_COUNT		2
#define STM32G0_BANK2_START_PAGE_NR	256
#else
#define STM32G0_BANK_COUNT		1
#endif

#define STM32G0_FLASH_SIZE		(FLASH_SIZE)
#define STM32G0_FLASH_PAGE_SIZE		(FLASH_PAGE_SIZE)
#define STM32G0_PAGES_PER_BANK		\
	((STM32G0_FLASH_SIZE / STM32G0_FLASH_PAGE_SIZE) / STM32G0_BANK_COUNT)

/*
 * offset and len must be aligned on 8 for write,
 * positive and not beyond end of flash
 * On dual-bank SoCs memory accesses starting on the first bank and continuing
 * beyond the first bank into the second bank are allowed.
 */
bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	return (!write || (offset % 8 == 0 && len % 8 == 0)) &&
		flash_stm32_range_exists(dev, offset, len);
}

static inline void flush_cache(FLASH_TypeDef *regs)
{
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
	volatile uint32_t *flash = (uint32_t *)(offset + CONFIG_FLASH_BASE_ADDRESS);
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
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
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/*
	 * If an erase operation in Flash memory also concerns data
	 * in the instruction cache, the user has to ensure that these data
	 * are rewritten before they are accessed during code execution.
	 */
	flush_cache(regs);

	tmp = regs->CR;
	page = offset / STM32G0_FLASH_PAGE_SIZE;

#if defined(STM32G0_DBANK_SUPPORT)
	bool swap_enabled = (regs->OPTR & FLASH_OPTR_nSWAP_BANK) == 0;

	/* big page-nr w/o swap or small page-nr w/ swap indicate bank2 */
	if ((page >= STM32G0_PAGES_PER_BANK) != swap_enabled) {
		page = (page % STM32G0_PAGES_PER_BANK) + STM32G0_BANK2_START_PAGE_NR;
		tmp |= FLASH_CR_BKER;
		LOG_DBG("Erase page %d on bank 2", page);
	} else {
		page = page % STM32G0_PAGES_PER_BANK;
		tmp &= ~FLASH_CR_BKER;
		LOG_DBG("Erase page %d on bank 1", page);
	}
#endif

	/* Set the PER bit and select the page you wish to erase */
	tmp |= FLASH_CR_PER;
	tmp &= ~FLASH_CR_PNB_Msk;
	tmp |= ((page << FLASH_CR_PNB_Pos) & FLASH_CR_PNB_Msk);

	/* Set the STRT bit and write the reg */
	tmp |= FLASH_CR_STRT;
	regs->CR = tmp;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	regs->CR &= ~FLASH_CR_PER;

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	unsigned int addr = offset;
	int rc = 0;

	for (; addr <= offset + len - 1 ; addr += STM32G0_FLASH_PAGE_SIZE) {
		rc = erase_page(dev, addr);
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
		rc = write_dword(dev, offset,
				UNALIGNED_GET((const uint64_t *) data + (i >> 3)));
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

/*
 * The address space is always continuous, even though a subset of G0 SoCs has
 * two flash banks.
 * Only the "physical" flash page-NRs are not continuous on those SoCs.
 * As a result the page numbers used in the zephyr flash api differs
 * from the "physical" flash page number.
 * The first is equal to the address offset divided by the page size, while
 * "physical" pages are numbered starting with 0 on bank1 and 256 on bank2.
 * As a result only a single homogeneous flash page layout needs to be defined.
 */
void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32g0_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32g0_flash_layout.pages_count == 0) {
		stm32g0_flash_layout.pages_count =
				STM32G0_FLASH_SIZE / STM32G0_FLASH_PAGE_SIZE;
		stm32g0_flash_layout.pages_size = STM32G0_FLASH_PAGE_SIZE;
	}

	*layout = &stm32g0_flash_layout;
	*layout_size = 1;
}

/* Override weak function */
int  flash_stm32_check_configuration(void)
{
#if defined(STM32G0_DBANK_SUPPORT) && (CONFIG_FLASH_SIZE == 256)
	/* Single bank mode not supported on dual bank SoCs with 256kiB flash */
	if ((regs->OPTR & FLASH_OPTR_DUAL_BANK) == 0) {
		LOG_ERR("Single bank configuration not supported by the driver");
		return -ENOTSUP;
	}
#endif
	return 0;
}
