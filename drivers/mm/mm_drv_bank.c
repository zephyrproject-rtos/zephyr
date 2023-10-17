/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Module for tracking page use within memory banks
 *
 * The memory management drivers may use the routines within this module
 * to track page use within their memory banks. This information in turn
 * could be leveraged by them to determine when to power them on or off to
 * better conserve energy.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mm/mm_drv_bank.h>
#include <zephyr/sys/mem_stats.h>

void sys_mm_drv_bank_init(struct sys_mm_drv_bank *bank, uint32_t bank_pages)
{
	bank->unmapped_pages = 0;
	bank->mapped_pages = bank_pages;
	bank->max_mapped_pages = bank_pages;
}

uint32_t sys_mm_drv_bank_page_mapped(struct sys_mm_drv_bank *bank)
{
	bank->unmapped_pages--;
	bank->mapped_pages++;
	if (bank->mapped_pages > bank->max_mapped_pages) {
		bank->max_mapped_pages = bank->mapped_pages;
	}
	return bank->mapped_pages;
}

uint32_t sys_mm_drv_bank_page_unmapped(struct sys_mm_drv_bank *bank)
{
	bank->unmapped_pages++;
	bank->mapped_pages--;
	return bank->unmapped_pages;
}

void sys_mm_drv_bank_stats_get(struct sys_mm_drv_bank *bank,
			       struct sys_memory_stats *stats)
{
	stats->free_bytes          = bank->unmapped_pages *
				     CONFIG_MM_DRV_PAGE_SIZE;
	stats->allocated_bytes     = bank->mapped_pages *
				     CONFIG_MM_DRV_PAGE_SIZE;
	stats->max_allocated_bytes = bank->max_mapped_pages *
				     CONFIG_MM_DRV_PAGE_SIZE;
}

void sys_mm_drv_bank_stats_reset_max(struct sys_mm_drv_bank *bank)
{
	bank->max_mapped_pages = bank->mapped_pages;
}
