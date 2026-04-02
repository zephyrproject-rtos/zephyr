/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for IT51XXX extended operations.
 * @ingroup it51xxx_flash_ex_op
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_IT51XXX_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_IT51XXX_FLASH_API_EX_H__

/**
 * @brief Extended operations for IT51XXX flash controllers.
 * @defgroup it51xxx_flash_ex_op IT51XXX
 * @ingroup flash_ex_op
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>

/**
 * @enum flash_it51xxx_ex_op
 * @brief Enumeration for IT51XXX flash extended operations.
 *
 * Defines which flash device is accessed by IT51XXX flash controller,
 * and the addressing mode for IT51XXX flash operations.
 */
enum flash_it51xxx_ex_op {
	/**
	 * Access the internal SPI e-flash.
	 *
	 * This operation targets the on-chip embedded SPI flash integrated
	 * inside the IT51XXX SoC.
	 */
	FLASH_IT51XXX_INTERNAL,
	/**
	 * Access the external SPI flash connected to FSPI CS0#.
	 *
	 * This operation targets an external flash device connected to the
	 * IT51XXX FSPI controller chip select 0.
	 */
	FLASH_IT51XXX_EXTERNAL_FSPI_CS0,
	/**
	 * Access the external SPI flash connected to FSPI CS1#.
	 *
	 * This operation targets an external flash device connected to the
	 * IT51XXX FSPI controller chip select 1.
	 */
	FLASH_IT51XXX_EXTERNAL_FSPI_CS1,
	/**
	 * 3-byte (24-bit) addressing mode.
	 *
	 * This mode supports flash devices up to 16MB capacity, using
	 * 24-bit address cycles.
	 */
	FLASH_IT51XXX_ADDR_3B,
	/**
	 * 4-byte (32-bit) addressing mode.
	 *
	 * This mode supports larger flash devices (>16MB capacity),
	 * requiring 32-bit address cycles.
	 */
	FLASH_IT51XXX_ADDR_4B,
};

/**
 * @}
 */

#endif /* __ZEPHYR_INCLUDE_DRIVERS_IT51XXX_FLASH_API_EX_H__ */
