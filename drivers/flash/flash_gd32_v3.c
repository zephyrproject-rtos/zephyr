/*
 * Copyright (c) 2022 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "flash_gd32.h"

#include <zephyr/logging/log.h>

#include <gd32_fmc.h>

LOG_MODULE_DECLARE(flash_gd32);

#define GD32_NV_FLASH_V3_NODE		DT_INST(0, gd_gd32_nv_flash_v3)
#define GD32_NV_FLASH_V3_TIMEOUT	DT_PROP(GD32_NV_FLASH_V3_NODE, max_erase_time_ms)

/**
 * @brief GD32 FMC v3 flash memory layout for GD32F4xx series.
 */
#if defined(CONFIG_FLASH_PAGE_LAYOUT) && \
	defined(CONFIG_SOC_SERIES_GD32F4XX)
#if (PRE_KB(512) == SOC_NV_FLASH_SIZE)
static const struct flash_pages_layout gd32_fmc_v3_layout[] = {
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 3, .pages_size = KB(128)},
};
#elif (PRE_KB(1024) == SOC_NV_FLASH_SIZE)
static const struct flash_pages_layout gd32_fmc_v3_layout[] = {
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#elif (PRE_KB(2048) == SOC_NV_FLASH_SIZE)
static const struct flash_pages_layout gd32_fmc_v3_layout[] = {
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#elif (PRE_KB(3072) == SOC_NV_FLASH_SIZE)
static const struct flash_pages_layout gd32_fmc_v3_layout[] = {
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
	{.pages_count = 4, .pages_size = KB(256)},
};
#else
#error "Unknown FMC layout for GD32F4xx series."
#endif
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#define gd32_fmc_v3_WRITE_ERR (FMC_STAT_PGMERR | FMC_STAT_PGSERR | FMC_STAT_WPERR)
#define gd32_fmc_v3_ERASE_ERR FMC_STAT_OPERR

/* SN bits in FMC_CTL are not continue values, use table below to map them. */
static uint8_t gd32_fmc_v3_sectors[] = {
	0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U,
	16U, 17U, 18U, 19U, 20U, 21U, 22U, 23U, 24U, 25U, 26U, 27U,
	12U, 13U, 14U, 15U
};

static inline void gd32_fmc_v3_unlock(void)
{
	FMC_KEY = UNLOCK_KEY0;
	FMC_KEY = UNLOCK_KEY1;
}

static inline void gd32_fmc_v3_lock(void)
{
	FMC_CTL |= FMC_CTL_LK;
}

static int gd32_fmc_v3_wait_idle(void)
{
	const int64_t expired_time = k_uptime_get() + GD32_NV_FLASH_V3_TIMEOUT;

	while (FMC_STAT & FMC_STAT_BUSY) {
		if (k_uptime_get() > expired_time) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

bool flash_gd32_valid_range(off_t offset, uint32_t len, bool write)
{
	const struct flash_pages_layout *page_layout;
	uint32_t cur = 0U, next = 0U;

	if ((offset > SOC_NV_FLASH_SIZE) ||
	    ((offset + len) > SOC_NV_FLASH_SIZE)) {
		return false;
	}

	if (write) {
		/* Check offset and len aligned to write-block-size. */
		if ((offset % sizeof(flash_prg_t)) ||
		    (len % sizeof(flash_prg_t))) {
			return false;
		}

	} else {
		for (size_t i = 0; i < ARRAY_SIZE(gd32_fmc_v3_layout); i++) {
			page_layout = &gd32_fmc_v3_layout[i];

			for (size_t j = 0; j < page_layout->pages_count; j++) {
				cur = next;

				next += page_layout->pages_size;

				/* Check bad offset. */
				if ((offset > cur) && (offset < next)) {
					return false;
				}

				/* Check bad len. */
				if (((offset + len) > cur) &&
				    ((offset + len) < next)) {
					return false;
				}

				if ((offset + len) == next) {
					return true;
				}
			}
		}
	}

	return true;
}

int flash_gd32_write_range(off_t offset, const void *data, size_t len)
{
	flash_prg_t *prg_flash = (flash_prg_t *)((uint8_t *)SOC_NV_FLASH_ADDR + offset);
	flash_prg_t *prg_data = (flash_prg_t *)data;
	int ret = 0;

	gd32_fmc_v3_unlock();

	if (FMC_STAT & FMC_STAT_BUSY) {
		return -EBUSY;
	}

	FMC_CTL |= FMC_CTL_PG;

	FMC_CTL &= ~FMC_CTL_PSZ;
	FMC_CTL |= CTL_PSZ(sizeof(flash_prg_t) - 1);

	for (size_t i = 0U; i < (len / sizeof(flash_prg_t)); i++) {
		*prg_flash++ = *prg_data++;
	}

	ret = gd32_fmc_v3_wait_idle();
	if (ret < 0) {
		goto expired_out;
	}

	if (FMC_STAT & gd32_fmc_v3_WRITE_ERR) {
		ret = -EIO;
		FMC_STAT |= gd32_fmc_v3_WRITE_ERR;
		LOG_ERR("FMC programming failed");
	}

expired_out:
	FMC_CTL &= ~FMC_CTL_PG;

	gd32_fmc_v3_lock();

	return ret;
}

static int gd32_fmc_v3_sector_erase(uint8_t sector)
{
	int ret = 0;

	gd32_fmc_v3_unlock();

	if (FMC_STAT & FMC_STAT_BUSY) {
		return -EBUSY;
	}

	FMC_CTL |= FMC_CTL_SER;

	FMC_CTL &= ~FMC_CTL_SN;
	FMC_CTL |= CTL_SN(sector);

	FMC_CTL |= FMC_CTL_START;

	ret = gd32_fmc_v3_wait_idle();
	if (ret < 0) {
		goto expired_out;
	}

	if (FMC_STAT & gd32_fmc_v3_ERASE_ERR) {
		ret = -EIO;
		FMC_STAT |= gd32_fmc_v3_ERASE_ERR;
		LOG_ERR("FMC sector %u erase failed", sector);
	}

expired_out:
	FMC_CTL &= ~FMC_CTL_SER;

	gd32_fmc_v3_lock();

	return ret;
}

int flash_gd32_erase_block(off_t offset, size_t size)
{
	const struct flash_pages_layout *page_layout;
	uint32_t erase_offset = 0U;
	uint8_t counter = 0U;
	int ret = 0;

	for (size_t i = 0; i < ARRAY_SIZE(gd32_fmc_v3_layout); i++) {
		page_layout = &gd32_fmc_v3_layout[i];

		for (size_t j = 0; j < page_layout->pages_count; j++) {
			if (erase_offset < offset) {
				counter++;
				erase_offset += page_layout->pages_size;

				continue;
			}

			uint8_t sector = gd32_fmc_v3_sectors[counter++];

			ret = gd32_fmc_v3_sector_erase(sector);
			if (ret < 0) {
				return ret;
			}

			erase_offset += page_layout->pages_size;

			if (erase_offset - offset >= size) {
				return 0;
			}
		}
	}

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_gd32_pages_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = gd32_fmc_v3_layout;
	*layout_size = ARRAY_SIZE(gd32_fmc_v3_layout);
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
