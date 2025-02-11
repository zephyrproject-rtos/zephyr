/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2023 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>

#include <soc.h>

#include "flash_stm32.h"

LOG_MODULE_REGISTER(flash_stm32f4x, CONFIG_FLASH_LOG_LEVEL);

#if FLASH_STM32_WRITE_BLOCK_SIZE == 8
typedef uint64_t flash_prg_t;
#define FLASH_PROGRAM_SIZE FLASH_PSIZE_DOUBLE_WORD
#elif FLASH_STM32_WRITE_BLOCK_SIZE == 4
typedef uint32_t flash_prg_t;
#define FLASH_PROGRAM_SIZE FLASH_PSIZE_WORD
#elif FLASH_STM32_WRITE_BLOCK_SIZE == 2
typedef uint16_t flash_prg_t;
#define FLASH_PROGRAM_SIZE FLASH_PSIZE_HALF_WORD
#elif FLASH_STM32_WRITE_BLOCK_SIZE == 1
typedef uint8_t flash_prg_t;
#define FLASH_PROGRAM_SIZE FLASH_PSIZE_BYTE
#else
#error Write block size must be a power of 2, from 1 to 8
#endif

bool flash_stm32_valid_range(const struct device *dev, off_t offset,
			     uint32_t len,
			     bool write)
{
	ARG_UNUSED(write);

#if (FLASH_SECTOR_TOTAL == 12) && defined(FLASH_OPTCR_DB1M)
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	/*
	 * RM0090, table 7.1: STM32F42xxx, STM32F43xxx
	 */
	if (regs->OPTCR & FLASH_OPTCR_DB1M) {
		/* Device configured in Dual Bank, but not supported for now */
		return false;
	}
#endif

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

static int write_value(const struct device *dev, off_t offset, flash_prg_t val)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
#if defined(FLASH_OPTCR_DB1M)
	bool dcache_enabled = false;
#endif /* FLASH_OPTCR_DB*/
	uint32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

#if defined(FLASH_OPTCR_DB1M)
	/*
	 * Disable the data cache to avoid the silicon errata ES0206 Rev 16 2.2.12:
	 * "Data cache might be corrupted during Flash memory read-while-write operation"
	 */
	if (regs->ACR & FLASH_ACR_DCEN) {
		dcache_enabled = true;
		regs->ACR &= (~FLASH_ACR_DCEN);
	}
#endif /* FLASH_OPTCR_DB1M */

	regs->CR &= CR_PSIZE_MASK;
	regs->CR |= FLASH_PROGRAM_SIZE;
	regs->CR |= FLASH_CR_PG;

	/* flush the register write */
	tmp = regs->CR;

	*((flash_prg_t *)(offset + FLASH_STM32_BASE_ADDRESS)) = val;

	rc = flash_stm32_wait_flash_idle(dev);
	regs->CR &= (~FLASH_CR_PG);

#if defined(FLASH_OPTCR_DB1M)
	/* Reset/enable the data cache if previously enabled */
	if (dcache_enabled) {
		regs->ACR |= FLASH_ACR_DCRST;
		regs->ACR &= (~FLASH_ACR_DCRST);
		regs->ACR |= FLASH_ACR_DCEN;
	}
#endif /* FLASH_OPTCR_DB1M */

	return rc;
}

static int erase_sector(const struct device *dev, uint32_t sector)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

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

#if FLASH_SECTOR_TOTAL == 24
	/*
	 * RM0090, §3.9.8: STM32F42xxx, STM32F43xxx
	 * RM0386, §3.7.5: STM32F469xx, STM32F479xx
	 */
	if (sector >= 12) {
		/* From sector 12, SNB is offset by 0b10000 */
		sector += 4U;
	}
#endif

	regs->CR &= CR_PSIZE_MASK;
	regs->CR |= FLASH_PROGRAM_SIZE;

	regs->CR &= ~FLASH_CR_SNB;
	regs->CR |= FLASH_CR_SER | (sector << 3);
	regs->CR |= FLASH_CR_STRT;

	/* flush the register write */
	tmp = regs->CR;

	rc = flash_stm32_wait_flash_idle(dev);
	regs->CR &= ~(FLASH_CR_SER | FLASH_CR_SNB);

	return rc;
}

