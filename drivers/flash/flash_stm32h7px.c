/* SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics */
/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>

#include <stm32_ll_flash.h>
#include <stm32_bitops.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32h7p, CONFIG_FLASH_LOG_LEVEL);

#include "flash_stm32.h"

#define STM32H7P_FLASH_BANKS_NUM	2U
#define STM32H7P_FLASH_SIZE		DT_REG_SIZE(DT_INST(0, st_stm32_nv_flash))
#define STM32H7P_FLASH_PAGE_SIZE	DT_PROP(DT_INST(0, st_stm32_nv_flash), erase_block_size)
#define STM32H7P_FLASH_PAGES_NUM	(STM32H7P_FLASH_SIZE / STM32H7P_FLASH_PAGE_SIZE)
#define STM32H7P_PAGES_PER_BANK		(STM32H7P_FLASH_PAGES_NUM / STM32H7P_FLASH_BANKS_NUM)
#define STM32H7P_FLASH_BANK_SIZE	(STM32H7P_FLASH_SIZE / STM32H7P_FLASH_BANKS_NUM)

/** Value of erased flash word (all bits read as 1) */
#define STM32H7P_ERASED_WORD_VALUE	0xFFFFFFFFU

static bool is_valid_flash_range(off_t offset, uint32_t len)
{
	uint32_t offset_plus_len;

		/* (offset + len) must not overflow */
	return !u32_add_overflow(offset, len, &offset_plus_len)
		/* offset must be a valid offset in flash */
		&& IN_RANGE(offset, 0, STM32H7P_FLASH_SIZE - 1)
		/* (offset + len) must be in [0; flash size]
		 * because it is equal to the last accessed
		 * byte in flash plus one (an access of `len`
		 * bytes starting at `offset` touches bytes
		 * `offset` to `offset + len` EXCLUDED)
		 */
		&& IN_RANGE(offset_plus_len, 0, STM32H7P_FLASH_SIZE);
}

static bool is_eraseable_flash_range(off_t offset, uint32_t len)
{
	/* Erase can only be done at page granularity */
	if ((offset % STM32H7P_FLASH_PAGE_SIZE) != 0
	    || (len % STM32H7P_FLASH_PAGE_SIZE) != 0) {
		return false;
	}

	return is_valid_flash_range(offset, len);
}

/**
 * @param bank Logical bank number (0 = Bank A, 1 = Bank B)
 * @return Physical LL_FLASH_BANK_x for given logical bank
 */
static uint32_t get_physical_bank(const struct device *dev, uint32_t bank)
{
	if (LL_FLASH_OB_IsBankSwapped(FLASH_STM32_REGS(dev))) {
		return (bank == 0) ? LL_FLASH_BANK_2 : LL_FLASH_BANK_1;
	} else {
		return (bank == 0) ? LL_FLASH_BANK_1 : LL_FLASH_BANK_2;
	}
}

static int flash_stm32_check_range_is_erased(uint32_t offset, uint32_t size)
{
	const uint32_t end = offset + size;
	const uint32_t *ptr = (const uint32_t *)(FLASH_STM32_BASE_ADDRESS + offset);

	__ASSERT_NO_MSG(offset % sizeof(uint32_t) == 0);

	for (uint32_t i = offset; i < end; i += sizeof(uint32_t), ptr++) {
		const uint32_t val = stm32_reg_read(ptr);

		if (val != STM32H7P_ERASED_WORD_VALUE) {
			LOG_ERR("Found non-erased word at offset 0x%08x (=0x%08x)", i, val);
			return -EIO;
		}
	}

	return 0;
}

