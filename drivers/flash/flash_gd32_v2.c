/*
 * Copyright (c) 2022 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "flash_gd32.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <gd32_fmc.h>

LOG_MODULE_DECLARE(flash_gd32);

#define GD32_NV_FLASH_V2_NODE		DT_INST(0, gd_gd32_nv_flash_v2)
#define GD32_NV_FLASH_V2_TIMEOUT	DT_PROP(GD32_NV_FLASH_V2_NODE, max_erase_time_ms)

#if !defined(CONFIG_SOC_GD32A503)
/**
 * @brief GD32 FMC v2 flash memory has 2 banks.
 * Bank0 holds the first 512KB, bank1 is used give capacity for reset.
 * The page size is the same within the same bank, but not equal for all banks.
 */
#if (PRE_KB(512) >= SOC_NV_FLASH_SIZE)
#define GD32_NV_FLASH_V2_BANK0_SIZE		SOC_NV_FLASH_SIZE
#define GD32_NV_FLASH_V2_BANK0_PAGE_SIZE	DT_PROP(GD32_NV_FLASH_V2_NODE, bank0_page_size)
#else
#define GD32_NV_FLASH_V2_BANK0_SIZE		KB(512)
#define GD32_NV_FLASH_V2_BANK0_PAGE_SIZE	DT_PROP(GD32_NV_FLASH_V2_NODE, bank0_page_size)
#define GD32_NV_FLASH_V2_BANK1_SIZE		(SOC_NV_FLASH_SIZE - KB(512))
#define GD32_NV_FLASH_V2_BANK1_PAGE_SIZE	DT_PROP(GD32_NV_FLASH_V2_NODE, bank1_page_size)
#endif

#elif defined(CONFIG_SOC_GD32A503)
/**
 * @brief GD32A503 series flash memory has 2 banks.
 * Bank0 holds the first 256KB, bank1 is used give capacity for reset.
 * The page size is 1KB for all banks.
 */
#if (PRE_KB(256) >= SOC_NV_FLASH_SIZE)
#define GD32_NV_FLASH_V2_BANK0_SIZE		SOC_NV_FLASH_SIZE
#define GD32_NV_FLASH_V2_BANK0_PAGE_SIZE	DT_PROP(GD32_NV_FLASH_V2_NODE, bank0_page_size)
#else
#define GD32_NV_FLASH_V2_BANK0_SIZE		KB(256)
#define GD32_NV_FLASH_V2_BANK0_PAGE_SIZE	DT_PROP(GD32_NV_FLASH_V2_NODE, bank0_page_size)
#define GD32_NV_FLASH_V2_BANK1_SIZE		(SOC_NV_FLASH_SIZE - KB(256))
#define GD32_NV_FLASH_V2_BANK1_PAGE_SIZE	DT_PROP(GD32_NV_FLASH_V2_NODE, bank1_page_size)
#endif
#endif

#define GD32_FMC_V2_BANK0_WRITE_ERR (FMC_STAT0_PGERR | FMC_STAT0_WPERR)
#define GD32_FMC_V2_BANK0_ERASE_ERR FMC_STAT0_WPERR

#define GD32_FMC_V2_BANK1_WRITE_ERR (FMC_STAT1_PGERR | FMC_STAT1_WPERR)
#define GD32_FMC_V2_BANK1_ERASE_ERR FMC_STAT1_WPERR

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static struct flash_pages_layout gd32_fmc_v2_layout[] = {
	{
	.pages_size = GD32_NV_FLASH_V2_BANK0_PAGE_SIZE,
	.pages_count = GD32_NV_FLASH_V2_BANK0_SIZE / GD32_NV_FLASH_V2_BANK0_PAGE_SIZE
	},
#ifdef GD32_NV_FLASH_V2_BANK1_SIZE
	{
	.pages_size = GD32_NV_FLASH_V2_BANK1_PAGE_SIZE,
	.pages_count = GD32_NV_FLASH_V2_BANK1_SIZE / GD32_NV_FLASH_V2_BANK1_PAGE_SIZE
	}
#endif
};
#endif

static inline void gd32_fmc_v2_bank0_unlock(void)
{
	FMC_KEY0 = UNLOCK_KEY0;
	FMC_KEY0 = UNLOCK_KEY1;
}

static inline void gd32_fmc_v2_bank0_lock(void)
{
	FMC_CTL0 |= FMC_CTL0_LK;
}

