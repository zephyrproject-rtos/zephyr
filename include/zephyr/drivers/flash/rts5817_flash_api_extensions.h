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

#ifndef ZEPHYR_INCLUDE_DRIVERS_FLASH_RTS5817_FLASH_API_EXTENSIONS_H_
#define ZEPHYR_INCLUDE_DRIVERS_FLASH_RTS5817_FLASH_API_EXTENSIONS_H_

#include <zephyr/drivers/flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extended operations for Realtek RTS5817 flash controller.
 * @defgroup rts5817_flash_ex_op Realtek RTS5817
 * @since 4.5
 * @version 0.1.0
 * @ingroup flash_ex_op
 * @{
 */

/**
 * @brief Enumeration for Realtek RTS5817 flash extended operations.
 */
enum flash_rts5817_ex_ops {
	/**
	 * @brief Write the flash status register(s).
	 *
	 * Volatile if the attached flash supports the volatile status register write enable
	 * command (0x50), non-volatile otherwise. Use @ref FLASH_RTS5817_EX_OP_WR_SR_NV to
	 * always write non-volatile. The QE bit is kept matched to the read mode.
	 *
	 * @param in  A pointer to a uint8_t[2] buffer: status register 1 and 2.
	 * @param out Not used.
	 */
	FLASH_RTS5817_EX_OP_WR_SR = FLASH_EX_OP_VENDOR_BASE,

	/**
	 * @brief Read the flash status register(s).
	 *
	 * @param in  Not used.
	 * @param out A pointer to a uint8_t[2] buffer: status register 1 and 2.
	 */
	FLASH_RTS5817_EX_OP_RD_SR,

	/**
	 * @brief Read the flash unique ID.
	 *
	 * @param in  A pointer to a uint8_t holding the number of bytes to read, 1 to 16.
	 * @param out A pointer to a buffer of at least that many bytes for the ID.
	 *
	 * @retval -EINVAL if the length is outside 1 to 16.
	 * @retval -ENOTSUP if the attached flash does not support reading a unique ID.
	 */
	FLASH_RTS5817_EX_OP_RD_UID,

	/**
	 * @brief Configure the flash software write protection.
	 *
	 * Set or clear the block protect (BP) bits of the flash, whether or not
	 * @kconfig{CONFIG_FLASH_RTS5817_WRITE_PROTECTION_AUTO} is enabled.
	 *
	 * @param in  A pointer to a uint8_t: a non-zero value enables write protection, 0
	 *            disables it.
	 * @param out Not used.
	 */
	FLASH_RTS5817_EX_OP_SOFT_PROTECT,

	/**
	 * @brief Write the flash status register(s) non-volatile.
	 *
	 * Same as @ref FLASH_RTS5817_EX_OP_WR_SR, but always uses the write enable command
	 * (0x06), so the value is kept across power cycles and each call counts against the
	 * endurance of the status register.
	 *
	 * @param in  A pointer to a uint8_t[2] buffer: status register 1 and 2.
	 * @param out Not used.
	 */
	FLASH_RTS5817_EX_OP_WR_SR_NV,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FLASH_RTS5817_FLASH_API_EXTENSIONS_H_ */
