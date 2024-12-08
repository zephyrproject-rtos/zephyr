/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 BayLibre, SAS
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32l4
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

#include "flash_stm32.h"

#if !defined(STM32L4R5xx) && !defined(STM32L4R7xx) && !defined(STM32L4R9xx) && \
	!defined(STM32L4S5xx) && !defined(STM32L4S7xx) && !defined(STM32L4S9xx) && \
	!defined(STM32L4Q5xx) && !defined(STM32L4P5xx)
#define STM32L4X_PAGE_SHIFT	11
#else
#define STM32L4X_PAGE_SHIFT	12
#endif

#if defined(FLASH_OPTR_DUALBANK) || defined(FLASH_STM32_DBANK)
#define CONTROL_DCACHE
#endif

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

/*
 * STM32L4xx devices can have up to 512 2K pages on two 256x2K pages banks
 *
 * STM32L4R/Sxx devices can have up to 512 4K pages on two 256x4K pages banks
 */
static unsigned int get_page(off_t offset)
{
	return offset >> STM32L4X_PAGE_SHIFT;
}

static int write_dword(const struct device *dev, off_t offset, uint64_t val)
{
	volatile uint32_t *flash = (uint32_t *)(offset + FLASH_STM32_BASE_ADDRESS);
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
#ifdef CONTROL_DCACHE
	bool dcache_enabled = false;
#endif /* CONTROL_DCACHE */
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

#ifdef CONTROL_DCACHE
	/*
	 * Disable the data cache to avoid the silicon errata 2.2.3:
	 * "Data cache might be corrupted during Flash memory read-while-write operation"
	 */
	if (regs->ACR & FLASH_ACR_DCEN) {
		dcache_enabled = true;
		regs->ACR &= (~FLASH_ACR_DCEN);
	}
#endif /* CONTROL_DCACHE */

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

#ifdef CONTROL_DCACHE
	/* Reset/enable the data cache if previously enabled */
	if (dcache_enabled) {
		regs->ACR |= FLASH_ACR_DCRST;
		regs->ACR &= (~FLASH_ACR_DCRST);
		regs->ACR |= FLASH_ACR_DCEN;
	}
#endif /* CONTROL_DCACHE */

	return rc;
}

#define SOC_NV_FLASH_SIZE DT_REG_SIZE(DT_INST(0, soc_nv_flash))

static int erase_page(const struct device *dev, unsigned int page)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	uint16_t pages_per_bank;
	int rc;

#if !defined(FLASH_OPTR_DUALBANK) && !defined(FLASH_STM32_DBANK)
	/* Single bank device. Each page is of 2KB size */
	pages_per_bank = SOC_NV_FLASH_SIZE >> 11;
#elif defined(FLASH_OPTR_DUALBANK)
	/* L4 series (2K page size) with configurable Dual Bank (default y) */
	/* Dual Bank is only option for 1M devices */
	if ((regs->OPTR & FLASH_OPTR_DUALBANK) ||
	    (SOC_NV_FLASH_SIZE == (1024*1024))) {
		/* Dual Bank configuration (nbr pages = flash size / 2 / 2K) */
		pages_per_bank = SOC_NV_FLASH_SIZE >> 12;
	} else {
		/* Single bank configuration. This has not been validated. */
		/* Not supported for now. */
		return -ENOTSUP;
	}
#elif defined(FLASH_STM32_DBANK)
	/* L4+ series (4K page size) with configurable Dual Bank (default y)*/
	if (regs->OPTR & FLASH_STM32_DBANK) {
		/* Dual Bank configuration (nbre pags = flash size / 2 / 4K) */
		pages_per_bank = SOC_NV_FLASH_SIZE >> 13;
	} else {
		/* Single bank configuration */
		/* Requires 128 bytes data read. This config is not supported */
		return -ENOTSUP;
	}
#endif

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	flush_cache(regs);

	/* Set the PER bit and select the page you wish to erase */
	regs->CR |= FLASH_CR_PER;
#ifdef FLASH_CR_BKER
	regs->CR &= ~FLASH_CR_BKER_Msk;
	/* Select bank, only for DUALBANK devices */
	if (page >= pages_per_bank) {
		regs->CR |= FLASH_CR_BKER;
	}
#endif
	regs->CR &= ~FLASH_CR_PNB_Msk;
	regs->CR |= ((page % pages_per_bank) << 3);

	/* Set the STRT bit */
	regs->CR |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->CR;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	regs->CR &= ~FLASH_CR_PER;

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	int i, rc = 0;

	i = get_page(offset);
	for (; i <= get_page(offset + len - 1) ; ++i) {
		rc = erase_page(dev, i);
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

	for (i = 0; i < len; i += 8, offset += 8U) {
		rc = write_dword(dev, offset,
				UNALIGNED_GET((const uint64_t *) data + (i >> 3)));
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
uint8_t flash_stm32_get_rdp_level(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

	return (regs->OPTR & FLASH_OPTR_RDP_Msk) >> FLASH_OPTR_RDP_Pos;
}

void flash_stm32_set_rdp_level(const struct device *dev, uint8_t level)
{
	write_optb(dev, FLASH_OPTR_RDP_Msk,
		(uint32_t)level << FLASH_OPTR_RDP_Pos);
}
#endif /* CONFIG_FLASH_STM32_READOUT_PROTECTION */

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32l4_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32l4_flash_layout.pages_count == 0) {
		stm32l4_flash_layout.pages_count = FLASH_SIZE / FLASH_PAGE_SIZE;
		stm32l4_flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &stm32l4_flash_layout;
	*layout_size = 1;
}
