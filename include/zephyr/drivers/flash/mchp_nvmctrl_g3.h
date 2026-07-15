/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file supporting ex_op for Microchip flash-controller G3 driver
 * @ingroup mchp_flash_interface
 *
 * This header provides definitions for additional flash memory
 * operations specific to the Microchip NVMCTRL G3 flash controller.
 * It extends the standard flash API capabilities by enabling
 * advanced operations such as and region locking/unlocking, clear
 * page buffer, etc.,
 *
 * @note This API extension shall only be used when targeting devices
 *       with the NVMCTRL G3 flash controller.
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G3_H_
#define INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G3_H_

/**
 * @brief Extended operations for Microchip G3 flash memory controllers.
 * @defgroup mchp_flash_interface Microchip
 * @ingroup flash_ex_op
 * @since 4.5
 * @version 0.1.0
 * @{
 */

#include <zephyr/types.h>
#include <stdbool.h>

/**
 * This enumeration defines the set of extended operations that can be performed
 * on the flash memory, such as erasing or writing the user row, and locking or
 * unlocking specific flash regions.
 */
enum flash_mchp_ex_ops_t {
	/** Enable write protection for a PFM region */
	FLASH_EX_OP_PFM_REGION_WRITE_PROTECT_ENABLE,
	/** Disable write protection for a PFM region */
	FLASH_EX_OP_PFM_REGION_WRITE_PROTECT_DISABLE,
	/** Lock write protection settings for a PFM region */
	FLASH_EX_OP_PFM_REGION_WRITE_PROTECT_LOCK,
	/** Enable write protection for BFM */
	FLASH_EX_OP_BFM_WRITE_PROTECT_ENABLE,
	/** Disable write protection for BFM */
	FLASH_EX_OP_BFM_WRITE_PROTECT_DISABLE,
	/** Lock write protection settings for BFM */
	FLASH_EX_OP_BFM_WRITE_PROTECT_LOCK,
};

/**
 * @brief Program Flash Memory (PFM) write protection regions.
 *
 * This enumeration defines the available write protection regions
 * for the Program Flash Memory. Each region can be independently
 * configured for write protection.
 */
enum pfm_wp_region {
	PFM_WP_REGION_0, /**< PFM write protection region 0 */
	PFM_WP_REGION_1, /**< PFM write protection region 1 */
	PFM_WP_REGION_2, /**< PFM write protection region 2 */
	PFM_WP_REGION_3, /**< PFM write protection region 3 */
	PFM_WP_REGION_4, /**< PFM write protection region 4 */
	PFM_WP_REGION_5, /**< PFM write protection region 5 */
	PFM_WP_REGION_6, /**< PFM write protection region 6 */
	PFM_WP_REGION_7, /**< PFM write protection region 7 */
};

/**
 * @brief Boot Flash Memory (BFM) bank selection.
 *
 * This enumeration defines the available boot flash memory banks/panels.
 * The boot flash memory can be organized into multiple banks for redundancy
 * and fail-safe boot operations.
 */
enum boot_flash_bank {
	BOOT_FLASH_BANK_1, /**< Boot Flash Memory Bank/Panel 1 */
	BOOT_FLASH_BANK_2  /**< Boot Flash Memory Bank/Panel 2 */
};

/**
 * @brief Program Flash Memory (PFM) extended operation parameters.
 *
 * This structure contains the parameters required for performing
 * extended operations on Program Flash Memory regions, such as
 * configuring write protection regions.
 */
struct pfm_exop {
	/** Write protection region selector */
	enum pfm_wp_region region;
	/** Base address of the write protection region */
	uint32_t base_addr;
	/** Size of the write protection region in bytes */
	uint32_t region_size;
	/** Enable mirroring for this region */
	bool mirror_enable;
};

/**
 * @brief Boot Flash Memory (BFM) extended operation parameters.
 *
 * This structure contains the parameters required for performing
 * extended operations on Boot Flash Memory, such as configuring
 * write protection and selecting the boot bank.
 */
struct bfm_exop {
	/** Boot flash bank/panel selection */
	enum boot_flash_bank boot_bank;
	/** Write protection page configuration */
	uint32_t wp_page;
};

/**
 * @}
 */

#endif /*INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G3_H_*/
