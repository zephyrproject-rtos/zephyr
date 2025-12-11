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
 * driver to track page usage within memory banks.
 *
 * @note The caller of these functions needs to ensure proper locking
 *       to protect the data when using these APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MM_DRV_BANK_H
#define ZEPHYR_INCLUDE_DRIVERS_MM_DRV_BANK_H

#include <zephyr/kernel.h>
#include <zephyr/sys/mem_stats.h>
#include <stdint.h>

/**
 * @brief Memory Banks Driver APIs
 * @defgroup mm_drv_bank_apis Memory Banks Driver APIs
 *
 * This contains APIs for a system-wide memory management driver to
 * track page usage within memory banks.
 *
 * @note The caller of these functions needs to ensure proper locking
 *       to protect the data when using these APIs.
 *
 * @ingroup memory_management
 * @{
 */

/**
 * @brief Information about memory banks.
 */
struct sys_mm_drv_bank {
	/** Number of unmapped pages. */
	uint32_t  unmapped_pages;

	/** Number of mapped pages. */
	uint32_t  mapped_pages;

	/** Maximum number of mapped pages since last counter reset. */
	uint32_t  max_mapped_pages;
};

/**
 * @brief Initialize a memory bank's data structure
 *
 * The driver may wish to track various information about the memory banks
 * that it uses. This routine will initialize a generic structure containing
 * that information. Since at the power on all memory banks are switched on
 * it will start with all pages mapped. In next phase of driver initialization
 * unused pages will be unmapped.
 *
 * @param[in,out] bank Pointer to the memory bank structure used for tracking
 * @param[in] bank_pages Number of pages in the memory bank
 */
void sys_mm_drv_bank_init(struct sys_mm_drv_bank *bank, uint32_t bank_pages);

/**
 * @brief Track the mapping of a page in the specified memory bank
 *
 * This function is used to update the number of mapped pages within the
 * specified memory bank.
 *
 * @param[in,out] bank Pointer to the memory bank's data structure
 *
 * @return The number of pages mapped within the memory bank
 */
uint32_t sys_mm_drv_bank_page_mapped(struct sys_mm_drv_bank *bank);

/**
 * @brief Track the unmapping of a page in the specified memory bank
 *
 * This function is used to update the number of unmapped pages within the
 * specified memory bank.
 *
 * @param[in,out] bank Pointer to the memory bank's data structure
 *
 * @return The number of unmapped pages within the memory bank
 */
uint32_t sys_mm_drv_bank_page_unmapped(struct sys_mm_drv_bank *bank);

/**
 * @brief Reset the max number of pages mapped in the bank
 *
 * This routine is used to reset the maximum number of pages mapped in
 * the specified memory bank to the current number of pages mapped in
 * that memory bank.
 *
 * @param[in,out] bank Pointer to the memory bank's data structure
 */
void sys_mm_drv_bank_stats_reset_max(struct sys_mm_drv_bank *bank);

/**
 * @brief Retrieve the memory usage stats for the specified memory bank
 *
 * This routine extracts the system memory stats from the memory bank.
 *
 * @param[in] bank Pointer to the memory bank's data structure
 * @param[in,out] stats Pointer to memory into which to copy the system memory stats
 */
void sys_mm_drv_bank_stats_get(struct sys_mm_drv_bank *bank,
			       struct sys_memory_stats *stats);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MM_DRV_BANK_H */
