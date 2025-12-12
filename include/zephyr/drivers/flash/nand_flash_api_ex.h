/*
 * Copyright (c) 2025 Endress+Hauser GmbH+Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NAND flash specific API.
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_NAND_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_NAND_FLASH_API_EX_H__

#include <zephyr/drivers/flash.h>

/**
 * @brief Enumeration for NAND flash extended operations.
 */
enum nand_flash_ex_ops {
	/**
	 * Checks whether a block is marked as bad. As input it takes the block index (size_t *). As
	 * output it returns 1 if the block is bad, 0 otherwise (int *).
	 */
	NAND_FLASH_IS_BAD_BLOCK = FLASH_EX_OP_VENDOR_BASE,

	/**
	 * Marks a block as bad. As input it takes the block index (size_t *). There is no output.
	 */
	NAND_FLASH_MARK_BAD_BLOCK,
};

/**
 * @brief Enumeration for NAND flash block status.
 */
enum nand_flash_block_status {
	/**
	 * Block is functional.
	 */
	NAND_BLOCK_GOOD = 0,

	/**
	 * Block is marked as bad.
	 */
	NAND_BLOCK_BAD = 1,
};

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

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_NAND_FLASH_API_EX_H__ */
