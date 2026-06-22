/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Microchip NVMCTRL G1 flash extended operations.
 * @ingroup mchp_nvmctrl_g1_flash_ex_op
 *
 * @note This file should only be included when targeting devices
 *       with the NVMCTRL G1 flash controller.
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G1_H_
#define INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G1_H_

/**
 * @brief Extended operations for Microchip NVMCTRL G1 flash controller.
 * @defgroup mchp_nvmctrl_g1_flash_ex_op Microchip NVMCTRL G1
 * @ingroup flash_ex_op
 * @{
 */

/**
 * @brief Enumeration of extended operation codes for the MCHP NVMCTRL G1 flash controller.
 */
typedef enum {
	/**
	 * @brief Erases the entire user row in flash memory.
	 *
	 * This operation erases the user row section of flash memory starting
	 * at the base address defined by SOC_NV_USERROW_BASE_ADDR.
	 *
	 * @param in  Not used.
	 * @param out Not used.
	 */
	FLASH_EX_OP_USER_ROW_ERASE,

	/**
	 * @brief Writes data to the user row in flash memory.
	 *
	 * This operation writes data to the user row section of flash memory.
	 * The offset and data length must be aligned to the user row write block
	 * size (quad-word, typically 16 bytes).
	 *
	 * @param in  Pointer to a @ref flash_mchp_ex_op_userrow_data_t structure
	 *            specifying the target offset, source data buffer, and length.
	 * @param out Not used.
	 */
	FLASH_EX_OP_USER_ROW_WRITE,

	/**
	 * @brief Locks all regions of flash memory.
	 *
	 * This operation iterates over all lock regions of the main flash array
	 * and issues a Lock Region (LR) command for each one.
	 *
	 * @param in  Not used.
	 * @param out Not used.
	 */
	FLASH_EX_OP_REGION_LOCK,

	/**
	 * @brief Unlocks all regions of flash memory.
	 *
	 * This operation iterates over all lock regions of the main flash array
	 * and issues an Unlock Region (UR) command for each one.
	 *
	 * @param in  Not used.
	 * @param out Not used.
	 */
	FLASH_EX_OP_REGION_UNLOCK
} flash_mchp_ex_ops_t;

/**
 * @brief Parameters for user row data operations on the MCHP NVMCTRL G1 flash.
 *
 * Used as the @p in argument to @ref FLASH_EX_OP_USER_ROW_WRITE to specify
 * the source data buffer and the target location within the user row region.
 */
typedef struct flash_mchp_ex_op_userrow_data {
	/** Pointer to the data buffer to be written. */
	const void *data;

	/** Length of the data buffer in bytes. Must be aligned to the write block size. */
	size_t data_len;

	/** Byte offset within the user row region where the write starts. Must be
	 *  aligned to the write block size.
	 */
	off_t offset;
} flash_mchp_ex_op_userrow_data_t;

/**
 * @}
 */

#endif /* INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_NVMCTRL_G1_H_ */