static int gd32_fmc_v2_bank0_wait_idle(void)
{
	const int64_t expired_time = k_uptime_get() + GD32_NV_FLASH_V2_TIMEOUT;

	while (FMC_STAT0 & FMC_STAT0_BUSY) {
		if (k_uptime_get() > expired_time) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int gd32_fmc_v2_bank0_write(k_off_t offset, const void *data, size_t len)
{
	flash_prg_t *prg_flash = (flash_prg_t *)((uint8_t *)SOC_NV_FLASH_ADDR + offset);
	flash_prg_t *prg_data = (flash_prg_t *)data;
	int ret = 0;

	gd32_fmc_v2_bank0_unlock();

	if (FMC_STAT0 & FMC_STAT0_BUSY) {
		return -EBUSY;
	}

	FMC_CTL0 |= FMC_CTL0_PG;

	for (size_t i = 0U; i < (len / sizeof(flash_prg_t)); i++) {
		*prg_flash++ = *prg_data++;
	}

	ret = gd32_fmc_v2_bank0_wait_idle();
	if (ret < 0) {
		goto expired_out;
	}

	if (FMC_STAT0 & GD32_FMC_V2_BANK0_WRITE_ERR) {
		ret = -EIO;
		FMC_STAT0 |= GD32_FMC_V2_BANK0_WRITE_ERR;
		LOG_ERR("FMC bank0 programming failed");
	}

expired_out:
	FMC_CTL0 &= ~FMC_CTL0_PG;

	gd32_fmc_v2_bank0_lock();

	return ret;
}

static int gd32_fmc_v2_bank0_page_erase(uint32_t page_addr)
{
	int ret = 0;

	gd32_fmc_v2_bank0_unlock();

	if (FMC_STAT0 & FMC_STAT0_BUSY) {
		return -EBUSY;
	}

	FMC_CTL0 |= FMC_CTL0_PER;

	FMC_ADDR0 = page_addr;

	FMC_CTL0 |= FMC_CTL0_START;

	ret = gd32_fmc_v2_bank0_wait_idle();
	if (ret < 0) {
		goto expired_out;
	}

	if (FMC_STAT0 & GD32_FMC_V2_BANK0_ERASE_ERR) {
		ret = -EIO;
		FMC_STAT0 |= GD32_FMC_V2_BANK0_ERASE_ERR;
		LOG_ERR("FMC bank0 page %u erase failed", page_addr);
	}

expired_out:
	FMC_CTL0 &= ~FMC_CTL0_PER;

	gd32_fmc_v2_bank0_lock();

	return ret;
}

static int gd32_fmc_v2_bank0_erase_block(k_off_t offset, size_t size)
{
	uint32_t page_addr = SOC_NV_FLASH_ADDR + offset;
	int ret = 0;

	while (size > 0U) {
		ret = gd32_fmc_v2_bank0_page_erase(page_addr);
		if (ret < 0) {
			return ret;
		}

		size -= GD32_NV_FLASH_V2_BANK0_PAGE_SIZE;
		page_addr += GD32_NV_FLASH_V2_BANK0_PAGE_SIZE;
	}

	return 0;
}

#ifdef GD32_NV_FLASH_V2_BANK1_SIZE
static inline void gd32_fmc_v2_bank1_unlock(void)
{
	FMC_KEY1 = UNLOCK_KEY0;
	FMC_KEY1 = UNLOCK_KEY1;
}

static inline void gd32_fmc_v2_bank1_lock(void)
{
	FMC_CTL1 |= FMC_CTL1_LK;
}

static int gd32_fmc_v2_bank1_wait_idle(void)
{
	const int64_t expired_time = k_uptime_get() + GD32_NV_FLASH_V2_TIMEOUT;

	while (FMC_STAT1 & FMC_STAT1_BUSY) {
		if (k_uptime_get() > expired_time) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int gd32_fmc_v2_bank1_write(k_off_t offset, const void *data, size_t len)
{
	flash_prg_t *prg_flash = (flash_prg_t *)((uint8_t *)SOC_NV_FLASH_ADDR + offset);
	flash_prg_t *prg_data = (flash_prg_t *)data;
	int ret = 0;

	gd32_fmc_v2_bank1_unlock();

	if (FMC_STAT1 & FMC_STAT1_BUSY) {
		return -EBUSY;
	}

	FMC_CTL1 |= FMC_CTL1_PG;

	for (size_t i = 0U; i < (len / sizeof(flash_prg_t)); i++) {
		*prg_flash++ = *prg_data++;
	}

	ret = gd32_fmc_v2_bank1_wait_idle();
	if (ret < 0) {
		goto expired_out;
	}

	if (FMC_STAT1 & GD32_FMC_V2_BANK1_WRITE_ERR) {
		ret = -EIO;
		FMC_STAT1 |= GD32_FMC_V2_BANK1_WRITE_ERR;
		LOG_ERR("FMC bank1 programming failed");
	}

expired_out:
	FMC_CTL1 &= ~FMC_CTL1_PG;

	gd32_fmc_v2_bank1_lock();

	return ret;
}

static int gd32_fmc_v2_bank1_page_erase(uint32_t page_addr)
{
	int ret = 0;

	gd32_fmc_v2_bank1_unlock();

	if (FMC_STAT1 & FMC_STAT1_BUSY) {
		return -EBUSY;
	}

	FMC_CTL1 |= FMC_CTL1_PER;

	FMC_ADDR1 = page_addr;

	FMC_CTL1 |= FMC_CTL1_START;

	ret = gd32_fmc_v2_bank1_wait_idle();
	if (ret < 0) {
		goto expired_out;
	}

	if (FMC_STAT1 & GD32_FMC_V2_BANK1_ERASE_ERR) {
		ret = -EIO;
		FMC_STAT1 |= GD32_FMC_V2_BANK1_ERASE_ERR;
		LOG_ERR("FMC bank1 page %u erase failed", page_addr);
	}

expired_out:
	FMC_CTL1 &= ~FMC_CTL1_PER;

	gd32_fmc_v2_bank1_lock();

	return ret;
}

static int gd32_fmc_v2_bank1_erase_block(k_off_t offset, size_t size)
{
	uint32_t page_addr = SOC_NV_FLASH_ADDR + offset;
	int ret = 0;

	while (size > 0U) {
		ret = gd32_fmc_v2_bank1_page_erase(page_addr);
		if (ret < 0) {
			return ret;
		}

		size -= GD32_NV_FLASH_V2_BANK0_SIZE;
		page_addr += GD32_NV_FLASH_V2_BANK0_SIZE;
	}

	return 0;
}
#endif /* GD32_NV_FLASH_V2_BANK1_SIZE */

bool flash_gd32_valid_range(k_off_t offset, uint32_t len, bool write)
{
	if ((offset > SOC_NV_FLASH_SIZE) ||
	    ((offset + len) > SOC_NV_FLASH_SIZE)) {
		return false;
	}

	if (write) {
		/* Check offset and len is flash_prg_t aligned. */
		if ((offset % sizeof(flash_prg_t)) ||
		    (len % sizeof(flash_prg_t))) {
			return false;
		}

	} else {
		if (offset < GD32_NV_FLASH_V2_BANK0_SIZE) {
			if (offset % GD32_NV_FLASH_V2_BANK0_PAGE_SIZE) {
				return false;
			}

			if (((offset + len) <= GD32_NV_FLASH_V2_BANK0_SIZE) &&
			    (len % GD32_NV_FLASH_V2_BANK0_PAGE_SIZE)) {
				return false;
			}
		}

#ifdef GD32_NV_FLASH_V2_BANK1_SIZE
		/* Remove bank0 info from offset and len. */
		if ((offset < GD32_NV_FLASH_V2_BANK0_SIZE) &&
		    ((offset + len) > GD32_NV_FLASH_V2_BANK0_SIZE))  {
			len -= (GD32_NV_FLASH_V2_BANK0_SIZE - offset);
			offset = GD32_NV_FLASH_V2_BANK0_SIZE;
		}

		if (offset >= GD32_NV_FLASH_V2_BANK0_SIZE) {
			if ((offset % GD32_NV_FLASH_V2_BANK1_PAGE_SIZE) ||
			    (len % GD32_NV_FLASH_V2_BANK1_PAGE_SIZE)) {
				return false;
			}
		}
#endif
	}

	return true;
}

int flash_gd32_write_range(k_off_t offset, const void *data, size_t len)
{
	size_t len0 = 0U;
	int ret = 0;

	if (offset < GD32_NV_FLASH_V2_BANK0_SIZE) {
		if ((offset + len) > GD32_NV_FLASH_V2_BANK0_SIZE) {
			len0 = GD32_NV_FLASH_V2_BANK0_SIZE - offset;
		} else {
			len0 = len;
		}

		ret = gd32_fmc_v2_bank0_write(offset, data, len0);
		if (ret < 0) {
			return ret;
		}
	}

#ifdef GD32_NV_FLASH_V2_BANK1_SIZE
	size_t len1 = len - len0;

	if (len1 == 0U) {
		return 0;
	}

	/* Will programming bank1, remove bank0 offset. */
	if (offset < GD32_NV_FLASH_V2_BANK0_SIZE)  {
		offset = GD32_NV_FLASH_V2_BANK0_SIZE;
	}

	ret = gd32_fmc_v2_bank1_write(offset, data, len1);
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}

int flash_gd32_erase_block(k_off_t offset, size_t size)
{
	size_t size0 = 0U;
	int ret = 0;

	if (offset < GD32_NV_FLASH_V2_BANK0_SIZE) {
		if ((offset + size0) > GD32_NV_FLASH_V2_BANK0_SIZE) {
			size0 = GD32_NV_FLASH_V2_BANK0_SIZE - offset;
		} else {
			size0 = size;
		}

		ret = gd32_fmc_v2_bank0_erase_block(offset, size0);
		if (ret < 0) {
			return ret;
		}
	}

#ifdef GD32_NV_FLASH_V2_BANK1_SIZE
	size_t size1 = size - size0;

	if (size1 == 0U) {
		return 0;
	}

	/* Will programming bank1, remove bank0 info from offset. */
	if (offset < GD32_NV_FLASH_V2_BANK0_SIZE)  {
		offset = GD32_NV_FLASH_V2_BANK0_SIZE;
	}

	ret = gd32_fmc_v2_bank1_erase_block(offset, size1);
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_gd32_pages_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = gd32_fmc_v2_layout;
	*layout_size = ARRAY_SIZE(gd32_fmc_v2_layout);

}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