int flash_stm32_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len)
{
	struct flash_pages_info info;
	uint32_t start_sector, end_sector;
	uint32_t i;
	int rc = 0;

	rc = flash_get_page_info_by_offs(dev, offset, &info);
	if (rc) {
		return rc;
	}
	start_sector = info.index;
	rc = flash_get_page_info_by_offs(dev, offset + len - 1, &info);
	if (rc) {
		return rc;
	}
	end_sector = info.index;

	for (i = start_sector; i <= end_sector; i++) {
		rc = erase_sector(dev, i);
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
		value = UNALIGNED_GET((flash_prg_t *)data + i);
		rc = write_value(dev, offset + i * sizeof(flash_prg_t), value);
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

	if (regs->OPTCR & FLASH_OPTCR_OPTLOCK) {
		return -EIO;
	}

	if ((regs->OPTCR & mask) == value) {
		return 0;
	}

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	regs->OPTCR = (regs->OPTCR & ~mask) | value;
	regs->OPTCR |= FLASH_OPTCR_OPTSTRT;

	/* Make sure previous write is completed. */
	barrier_dsync_fence_full();

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

#if defined(CONFIG_FLASH_STM32_WRITE_PROTECT)
int flash_stm32_update_wp_sectors(const struct device *dev,
				  uint32_t changed_sectors,
				  uint32_t protected_sectors)
{
	changed_sectors <<= FLASH_OPTCR_nWRP_Pos;
	protected_sectors <<= FLASH_OPTCR_nWRP_Pos;

	if ((changed_sectors & FLASH_OPTCR_nWRP_Msk) != changed_sectors) {
		return -EINVAL;
	}

	/* Sector is protected when bit == 0. Flip protected_sectors bits */
	protected_sectors = ~protected_sectors & changed_sectors;

	return write_optb(dev, changed_sectors, protected_sectors);
}

int flash_stm32_get_wp_sectors(const struct device *dev,
			       uint32_t *protected_sectors)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

	*protected_sectors =
		(~regs->OPTCR & FLASH_OPTCR_nWRP_Msk) >> FLASH_OPTCR_nWRP_Pos;

	return 0;
}
#endif /* CONFIG_FLASH_STM32_WRITE_PROTECT */

#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION)
int flash_stm32_update_rdp(const struct device *dev, bool enable,
			   bool permanent)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint8_t current_level, target_level;

	current_level =
		(regs->OPTCR & FLASH_OPTCR_RDP_Msk) >> FLASH_OPTCR_RDP_Pos;
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

		write_optb(dev, FLASH_OPTCR_RDP_Msk,
			   (uint32_t)target_level << FLASH_OPTCR_RDP_Pos);
	}
	return 0;
}

int flash_stm32_get_rdp(const struct device *dev, bool *enabled,
			bool *permanent)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	uint8_t current_level;

	current_level =
		(regs->OPTCR & FLASH_OPTCR_RDP_Msk) >> FLASH_OPTCR_RDP_Pos;

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

/*
 * Different SoC flash layouts are specified in across various
 * reference manuals, but the flash layout for a given number of
 * sectors is consistent across these manuals, with one "gotcha". The
 * number of sectors is given by the HAL as FLASH_SECTOR_TOTAL.
 *
 * The only "gotcha" is that when there are 24 sectors, they are split
 * across 2 "banks" of 12 sectors each, with another set of small
 * sectors (16 KB) in the second bank occurring after the large ones
 * (128 KB) in the first. We could consider supporting this as two
 * devices to make the layout cleaner, but this will do for now.
 */
#ifndef FLASH_SECTOR_TOTAL
#error "Unknown flash layout"
#else  /* defined(FLASH_SECTOR_TOTAL) */
#if FLASH_SECTOR_TOTAL == 5
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/* RM0401, table 5: STM32F410Tx, STM32F410Cx, STM32F410Rx */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
};
#elif FLASH_SECTOR_TOTAL == 6
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/* RM0368, table 5: STM32F401xC */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 1, .pages_size = KB(128)},
};
#elif FLASH_SECTOR_TOTAL == 8
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/*
	 * RM0368, table 5: STM32F401xE
	 * RM0383, table 4: STM32F411xE
	 * RM0390, table 4: STM32F446xx
	 */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 3, .pages_size = KB(128)},
};
#elif FLASH_SECTOR_TOTAL == 12
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/*
	 * RM0090, table 5: STM32F405xx, STM32F415xx, STM32F407xx, STM32F417xx
	 * RM0402, table 5: STM32F412Zx, STM32F412Vx, STM32F412Rx, STM32F412Cx
	 */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#elif FLASH_SECTOR_TOTAL == 16
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/* RM0430, table 5.: STM32F413xx, STM32F423xx */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 11, .pages_size = KB(128)},
};
#elif FLASH_SECTOR_TOTAL == 24
static const struct flash_pages_layout stm32f4_flash_layout[] = {
	/*
	 * RM0090, table 6: STM32F427xx, STM32F437xx, STM32F429xx, STM32F439xx
	 * RM0386, table 4: STM32F469xx, STM32F479xx
	 */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#else
#error "Unknown flash layout"
#endif /* FLASH_SECTOR_TOTAL == 5 */
#endif/* !defined(FLASH_SECTOR_TOTAL) */

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = stm32f4_flash_layout;
	*layout_size = ARRAY_SIZE(stm32f4_flash_layout);
}
