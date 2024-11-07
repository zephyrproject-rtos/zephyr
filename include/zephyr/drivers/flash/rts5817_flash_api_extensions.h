/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Realtek RTS5817 flash extended operations.
 * @ingroup rts5817_flash_ex_op
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_RTS5817_FLASH_API_EXTENSIONS_H__
#define __ZEPHYR_INCLUDE_DRIVERS_RTS5817_FLASH_API_EXTENSIONS_H__

/**
 * @brief Extended operations for Realtek RTS5817 flash controller.
 * @defgroup rts5817_flash_ex_op Realtek RTS5817
 * @ingroup flash_ex_op
 * @{
 */

/**
 * @brief Enumeration for Realtek RTS5817 flash extended operations.
 */

enum flash_rts5817_ex_ops {
	/**
	 * @brief Writes to the flash Status Register(s).
	 *
	 * This operation is used to set flash status register(s).
	 *
	 * @param in  A pointer to a uint8_t buffer containing the value to be written.
	 *            The buffer length depends on the flash.
	 * @param out Not used.
	 */
	FLASH_RTS5817_EX_OP_WR_SR = FLASH_EX_OP_VENDOR_BASE,

	/**
	 * @brief Reads the flash Status Register(s).
	 *
	 * This operation is used to read flash status register(s).
	 *
	 * @param in  Not used.
	 * @param out A pointer to a uint8_t buffer where the read value of
	 *            flash status register(s) will be stored.The buffer length
	 *            depends on the flash.
	 */
	FLASH_RTS5817_EX_OP_RD_SR,

	/**
	 * @brief Reads the flash Unique ID.
	 *
	 * This operation is used to read flash unique ID.
	 *
	 * @param in  The length of the buffer.
	 * @param out A pointer to a uint8_t buffer where the unique ID of
	 *            flash will be stored.
	 */
	FLASH_RTS5817_EX_OP_RD_UID,

	/**
	 * @brief Configs the flash software protection bits.
	 *
	 * This operation sets the software protection state of flash via setting BP bits.
	 *
	 * @param in  A pointer to a uint8_t. If the value is non-zero, write protection is
	 *            enabled.
	 * @param out Not used.
	 */
	FLASH_RTS5817_EX_OP_SOFT_PROTECT,

	/**
	 * @brief Sets the hardware Write Protect (WP#) pin state.
	 *
	 * This operation controls the controller's hardware WP# pin output.
	 *
	 * @param in  A pointer to a @ref flash_rts5817_wp_state structure.
	 * @param out Not used.
	 */
	FLASH_RTS5817_EX_OP_WP_PIN_SET,

	/**
	 * @brief Gets the hardware Write Protect (WP#) pin state.
	 *
	 * This operation reads the current state of the controller's
	 * hardware WP# pin setting.
	 *
	 * @param in  Not used.
	 * @param out A pointer to a @ref flash_rts5817_wp_state structure.
	 */
	FLASH_RTS5817_EX_OP_WP_PIN_GET,
};

/**
 * @brief Enumeration for hardware Write Protect (WP#) pin state.
 */
enum flash_rts5817_wp_state {
	/** WP# state low */
	FLASH_RTS5817_WP_LOW = 0,
	/** WP# state high */
	FLASH_RTS5817_WP_HIGH = 1,
};

/**
 * @}
 */

#endif /* __ZEPHYR_INCLUDE_DRIVERS_RTS5817_FLASH_API_EXTENSIONS_H__ */
