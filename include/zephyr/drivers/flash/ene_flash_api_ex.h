/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for ENE flash extended operations.
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_ENE_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_ENE_FLASH_API_EX_H__

#include <zephyr/drivers/flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extended operations for ENE flash controllers.
 * @defgroup ene_flash_ex_op ENE Flash Extended Operations
 * @ingroup flash_interface
 * @{
 */

/**
 * @brief Enumeration for ENE flash extended operations.
 */
enum flash_ene_ex_ops {
	/** Configure flash protection regions. */
	FLASH_ENE_EX_OP_PROTECT_CONFIG = FLASH_EX_OP_VENDOR_BASE,
	/** Get current flash protection configuration. */
	FLASH_ENE_EX_OP_PROTECT_GET,
};

/**
 * @brief Parameters for flash protection operations.
 *
 * Defines the content for @ref FLASH_ENE_EX_OP_PROTECT_CONFIG and
 * @ref FLASH_ENE_EX_OP_PROTECT_GET operations.
 */
struct ene_ex_ops_protect_config {
	/** Region number to configure (0 to @ref MAX_PROTECT_REGION_NUM). */
	uint8_t region_num;
	/** Starting page index of the protection region. */
	uint16_t start_page_index;
	/** Ending page index of the protection region. */
	uint16_t end_page_index;
	/** Protection type bitmask (see ENE Flash protect type control bits). */
	uint16_t protect_type;
	/** Lock the configuration to prevent further changes. */
	bool config_lock;
};

/**
 * @name ENE Flash protect type control bits
 * @{
 */
/** Host Interface Write Protect. */
#define HIFWP    BIT(13)
/** Host Interface Read Protect. */
#define HIFRP    BIT(12)
/** I2C Port D32 Write Protect. */
#define I2CD32WP BIT(11)
/** I2C Port D32 Read Protect. */
#define I2CD32RP BIT(10)
/** EDI Interface Write Protect. */
#define EDI32WP  BIT(9)
/** EDI Interface Read Protect. */
#define EDI32RP  BIT(8)
/** DMA Write Protect. */
#define DMAWP    BIT(7)
/** DMA Read Protect. */
#define DMARP    BIT(6)
/** MCU Write Protect. */
#define MCUWP    BIT(5)
/** MCU Read Protect. */
#define MCURP    BIT(4)
/** @} */

/** @brief Maximum number of supported protection regions. */
#define MAX_PROTECT_REGION_NUM 7

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ZEPHYR_INCLUDE_DRIVERS_ENE_FLASH_API_EX_H__ */
