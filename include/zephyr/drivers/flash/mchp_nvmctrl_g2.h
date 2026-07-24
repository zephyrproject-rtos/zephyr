/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file supporting ex_op for Microchip flash-controller G2 driver
 * @ingroup mchp_flash_interface
 *
 * This header provides definitions for additional flash memory
 * operations specific to the Microchip NVMCTRL G2 flash controller.
 * It extends the standard flash driver capabilities by enabling
 * advanced operations such as region locking/unlocking, clear
 * page buffer, etc.
 *
 * @note This file should only be included when targeting devices
 *       with the NVMCTRL G2 flash controller.
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G2_H_
#define INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G2_H_

/**
 * @brief Extended operations for Microchip G3 flash memory controllers.
 * @defgroup mchp_flash_interface Microchip
 * @ingroup flash_ex_op
 * @since 4.5
 * @version 0.1.0
 * @{
 */

/**
 * This enumeration defines the set of extended operations that can be performed
 * on the flash memory, such as erasing or writing the user row, and locking or
 * unlocking specific flash regions.
 */
typedef enum {
	FLASH_EX_OP_REGION_LOCK,               /**< Lock a specific region */
	FLASH_EX_OP_REGION_UNLOCK,             /**< Unlock a specific region */
	FLASH_EX_OP_SET_PWR_REDUCTION_MODE,    /**< Sets the Power Reduction mode */
	FLASH_EX_OP_CLR_PWR_REDUCTION_MODE,    /**< Clears the Power Reduction mode */
	FLASH_EX_OP_CLR_PAGE_BUFFER,           /**< Clears the flash page buffer */
	FLASH_EX_OP_SET_SECURITY_BIT,          /**< Set Security Bit */
	FLASH_EX_OP_INVALIDATE_CACHE_LINES,    /**< Invalidates all cache lines */
	FLASH_EX_OP_SET_CHIP_ERASE_HARDLOCK,   /**< Set Chip Erase Hard Lock */
} flash_mchp_ex_ops_t;

/**
 * @}
 */

#endif /* INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G2_H_ */
