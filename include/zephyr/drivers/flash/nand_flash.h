/*
 * Copyright (c) 2025 Endress+Hauser GmbH+Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NAND flash specific data structures.
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_NAND_FLASH_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_NAND_FLASH_H__

#include <zephyr/drivers/flash.h>

/**
 * @brief Structure representing a NAND flash address.
 */
struct nand_flash_address {
	/* Page index of address. */
	uint16_t page;

	/* Block index of address. */
	uint16_t block;

	/* Plane index of address. */
	uint16_t plane;
};

/**
 * @brief Structure representing a NAND flash feature.
 */
struct nand_flash_feature {
	/* Feature address. */
	uint8_t feature_addr;

	/* Feature data. */
	uint8_t feature_data[4];
};

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_NAND_FLASH_H__ */
