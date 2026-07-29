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

#ifndef ZEPHYR_INCLUDE_DRIVERS_FLASH_IT51XXX_FLASH_API_EX_H_
#define ZEPHYR_INCLUDE_DRIVERS_FLASH_IT51XXX_FLASH_API_EX_H_

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

/** number of supported protection paths */
#define IT51XXX_PROTECT_PATH_COUNT 3
/** EC protection path */
#define PROTECT_PATH_EC            BIT(0)
/** Host protection path */
#define PROTECT_PATH_HOST          BIT(1)
/** DBGR protection path */
#define PROTECT_PATH_DBGR          BIT(2)
/** All protection paths */
#define PROTECT_PATH_ALL           (PROTECT_PATH_EC | PROTECT_PATH_HOST | PROTECT_PATH_DBGR)

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
	/**
	 * Write protection.
	 */
	FLASH_IT51XXX_WRITE_PROTECT,
	/**
	 * Read protection.
	 */
	FLASH_IT51XXX_READ_PROTECT,
};

/**
 * @brief flash address protection request/result
 */
struct flash_it51xxx_ex_op_addr_protection {
	/** start address of the protection region */
	uint32_t addr;
	/** size of the protection region in bytes */
	size_t size;
	/** protection path bitmap (see PROTECT_PATH_*) */
	uint8_t path;
	/** protection status of the specified region */
	bool is_protected;
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FLASH_IT51XXX_FLASH_API_EX_H_ */
