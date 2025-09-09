/*
 * Copyright (c) 2023 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  * @brief Header file for STM32 flash extended operations.
  * @ingroup stm32_flash_ex_op
  */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_STM32_FLASH_API_EXTENSIONS_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_STM32_FLASH_API_EXTENSIONS_H__

/**
 * @brief Extended operations for STM32 flash controllers.
 * @defgroup stm32_flash_ex_op STM32
 * @ingroup flash_ex_op
 * @{
 */

#include <zephyr/drivers/flash.h>

/**
 * @brief Enumeration for STM32 flash extended operations.
 */
enum stm32_ex_ops {
	/**
	 * STM32 sector write protection control.
	 *
	 * @kconfig_dep{CONFIG_FLASH_STM32_WRITE_PROTECT}
	 *
	 * As an input, this operation takes a flash_stm32_ex_op_sector_wp_in
	 * with two sector masks:
	 * first mask is used to enable protection on sectors, while second mask
	 * is used to do the opposite. If both masks are 0, then protection will
	 * remain unchanged. If same sector is set on both masks, protection
	 * will be enabled.
	 *
	 * As an output, sector mask with enabled protection is returned.
	 * Input can be NULL if we only want to get protected sectors.
	 * Output can be NULL if not needed.
	 */
	FLASH_STM32_EX_OP_SECTOR_WP = FLASH_EX_OP_VENDOR_BASE,
	/**
	 * STM32 sector readout protection control.
	 *
	 * @kconfig_dep{CONFIG_FLASH_STM32_READOUT_PROTECTION}
	 *
	 * As an input, this operation takes a flash_stm32_ex_op_rdp with information
	 * about desired RDP state. As an output, the status after applying changes
	 * is returned.
	 */
	FLASH_STM32_EX_OP_RDP,
	/**
	 * STM32 block option register.
	 *
	 * This operation causes option register to be locked until next boot.
	 * After calling, it's not possible to change option bytes (WP, RDP,
	 * user bytes).
	 */
	FLASH_STM32_EX_OP_BLOCK_OPTION_REG,
	/**
	 * STM32 block control register.
	 *
	 * This operation causes control register to be locked until next boot.
	 * After calling, it's not possible to perform basic operation like
	 * erasing or writing.
	 */
	FLASH_STM32_EX_OP_BLOCK_CONTROL_REG,
	/**
	 * STM32 option bytes read.
	 *
	 * Read the option bytes content, out takes a *uint32_t, in is unused.
	 */
	FLASH_STM32_EX_OP_OPTB_READ,
	/**
	 * STM32 option bytes write.
	 *
	 * Write the option bytes content, in takes the new value, out is
	 * unused. Note that the new value only takes effect after the device
	 * is restarted.
	 */
	FLASH_STM32_EX_OP_OPTB_WRITE,
};

/**
 * @brief Enumeration for STM32 QSPI extended operations.
 */
enum stm32_qspi_ex_ops {
	/**
	 * QSPI generic read command.
	 *
	 * Transmit the custom command and read the result to the user-provided
	 * buffer.
	 */
	FLASH_STM32_QSPI_EX_OP_GENERIC_READ,
	/**
	 * QSPI generic write command.
	 *
	 * Transmit the custom command and then write data taken from the
	 * user-provided buffer.
	 */
	FLASH_STM32_QSPI_EX_OP_GENERIC_WRITE,
};

/**
 * @brief Input structure for @ref FLASH_STM32_EX_OP_SECTOR_WP operation.
 */
struct flash_stm32_ex_op_sector_wp_in {
	/** Mask of sectors to enable protection on. */
	uint64_t enable_mask;
	/** Mask of sectors to disable protection on. */
	uint64_t disable_mask;
};

/**
 * @brief Output structure for @ref FLASH_STM32_EX_OP_SECTOR_WP operation.
 */
struct flash_stm32_ex_op_sector_wp_out {
	/** Mask of protected sectors. */
	uint64_t protected_mask;
};

/**
 * @brief Input structure for @ref FLASH_STM32_EX_OP_RDP operation.
 */
struct flash_stm32_ex_op_rdp {
	/** Whether to enable or disable the readout protection. */
	bool enable;
	/** Whether to make the readout protection permanent. */
	bool permanent;
};

/**
 * @}
 */

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_STM32_FLASH_API_EXTENSIONS_H__ */