/* Additional backend functions implementation */
int flash_stm32_wait_flash_idle(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(STM32_FLASH_TIMEOUT));
	bool busy, expired;
	uint32_t asr, bsr;

	do {
		/*
		 * RM0533 Rev 1 §5.9.5 "FLASH bank x status register (FLASH_xSR)":
		 * It is not recommended to do software polling on BUSY to know when
		 * one operation completed because, depending of operation, more pulses
		 * are possible for one only operation. For software polling it is therefore
		 * better to use QW flag or to check the EOPF flag.
		 */
		busy = LL_FLASH_IsActiveFlag_QW(regs, LL_FLASH_BANK_1)
		       || LL_FLASH_IsActiveFlag_QW(regs, LL_FLASH_BANK_2);
		asr = LL_FLASH_ReadFlag(regs, LL_FLASH_BANK_1, LL_FLASH_FLAG_ERRORS_ALL);
		bsr = LL_FLASH_ReadFlag(regs, LL_FLASH_BANK_2, LL_FLASH_FLAG_ERRORS_ALL);
		expired = sys_timepoint_expired(timeout);
	} while (busy && (asr == 0U) && (bsr == 0U) && !expired);

	if (asr != 0U || bsr != 0U) {
		LL_FLASH_ClearFlag(regs, LL_FLASH_BANK_1, LL_FLASH_FLAG_ERRORS_ALL);
		LL_FLASH_ClearFlag(regs, LL_FLASH_BANK_2, LL_FLASH_FLAG_ERRORS_ALL);
		LOG_ERR("Flash error: ASR=0x%08x BSR=0x%08x", asr, bsr);
		return -EIO;
	} else if (expired) {
		LOG_ERR("Flash operation timeout");
		return -ETIMEDOUT;
	} else {
		return 0;
	}
}

/* Backend functions implementation */

bool flash_stm32_valid_range(const struct device *dev, off_t offset, uint32_t len,
			     bool write)
{
	ARG_UNUSED(dev);

	if (write && !flash_stm32_valid_write(offset, len)) {
		return false;
	}

	return is_valid_flash_range(offset, len);
}

int flash_stm32_block_erase_loop(const struct device *dev, unsigned int offset, unsigned int len)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	int res = 0;

	if (!is_eraseable_flash_range(offset, len)) {
		LOG_ERR("Erase range is not page-aligned: offset=0x%x, len=0x%x", offset, len);
		return -EINVAL;
	}

	const uint32_t start_page = (uint32_t)offset / STM32H7P_FLASH_PAGE_SIZE;
	const uint32_t page_count = len / STM32H7P_FLASH_PAGE_SIZE;
	const uint32_t last_page = start_page + page_count - 1;

	for (uint32_t bank = 0, page = start_page;
	     bank < STM32H7P_FLASH_BANKS_NUM && page <= last_page;
	     bank++ /* page is incremented in loop body*/) {
		const uint32_t bank_start_page = bank * STM32H7P_PAGES_PER_BANK;
		const uint32_t bank_end_page = bank_start_page + (STM32H7P_PAGES_PER_BANK - 1);

		if (!IN_RANGE(page, bank_start_page, bank_end_page)) {
			continue; /* No pages to erase in this bank */
		}

		const uint32_t erase_start_page = MAX(page, bank_start_page);
		const uint32_t erase_end_page = MIN(last_page, bank_end_page);
		const uint32_t ll_bank = get_physical_bank(dev, bank);

		LOG_INF("Erasing bank %u pages %u~%u", bank, erase_start_page, erase_end_page);

		if (LL_FLASH_IsLocked(regs, ll_bank)) {
			LOG_ERR("Flash bank %u is locked", bank);
			return -EIO;
		}

		for (page = erase_start_page; page <= erase_end_page; page++) {
			const uint32_t inbank_page_index = page - bank_start_page;

			res = flash_stm32_wait_flash_idle(dev);
			if (res < 0) {
				return res;
			}

			/* N.B.: this implicitly enables page erase */
			LL_FLASH_StartErasePage(regs, ll_bank, inbank_page_index);

			res = flash_stm32_wait_flash_idle(dev);

			LL_FLASH_DisablePageErase(regs, ll_bank);

			if (res < 0) {
				LOG_ERR("Failed to erase page %u in bank %u: %d", page, bank, res);
				break;
			}
		}
	}

	return res;
}

