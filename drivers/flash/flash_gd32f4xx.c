/*
 * Copyright (c) 2022 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include "flash_gd32.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(flash_gd32);

/* Considered move erase timeout to dts, but currently just push it in here. */
#define GD32F4XX_FLASH_TIMEOUT 16000

#ifdef CONFIG_FLASH_PAGE_LAYOUT
#if SOC_NV_FLASH_SIZE <= (512 * 1024)
static const struct flash_pages_layout flash_gd32f4xx_layout[] = {
	/* GD32F405xE, GD32F407xE, GD32F450xE */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 3, .pages_size = KB(128)},
};
#elif SOC_NV_FLASH_SIZE <= (1024 * 1024)
static const struct flash_pages_layout flash_gd32f4xx_layout[] = {
	/* GD32F405xG, GD32F407xG, GD32F450xG */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#elif SOC_NV_FLASH_SIZE <= (2048 * 1024)
static const struct flash_pages_layout flash_gd32f4xx_layout[] = {
	/* GD32F450xI */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
};
#elif SOC_NV_FLASH_SIZE <= (3072 * 1024)
static const struct flash_pages_layout flash_gd32f4xx_layout[] = {
	/* GD32F405xK, GD32F407xK, GD32F450xK */
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
	{.pages_count = 4, .pages_size = KB(16)},
	{.pages_count = 1, .pages_size = KB(64)},
	{.pages_count = 7, .pages_size = KB(128)},
	{.pages_count = 4, .pages_size = KB(256)},
};
#else
#error "Unknown flash size for GD32F4xx series."
#endif
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

/* SN bits in FMC_CTL are not continues value, use below table to mapping it. */
static uint8_t flash_gd32f4xx_sectors_id[] = {
	0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U,
	16U, 17U, 18U, 19U, 20U, 21U, 22U, 23U, 24U, 25U, 26U, 27U,
	12U, 13U, 14U, 15U
};

static void flash_gd32_err_handler(void)
{
	if (FMC_STAT & FMC_STAT_WPERR) {
		FMC_STAT &= ~FMC_STAT_WPERR;
		LOG_ERR("WPERR: erase/program on protected pages.");
	}

	if (FMC_STAT & FMC_STAT_PGMERR) {
		FMC_STAT &= ~FMC_STAT_PGMERR;
		LOG_ERR("PGMERR: program write size size not match.");
	}

	if (FMC_STAT & FMC_STAT_PGSERR) {
		FMC_STAT &= ~FMC_STAT_PGSERR;
		LOG_ERR("PGSERR: PG bit not set.");
	}

	if (FMC_STAT & FMC_STAT_RDDERR) {
		FMC_STAT &= ~FMC_STAT_RDDERR;
		LOG_ERR("RDDERR: read protection sector.");
	}

	FMC_STAT &= ~FMC_STAT_OPERR;
}

int flash_gd32_programming(off_t offset, const void *data, size_t len)
{
	int64_t timeout_time = k_uptime_get() + GD32F4XX_FLASH_TIMEOUT;
	flash_prog_t *src = (flash_prog_t *)data;
	flash_prog_t *dst = (flash_prog_t *)SOC_NV_FLASH_ADDR + offset;

	if (FMC_STAT & FMC_STAT_BUSY) {
		return -EBUSY;
	}

	/* Enable flash programming */
	FMC_CTL |= FMC_CTL_PG;

	for (size_t i = 0; i < (len / sizeof(flash_prog_t)); i++) {
		*dst++ = *src++;
	}

	/* Wait for the programming complete. */
	while (FMC_STAT & FMC_STAT_BUSY) {
		if (k_uptime_get() >= timeout_time) {
			return -EIO;
		}
	}

	/* Disable flash programming */
	FMC_CTL &= ~FMC_CTL_PG;

	if (FMC_STAT & FMC_STAT_OPERR) {
		flash_gd32_err_handler();
		return -EIO;
	}

	return 0;
}

int flash_gd32_page_erase(uint32_t sector)
{
	int64_t timeout_time = k_uptime_get() + GD32F4XX_FLASH_TIMEOUT;

	if (FMC_STAT & FMC_STAT_BUSY) {
		return -EBUSY;
	}

	FMC_CTL |= FMC_CTL_SER;

	FMC_CTL &= ~FMC_CTL_SN;
	FMC_CTL |= CTL_SN(flash_gd32f4xx_sectors_id[sector]);

	FMC_CTL |= FMC_CTL_START;

	/* Wait for the sector erase complete. */
	while (FMC_STAT & FMC_STAT_BUSY) {
		if (k_uptime_get() >= timeout_time) {
			return -EIO;
		}
	}

	if (FMC_STAT & FMC_STAT_OPERR) {
		flash_gd32_err_handler();
		return -EIO;
	}

	/* Verify the erased sector is correct. */
	if (CTL_SN(flash_gd32f4xx_sectors_id[sector]) != (FMC_CTL & FMC_CTL_SN)) {
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_gd32_pages_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = flash_gd32f4xx_layout;
	*layout_size = ARRAY_SIZE(flash_gd32f4xx_layout);
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
