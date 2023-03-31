/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Memory Banks Driver APIs
 *
 * This contains generic APIs to be used by a system-wide memory management
 * driver to track page usage within memory banks. It is incumbent upon the
 * caller to ensure that proper locking is used to protect the data when
 * using these APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MM_DRV_BANK_H
#define ZEPHYR_INCLUDE_DRIVERS_MM_DRV_BANK_H

#include <zephyr/kernel.h>
#include <zephyr/sys/mem_stats.h>
#include <stdint.h>

struct mem_drv_bank {
	uint32_t  unmapped_pages;
	uint32_t  mapped_pages;
	uint32_t  max_mapped_pages;
};

/**
 * @brief Initialize a memory bank's data structure
 *
 * The driver may wish to track various information about the memory banks
 * that it uses. This routine will initialize a generic structure containing
 * that information.
 *
 * @param bank Pointer to the memory bank structure used for tracking
 * @param bank_pages Number of pages in the memory bank
 */
void sys_mm_drv_bank_init(struct mem_drv_bank *bank, uint32_t bank_pages);

/**
 * @brief Track the mapping of a page in the specified memory bank
 *
 * This function is used to update the number of mapped pages within the
 * specified memory bank.
 *
 * @param bank Pointer to the memory bank's data structure
 *
 * @return The number of pages mapped within the memory bank
 */
uint32_t sys_mm_drv_bank_page_mapped(struct mem_drv_bank *bank);

/**
 * @brief Track the unmapping of a page in the specified memory bank
 *
 * This function is used to update the number of unmapped pages within the
 * specified memory bank.
 *
 * @param bank Pointer to the memory bank's data structure
 *
 * @return The number of unmapped pages within the memory bank
 */
uint32_t sys_mm_drv_bank_page_unmapped(struct mem_drv_bank *bank);

/**
 * @brief Reset the max number of pages mapped in the bank
 *
 * This routine is used to reset the maximum number of pages mapped in
 * the specified memory bank to the current number of pages mapped in
 * that memory bank.
 *
 * @param bank Pointer to the memory bank's data structure
 */
void sys_mm_drv_bank_stats_reset_max(struct mem_drv_bank *bank);

/**
 * @brief Retrieve the memory usage stats for the specified memory bank
 *
 * This routine extracts the system memory stats from the memory bank.
 *
 * @param bank Pointer to the memory bank's data structure
 * @param stats Pointer to memory into which to copy the system memory stats
 */
void sys_mm_drv_bank_stats_get(struct mem_drv_bank *bank,
			       struct sys_memory_stats *stats);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MM_DRV_BANK_H */