int flash_stm32_write_range(const struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	const uint32_t end = offset + len;
	const uint8_t *pdata = data;
	int res;

	/*
	 * Check that destination in flash is erased.
	 * Per RM0533 Rev 1 §5.3.8 "FLASH program operations":
	 * "It is not recommended to overwrite a flash word that is not virgin.
	 *  The result may lead to an inconsistent ECC code that is systematically
	 *  reported by the embedded flash memory, as described in Section 5.7.5:
	 *  Error correction code error (SNECCERRF/DBECCERRF)."
	 *
	 * Verify that entire target range is virgin before writing.
	 */
	res = flash_stm32_check_range_is_erased(offset, len);
	if (res < 0) {
		return res;
	}

	for (uint32_t bank = 0, write_offset = offset;
	     bank < STM32H7P_FLASH_BANKS_NUM && write_offset < end;
	     bank++ /* write_offset is incremented in loop body*/) {
		const uint32_t bank_start_offset = bank * STM32H7P_FLASH_BANK_SIZE;
		const uint32_t bank_end_offset = bank_start_offset + STM32H7P_FLASH_BANK_SIZE - 1;

		if (!IN_RANGE(write_offset, bank_start_offset, bank_end_offset)) {
			continue; /* No data to write in this bank */
		}

		const uint32_t write_start_offset = MAX(write_offset, bank_start_offset);
		const uint32_t write_end_offset = MIN(end, bank_end_offset);
		const uint32_t write_count = write_end_offset - write_start_offset + 1;
		const uint32_t ll_bank = get_physical_bank(dev, bank);

		LOG_INF("Writing %u bytes in bank %u (offset 0x%x~0x%x)",
			write_count, bank, write_start_offset, write_end_offset);

		if (LL_FLASH_IsLocked(regs, ll_bank)) {
			LOG_ERR("Flash bank %u is locked", bank);
			return -EIO;
		}

		LL_FLASH_EnableProgramming(regs, ll_bank);
		if (!LL_FLASH_IsEnabledProgramming(regs, ll_bank)) {
			LOG_ERR("Error enabling flash programming for bank %u", bank);
			return -EIO;
		}

		for (/* nothing */;
		     write_offset < write_end_offset;
		     write_offset += FLASH_STM32_WRITE_BLOCK_SIZE) {
			res = flash_stm32_wait_flash_idle(dev);
			if (res < 0) {
				return res;
			}

			/*
			 * Flash is on AXI interface with 64-bit accesses.
			 * Perform unaligned reads but write using biggest size.
			 */
			for (uint32_t i = 0;
			     i < FLASH_STM32_WRITE_BLOCK_SIZE;
			     i += sizeof(uint64_t)) {
				uint64_t dw = sys_get_le64(pdata + (write_offset - offset) + i);

				sys_write64(dw, FLASH_STM32_BASE_ADDRESS + write_offset + i);
			}

			res = flash_stm32_wait_flash_idle(dev);

			if (res < 0) {
				LOG_ERR("Failed to write at offset 0x%x in bank %u: %d",
					write_offset, bank, res);
				break;
			}
		}

		LL_FLASH_DisableProgramming(regs, ll_bank);

		if (res < 0) {
			break; /* Stop writing if an error occurred */
		}
	}

	return res;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout stm32h7p_flash_layout[] = {
	/*
	 * The STM32H7P flash is divided in two banks.
	 * However, both banks only contain pages of identical size.
	 * Since the Zephyr layout should only convey page granularity
	 * information rather than actual physical organization, describe
	 * the flash using a single descriptor covering both banks.
	 */
	{ .pages_size = STM32H7P_FLASH_PAGE_SIZE,
	  .pages_count = STM32H7P_FLASH_SIZE / STM32H7P_FLASH_PAGE_SIZE },
};

void flash_stm32_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = stm32h7p_flash_layout;
	*layout_size = ARRAY_SIZE(stm32h7p_flash_layout);
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
