/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Realtek RTS5912 flash extended operations.
 * @ingroup rts5912_flash_ex_op
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_RTS5912_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_RTS5912_FLASH_API_EX_H__

/**
 * @brief Extended operations for Realtek RTS5912 flash controller.
 * @defgroup rts5912_flash_ex_op Realtek RTS5912
 * @ingroup flash_ex_op
 * @{
 */

/**
 * @brief Enumeration for Realtek RTS5912 flash extended operations.
 */
enum flash_rts5912_ex_ops {
	/**
	 * @brief Sends the Write Enable (WREN) command to the flash chip.
	 *
	 * This operation sets the Write Enable Latch (WEL) bit in the flash
	 * memory's status register, allowing write and erase operations.
	 *
	 * @param in  Not used.
	 * @param out Not used.
	 */
	FLASH_RTS5912_EX_OP_WR_ENABLE = FLASH_EX_OP_VENDOR_BASE,

	/**
	 * @brief Sends the Write Disable (WRDI) command to the flash chip.
	 *
	 * This operation resets the Write Enable Latch (WEL) bit, protecting
	 * the memory from being written or erased.
	 *
	 * @param in  Not used.
	 * @param out Not used.
	 */
	FLASH_RTS5912_EX_OP_WR_DISABLE,

	/**
	 * @brief Writes to the flash Status Register 1 (SR1).
	 *
	 * This operation writes a single byte to the main status register.
	 *
	 * @param in  Not used.
	 * @param out A pointer to a uint8_t containing the value to be written
	 *            to SR1.
	 */
	FLASH_RTS5912_EX_OP_WR_SR,

	/**
	 * @brief Writes to the flash Status Register 2 (SR2).
	 *
	 * This operation writes a single byte to the second status register.
	 *
	 * @param in  Not used.
	 * @param out A pointer to a uint8_t containing the value to be written
	 *            to SR2.
	 */
	FLASH_RTS5912_EX_OP_WR_SR2,

	/**
	 * @brief Reads the flash Status Register 1 (SR1).
	 *
	 * This operation reads a single byte from the main status register.
	 *
	 * @param in  Not used.
	 * @param out A pointer to a uint8_t buffer where the read value of
	 *            SR1 will be stored.
	 */
	FLASH_RTS5912_EX_OP_RD_SR,

	/**
	 * @brief Reads the flash Status Register 2 (SR2).
	 *
	 * This operation reads a single byte from the second status register.
	 *
	 * @param in  Not used.
	 * @param out A pointer to a uint8_t buffer where the read value of
	 *            SR2 will be stored.
	 */
	FLASH_RTS5912_EX_OP_RD_SR2,

	/**
	 * @brief Sets the hardware Write Protect (WP#) pin state.
	 *
	 * This operation controls the controller's hardware WP# pin output.
	 *
	 * @param in  Not used.
	 * @param out A pointer to a uint8_t. If the value is non-zero,
	 *            the WP# pin is asserted (active low). If zero, it is
	 *            de-asserted.
	 */
	FLASH_RTS5912_EX_OP_SET_WP,

	/**
	 * @brief Gets the hardware Write Protect (WP#) pin state.
	 *
	 * This operation reads the current state of the controller's
	 * hardware WP# pin setting.
	 *
	 * @param in  Not used.
	 * @param out A pointer to a uint8_t buffer to store the current
	 *            WP# pin state. A non-zero value indicates the pin
	 *            is configured to be asserted.
	 */
	FLASH_RTS5912_EX_OP_GET_WP,
};

/**
 * @}
 */

#endif /* __ZEPHYR_INCLUDE_DRIVERS_RTS5912_FLASH_API_EX_H__ */
